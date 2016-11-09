/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_buttons.c

DESCRIPTION
    This is the button interpreter for the Sink device
    This file extracts the button messages from the PIO subsystem and figures out
    the button press type and time. It passes the information to the button manager
    which is responsible for translating the button press generated into a system event
NOTES

*/
#include "sink_private.h"
#include "sink_buttonmanager.h"
#include "sink_buttons.h"
#include "sink_powermanager.h"
#include "sink_statemanager.h"
#include "sink_dut.h"
#include "sink_scan.h"

#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#endif

#include <pio_common.h>
#include <charger.h>
#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <pio.h>
#include <psu.h>
#include <stddef.h>

#ifdef ENABLE_CAPSENSE
#include <capsense.h>
#endif

#ifdef DEBUG_BUTTONS
#else
#endif


#define PIOGET() (PioGet32() & ~(VREG_PIN_MASK | CHG_PIN_MASK))

/* cap sense hardware is not available for a short time after fw boot, ignore any messages generated
   during this time */
#define CAPSENSE_INIT_INTERVAL 250

typedef enum ButtonsIntMsgTag
{
    B_MESSAGE_CAPSENSE_ENABLE,
    B_MULTIPLE_TIMER ,
    B_INTERNAL_TIMER ,
    B_REPEAT_TIMER
}ButtonsIntMsg_t;


/*
    LOCAL FUNCTION PROTOTYPES
 */
static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage )   ;
static bool ButtonsWasButtonPressed ( u32 pOldState , u32 pNewState) ;
static u32 ButtonsWhichButtonChanged ( u32 pOldState , u32 pNewState ) ;
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask ,u32 pButtonMask  , ButtonsTime_t pTime  ) ;
static void ButtonsEdgeDetect ( const u32 pState , ButtonsTaskData * pButtonsTask ) ;
static void ButtonsLevelDetect ( const u32 pState , ButtonsTaskData * pButtonsTask )  ;
static void ButtonsCheckDetection(u16 CapSenseState, u32 PioState);

/****************************************************************************
DESCRIPTION
    Initialises the Button Module parameters
*/
void ButtonsInit ( ButtonsTaskData *pButtonsTask )
{
#ifdef ENABLE_CAPSENSE
    u8 i;
    bool success;
#endif

    pButtonsTask->task.handler = ButtonsMessageHandler;

    /*connect the underlying PIO task to this task*/
    MessagePioTask(&pButtonsTask->task);

    /*connect the underlying Charger task to this task*/
    MessageChargerTask(&pButtonsTask->task);

#ifdef ENABLE_CAPSENSE
    /* set the update rate, currently a fast update rate to detect short touches */
    success = CapsenseConfigure(CAPSENSE_SET_CINT_UPDATE_DIVIDER, 0);

    /* set an initial trigger level for the cap sensors, this level will depend
       upon hardware and tests will need to be carried out to determine what a particular
       implementation requires */
    for (i = 0; success && (i < BM_CAP_SENSORS); i++)
       success = CapsenseConfigurePad(i, CAPSENSE_SET_TRIGGER_LEVEL, BM_CAP_SENSOR_LOW_SENSITIVITY);

    LOGD("B: capsense %s\n", success ? "OK" : "FAIL: check CONFIG_CAP_SENSE_PRELOAD");

    /* initialise task handler for capsense events after a short delay due to spurious events
       generated from the firmware during this time */
    MessageSendLater(&pButtonsTask->task,B_MESSAGE_CAPSENSE_ENABLE,0,CAPSENSE_INIT_INTERVAL);

#endif
}

/****************************************************************************
DESCRIPTION
    Called after the configuration has been read and will trigger buttons events
    if a pio has been pressed or held whilst the configuration was still being loaded
    , i.e. the power on button press
*/
void ButtonsCheckForChangeAfterInit(void)
{
    u32 input_state; /* contains translated pio and capsense bits, format is inputs */

    ButtonsTaskData * lButtonsTask = theSink.theButtonsTask ;

    lButtonsTask->gBTime = B_INVALID ;

    /* translate the pio and capsense bits into 'input' bits mask */
#ifdef ENABLE_CAPSENSE
    input_state = ButtonsTranslate(lButtonsTask->gOldCapState, (((PIOGET()^theSink.conf6->PIOIO.pio_invert) | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE)));
#else
    input_state = ButtonsTranslate(0, (((PIOGET()^theSink.conf6->PIOIO.pio_invert) | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE)));
#endif
    /* perform a level detect looking for transistion of recently added button definition, mask inputs
       against level configured inputs to prevent false button press indications */
    ButtonsLevelDetect ( (input_state & lButtonsTask->gPerformInputLevelCheck) , lButtonsTask ) ;

    /* perform an edge detect looking for transistion of recently added button definition, mask inputs
       against edge configured inputs to prevent false button press indications */
    ButtonsEdgeDetect ( (input_state & lButtonsTask->gPerformInputEdgeCheck) , lButtonsTask) ;

    /* store current input states in order to be able to detect the transition of an input (pio or capsense)
       in the button handler */
    lButtonsTask->gBOldInputState = input_state;
    lButtonsTask->gOldPIOState = (((PIOGET()^theSink.conf6->PIOIO.pio_invert) | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE));

    /* Debounce required PIO lines */
    if(!PioCommonDebounce((lButtonsTask->gButtonPIOLevelMask & ~(VREG_PIN_MASK|CHG_PIN_MASK)),
                           lButtonsTask->button_config->debounce_number,
                           lButtonsTask->button_config->debounce_period_ms ))
    {
        LOGD("B: **** ERROR **** PIO NOT AVAILABLE = 0x%lx\n",input_state);
#ifdef DEBUG_BUTTONS
        Panic();
#endif

    }

    ChargerDebounce( (CHARGER_VREG_EVENT|CHARGER_CONNECT_EVENT), lButtonsTask->button_config->debounce_number, lButtonsTask->button_config->debounce_period_ms );
    LOGD("B: initial buttoncheck\n");

}

/****************************************************************************
DESCRIPTION
    the button event message handler - converts button events to the system events
*/

static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage )
{
    ButtonsTaskData * lBTask = (ButtonsTaskData*)pTask ;

    LOGD("B:Message\n");
    switch ( pId )
    {
#ifdef ENABLE_CAPSENSE

        /* delay handling of messages until capsense hardware has settled down */
        case B_MESSAGE_CAPSENSE_ENABLE :
        {
            /* connect the cap sense task to this task */
            MessageCapsenseTask(&lBTask->task);
        }
        break;

        /* indication of a change of one of the capacitive touch sensors */
        case MESSAGE_CAPSENSE_CHANGED :
        {
            u8 i;
            const MessageCapsenseChanged * lMessage = ( const MessageCapsenseChanged * ) (pMessage ) ;
            u16 CurrentCapState = lBTask->gOldCapState;

            LOGD("B:Cap - Events = %x pad = %x dir = %x\n",lMessage->num_events,lMessage->event[0].pad,lMessage->event[0].direction);

            /* the cap sense changed message may contain more than one button press, check for other
               presses or releases */
            for(i=0;i<lMessage->num_events;i++)
            {
                /* update the state of any sensor changes, direction is inverted, 0 = down, 1 = up  */
                if(lMessage->event[i].direction == CAPSENSE_EVENT_POS)
                    CurrentCapState |= (1<<(lMessage->event[i].pad));
                /* due to the fact that the fw will not report multiple press need to maintain
                    which touch sensors are up and which are down and set/reset appropriately */
                else
                    CurrentCapState &= ~(1<<(lMessage->event[i].pad));

                /* as the touch sensors as less effective when releasing, increase sesitivity
                   when button indicates pressed to accurately detect the release event */
                if(lMessage->event[i].direction == CAPSENSE_EVENT_POS)
                {
                   if(!CapsenseConfigurePad(lMessage->event[i].pad, CAPSENSE_SET_TRIGGER_LEVEL, BM_CAP_SENSOR_HIGH_SENSITIVITY))
                       LOGD("B:Cap - invalid threshold %d\n",BM_CAP_SENSOR_LOW_SENSITIVITY);

                   /* button gone down, it is possible to record the time in mS at which the
                      button went down, this can be used to accurately determine how long the
                      button was held down for */
                   LOGD("B:Cap - Down Time %x = %ld mS\n",i,(u32)lMessage->event[i].time_ms);
                }
                /* return to original sensitivity when releasing button */
                else
                {
                   if(!CapsenseConfigurePad(lMessage->event[i].pad, CAPSENSE_SET_TRIGGER_LEVEL, BM_CAP_SENSOR_LOW_SENSITIVITY))
                       LOGD("B:Cap - invalid threshold %d\n",BM_CAP_SENSOR_LOW_SENSITIVITY);

                   /* button gone up, the time in mS can be used to get accurate duration information */
                   LOGD("B:Cap - Up Time %x = %ld mS\n",i,(u32)lMessage->event[i].time_ms);
                }
            }

            LOGD("B:Cap - state = %x\n",CurrentCapState);

            /* check whether the sensor status change requires an event to be generated */
            ButtonsCheckDetection(CurrentCapState, lBTask->gOldPIOState);

            /* update the last state value */
            lBTask->gOldCapState = CurrentCapState;
        }
        break;
#endif

        case MESSAGE_PIO_CHANGED :
        {
            const MessagePioChanged * lMessage = ( const MessagePioChanged * ) (pMessage ) ;
            /* get current pio state, eor with pio invert mask  */
            u32 lNewPioState = (u32)(theSink.conf6->PIOIO.pio_invert ^ (( lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) | (((u32)lMessage->state16to31)<<16)));

            LOGD("B:BMH - PIO_CHANGE: %x %x\n",lMessage->state16to31, lMessage->state);

#ifdef ENABLE_CAPSENSE

            /* check whether the pio status change requires an event to be generated */
            ButtonsCheckDetection(lBTask->gOldCapState, lNewPioState);
#else
            /* check whether the pio status change requires an event to be generated */
            ButtonsCheckDetection(0, lNewPioState);
#endif
        }
        break ;

        case MESSAGE_CHARGER_CHANGED:
        {
            const MessageChargerChanged *m = (const MessageChargerChanged *) (pMessage ) ;

            LOGD("B:BMH - CHG_CHANGE: %lx\n",(((u32)m->vreg_en_high << VREG_PIN) | ((u32)m->charger_connected << CHG_PIN) | (PIOGET()^theSink.conf6->PIOIO.pio_invert)));

            /* when a charger or vreg change event is detectecd perform both an edge and level detection
               passing in only those approriately masked pios for edge or level configured buttons */
                        /* check whether the pio status change requires an event to be generated */
#ifdef ENABLE_CAPSENSE
            ButtonsCheckDetection(lBTask->gOldCapState, (((u32)m->vreg_en_high << VREG_PIN) | ((u32)m->charger_connected << CHG_PIN) | (PIOGET()^theSink.conf6->PIOIO.pio_invert)));
#else
            ButtonsCheckDetection(0, (((u32)m->vreg_en_high << VREG_PIN) | ((u32)m->charger_connected << CHG_PIN) | (PIOGET()^theSink.conf6->PIOIO.pio_invert)));
#endif
        }
        break;

        case B_MULTIPLE_TIMER:
        {
                /*if we have reached here, then a double timer has been received*/
            LOGD("B:Double[%lx][%x]\n", lBTask->gBMultipleState , B_SHORT_SINGLE);

            /* when the multiple press timer has expired, check to see if a double press was
               made and if so indicate it */
            if(lBTask->gBTapCount == DOUBLE_PRESS)
                ButtonsButtonDetected ( lBTask , (lBTask->gBMultipleState & lBTask->gPerformInputLevelCheck) , B_DOUBLE  );
            /* no double press and no triple press, therefore indicate as a short single press */
            else
                ButtonsButtonDetected ( lBTask, (lBTask->gBMultipleState & lBTask->gPerformInputLevelCheck), B_SHORT_SINGLE );

            /* reset the multiple button press count */
            lBTask->gBTapCount = 0 ;
            lBTask->gBMultipleState = 0x0000;
        }
        break ;

        case B_INTERNAL_TIMER:
        {
            /*if we have reached here, then the buttons have been held longer than one of the timed messages*/
            LOGD("B:Timer\n");

            /* since a long/vlong or vvlong has been triggered, cancel any pending double press checks */
            lBTask->gBMultipleState = 0x0000 ;
            lBTask->gBTapCount = 0 ;
            MessageCancelAll ( &lBTask->task , B_MULTIPLE_TIMER ) ;

            /* an internal timer has triggered which was initiated from the level detect function call */
            if ( lBTask->gBTime == B_VERY_LONG )
            {
                /* update timer state flag */
                lBTask->gBTime = B_VERY_VERY_LONG ;
            }
            /* a long press timer event has triggered */
            else if ( lBTask->gBTime == B_LONG )
            {
                /* don't send very very long timer message until needed, i.e. very_long timer expired */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER , 0 ,  (lBTask->button_config->very_very_long_press_time - lBTask->button_config->very_long_press_time ) ) ;
                /* update tiemr state flag */
                lBTask->gBTime = B_VERY_LONG ;
                /*notify the app that the timer has expired*/
                MessageSend( &theSink.task , EventSysVLongTimer , 0 ) ;
            }
            /* the first timer event triggered from the level detect call */
            else
            {
                /* only send very long message when long timer expired to save messaging.                 */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER , 0 , (lBTask->button_config->very_long_press_time - lBTask->button_config->long_press_time)) ;
                /*notify the app that the timer has expired*/
                MessageSend( &theSink.task , EventSysLongTimer , 0 ) ;
                lBTask->gBTime = B_LONG ;
            }
            /*indicate that we have received a message */
            ButtonsButtonDetected ( lBTask, (lBTask->gBOldInputState & lBTask->gPerformInputLevelCheck) , lBTask->gBTime );
        }
        break ;

        case B_REPEAT_TIMER:
        {
            /*if we have reached here, the repeat time has been reached so send a new message*/
            LOGD("B:Repeat[%lx][%x]\n", lBTask->gBOldInputState , B_REPEAT  );

            /*send another repeat message*/
            MessageSendLater ( &lBTask->task , B_REPEAT_TIMER , 0 ,  lBTask->button_config->repeat_time ) ;

            ButtonsButtonDetected ( lBTask, (lBTask->gBOldInputState & lBTask->gPerformInputLevelCheck) , B_REPEAT );
        }
        break;
        default :
           LOGD("B:?[%x]\n",pId);
        break ;
    }
}



/****************************************************************************
DESCRIPTION
    helper method - returns true if a button was pressed
*/
static bool ButtonsWasButtonPressed ( u32 pOldState , u32 pNewState)
{
    bool lWasButtonPressed = FALSE ;

    u32 lButton = ButtonsWhichButtonChanged ( pOldState , pNewState ) ;

    if ( ( lButton & pNewState ) != 0 )
    {
        lWasButtonPressed = TRUE ;
    }

    LOGD("B:But WasPressed OldSt[%lx] NewSt[%lx] Input[%lx] Pressed[%x]\n", pOldState, pNewState, lButton,lWasButtonPressed);

    return lWasButtonPressed ;
}
/****************************************************************************
DESCRIPTION
    helper method - returns mask ofwhich button changed
*/
static u32 ButtonsWhichButtonChanged ( u32 pOldState , u32 pNewState )
{
    u32 lWhichButton = 0 ;

    lWhichButton = (pNewState ^ pOldState ) ;

    return lWhichButton ;
}

/****************************************************************************
DESCRIPTION
    function to handle a button press - informs button manager of a change
    currently makes direct call - may want to use message handling (tasks)
*/
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask, u32 pButtonMask  , ButtonsTime_t pTime )
{
    LOGD("B:But Det[%lx]\n", pButtonMask);

    if( pButtonMask == 0 )
    {
        MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
    }
    else
    {
        BMButtonDetected ( pButtonMask, pTime ) ;

        if (stateManagerGetState() == deviceTestMode)
        {
            checkDUTKeyRelease(pButtonMask, pTime);
        }
    }
}

/****************************************************************************
DESCRIPTION
    function to detect level changes of buttons / multiple buttons, both
    PIO and capsense
*/
static void ButtonsLevelDetect ( const u32 pInput , ButtonsTaskData * pButtonsTask )
{
    u32 lNewInput = (u32) (pInput & (pButtonsTask->gPerformInputLevelCheck)) ;
    u32 lOldInput = (u32) (pButtonsTask->gBOldInputState & pButtonsTask->gPerformInputLevelCheck);

    LOGD("But Lev Det|:NewInput[%lx] OldInput[%lx]\n", lNewInput, pButtonsTask->gBOldInputState );

    if ( ButtonsWasButtonPressed(lOldInput, lNewInput )  )
    {
        /* check whether device needs to be made connectable as a result of a button press
           on a multipoint device */
        if(theSink.features.GoConnectableButtonPress)
            sinkEnableMultipointConnectable();

        /*cancel all previously timed messages*/
        MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER ) ;
        MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;

        /* send new timed messages*/
        MessageSendLater ( &pButtonsTask->task , B_INTERNAL_TIMER , 0 ,  pButtonsTask->button_config->long_press_time ) ;
        MessageSendLater ( &pButtonsTask->task , B_REPEAT_TIMER   , 0 ,  pButtonsTask->button_config->repeat_time ) ;

        /*having restrted the timers, reset the time*/
        pButtonsTask->gBTime = B_SHORT ;

        if (stateManagerGetState() == deviceTestMode)
        {
            checkDUTKeyPress(lNewInput);
        }
    }
    /*button was released or was masked out, check to make sure there is an input bit change as vreg enable
      can generate an addition MSG without any input's changing state */
    else if(lOldInput!= lNewInput )
    {
        /*it was only a released if there was a button actually pressed last time around -
          buttons we have masked out still end up here but no state changes are made  */
        if ( lOldInput != 0 )
        {
             /*if we have had a double press in the required time
                 and the button pressed was the same as this one*/
             if (  (pButtonsTask->gBTapCount ) && (lOldInput == pButtonsTask->gBMultipleState ) )
             {
                /* button has been released, increment multiple press counter */
                 pButtonsTask->gBTapCount++;

                 LOGD("TapCount: [%lx][%lx][%x]\n", lOldInput , pButtonsTask->gBMultipleState , pButtonsTask->gBTapCount  );

                 /* if the multiple press count is 2, set the time as a double, this will be used
                    if no further presses are detected before the multiple press timeout occurs */
                 if(pButtonsTask->gBTapCount == DOUBLE_PRESS)
                    pButtonsTask->gBTime = B_DOUBLE ;
                 /* if the multiple press count is 3, process immediately as a triple press and cancel the
                    multiple press timer */
                 else if(pButtonsTask->gBTapCount == TRIPLE_PRESS)
                 {
                    pButtonsTask->gBTime = B_TRIPLE ;
                    /* indicate a triple press has been detected */
                    ButtonsButtonDetected ( pButtonsTask , lOldInput , B_TRIPLE  );
                    /* reset current state and multiple press counter */
                    pButtonsTask->gBMultipleState = 0x0000 ;
                    pButtonsTask->gBTapCount = 0;
                    MessageCancelAll ( &pButtonsTask->task , B_MULTIPLE_TIMER ) ;
                 }
             }

             /*only send a message if it was a short one - long / v long /double handled elsewhere*/
             if ( (pButtonsTask->gBTime == B_SHORT ) )
             {
                 ButtonsButtonDetected ( pButtonsTask , lOldInput , B_SHORT  );

                 /*store the double state*/
                 pButtonsTask->gBMultipleState = lOldInput ;
                 pButtonsTask->gBTapCount++;

                    /*start the double timer - only applicable to a short press*/
                 MessageSendLater ( &pButtonsTask->task , B_MULTIPLE_TIMER , 0 ,pButtonsTask->button_config->double_press_time ) ;
             }
             else if ( (pButtonsTask->gBTime == B_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , lOldInput , B_LONG_RELEASE  );
             }
             else if ( (pButtonsTask->gBTime == B_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , lOldInput , B_VERY_LONG_RELEASE  );
             }
             else if ( (pButtonsTask->gBTime == B_VERY_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , lOldInput , B_VERY_VERY_LONG_RELEASE  );
             }

             if (pButtonsTask->gBTime != B_INVALID)
             {
                MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER) ;
                MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
             }

             /*removing this allows all releases to generate combination presses is this right?*/
             if ( !lNewInput )
             {
                 pButtonsTask->gBTime = B_INVALID ;
             }
         }
    }
}



/****************************************************************************

DESCRIPTION
    function to detect edge changes of buttons / multiple buttons.

*/
static void ButtonsEdgeDetect ( const u32 pInput , ButtonsTaskData * pButtonsTask )
{
    u32 lNewInput = (u32) (pInput & (pButtonsTask->gPerformInputEdgeCheck) ) ;
    u32 lOldInput = (u32) (pButtonsTask->gBOldInputState & pButtonsTask->gPerformInputEdgeCheck);
    u32 lInput = ButtonsWhichButtonChanged( lOldInput , lNewInput ) ;

    LOGD("But Edge Det: Old Edge[%lx] New Edge[%lx] Button[%lx] Low2High[%lx}\n", lOldInput , lNewInput ,lInput ,(lNewInput & lInput)  );

    /*if a button has changed*/
    if ( lInput )
    {
            /*determine which edge has been received and process accordingly*/
        if ( lNewInput & lInput )
        {
            ButtonsButtonDetected ( pButtonsTask , lInput , B_LOW_TO_HIGH )   ;
            /* check whether device needs to be made connectable as a result of a button press
               on a multipoint device */
            if(theSink.features.GoConnectableButtonPress)
                sinkEnableMultipointConnectable();
        }
        else
        {
            ButtonsButtonDetected ( pButtonsTask , lInput , B_HIGH_TO_LOW  )   ;
        }
    }
}


/****************************************************************************

DESCRIPTION
    function to detect edge changes of buttons / multiple buttons.

*/
static void ButtonsCheckDetection(u16 CapSenseState, u32 PioState)
{
    ButtonsTaskData * lBTask = theSink.theButtonsTask ;

    /* take the current capacitive touch state and pio state and translate into a u32
       input state used for checking button presses */
    u32 lInputState = ButtonsTranslate(CapSenseState, PioState);

    LOGD("But CheckDet: Inputs: Cap[%x] PIO[%lx] Inputs[%lx] OldState[%lx]\n", CapSenseState , PioState , lInputState, lBTask->gBOldInputState);
    LOGD("But CheckDet: Masks:  Edge [%lx] Lev [%lx]\n",lBTask->gPerformInputEdgeCheck, lBTask->gPerformInputLevelCheck);

    /* when an input is configured for an edge detect only there is significant performance gain to be had
       by only doing an edge detect call and not a level detect. To do this use a previously set edge
       detect mask and check this against the current pio being reported. Also need to check if a previously
       set PIO has now been removed and check for the edge transition once again. */
    if((lBTask->gPerformInputEdgeCheck & lInputState) ||
       (lBTask->gPerformInputEdgeCheck & lBTask->gBOldInputState))
    {
        /* check for a valid edge transition against current pio states masked with edge configured pios
           and perform appropriate action */
        ButtonsEdgeDetect  ( lInputState, lBTask ) ;
    }

    /* only do a level detect call which is vm/messaging intensive when a pio has been configured as
       short or long or very long or very very long, i.e. not rising or falling */
    if((lBTask->gPerformInputLevelCheck & lInputState ) ||
       (lBTask->gPerformInputLevelCheck & lBTask->gBOldInputState))
    {
        /* perform a level detection, this call uses a number of messages and is quite slow to process */
        ButtonsLevelDetect ( lInputState, lBTask) ;
    }

#ifdef ENABLE_GAIA
    gaiaReportPioChange(PioState);
#endif

    /* update last button state for next time around */
    lBTask->gBOldInputState = lInputState;
    lBTask->gOldPIOState = PioState;
}

/****************************************************************************

DESCRIPTION
    this function remaps the cap sense and pio bitmask into an input assignment
    pattern specified by pskey user 10, this allows buttons to be triggered from
    pios of anywhere from 0 to 31 and cap sense 0 to 5

*/
u32 ButtonsTranslate(u16 CapSenseState, u32 PioState)
{
    ButtonsTaskData * lBTask = theSink.theButtonsTask ;
    u32 lResult = 0;
    u8 i;

    /* merge capacitive touch switch data and pio data using the button translation
       data, cycle through all 16 translations */
    for(i=0;i<BM_NUM_BUTTON_TRANSLATIONS;i++)
    {
        /* if the button is assigned to a pio then check the pio state */
        if(lBTask->gTranslations[i].input_source == B_PIO)
        {
            /* if pio state is set then add to result */
            if(PioState & ((u32)1<<lBTask->gTranslations[i].input_number))
                lResult |= ((u32)1<<lBTask->gTranslations[i].button_no);
        }
        /* if not a pio check for cap sense input source */
        else if(lBTask->gTranslations[i].input_source == B_CAP)
        {
            /* if capsense state is set then add to result */
            if(CapSenseState & ((u32)1<<lBTask->gTranslations[i].input_number))
                lResult |= ((u32)1<<lBTask->gTranslations[i].button_no);
        }
    }
    LOGD("But Trans: Cap[%x] Pio[%lx] Input[%lx]\n", CapSenseState , PioState , lResult );
    /* return 32 bit result as a mixture of pio and cap sense inputs */
    return lResult;

}


/*
DESCRIPTION
    this function remaps an input assignment into a cap sense or pio bitmask
    pattern specified by pskey user 10

*/
u32 ButtonsTranslateInput(u16 block, u16 index, bool include_cap_sense)
{
    ButtonsTaskData * lButtonsTask = theSink.theButtonsTask ;
    ButtonEvents_t * lButtonEvent = &lButtonsTask->gButtonEvents[block][index];

    if ( lButtonEvent )
    {
        u8 i;
        u32 mask = 0;
        /* get current input button requirements */
        u32 lMask = (u32)(lButtonEvent->ButtonMaskLS | (u32)(lButtonEvent->ButtonMaskVC) << VREG_PIN);


        /* search through the translation table for matching buttons */
        for(i=0;i<BM_NUM_BUTTON_TRANSLATIONS;i++)
        {
            /* when matching translation found set the translated bit which will be used
               for edge and level checking on the translated pio/cap sense data bits */
            if(lMask & ((u32)1<<lButtonsTask->gTranslations[i].button_no))
            {
                /* only include the cap sense bits if specifically required */
                if((include_cap_sense)||
                   ((!include_cap_sense)&&(lButtonsTask->gTranslations[i].input_source != B_CAP)))
                {
                    mask |= ((u32)1<<lButtonsTask->gTranslations[i].input_number);
                }
            }
        }

 /*       LOGD("But Trans inputs =[0x%lx] translated =[0x%lx]\n", lMask , mask );       */

        return mask;
    }

    LOGD("But Trans input: invalid button event[0x%x][0x%x]\n", block , index );
#ifdef DEBUG_BUTTONS
    Panic();
#endif
    return FALSE; /* ERROR */

}
