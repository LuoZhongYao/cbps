/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_display.c

DESCRIPTION

*/

#include "sink_private.h"
#include "sink_display.h"
#include "sink_statemanager.h"
#include <display.h>
#include <string.h>
#include <display_plugin_if.h>
#include <display_example_plugin.h>
#include <display_plugin_cns10010.h>

#ifdef ENABLE_DISPLAY

#ifdef DEBUG_DISPLAY
#else
#endif


/*
#define DISPLAY_PLUGIN (TaskData *) &display_example_plugin
*/
#define DISPLAY_PLUGIN (TaskData *) &display_plugin_cns10010


/****************************************************************************
NAME
    displayInit

DESCRIPTION
    Initialise the display

RETURNS
    void
*/

void displayInit(void)
{
    LOGD("DISPLAY: INIT\n");
    DisplayInit(DISPLAY_PLUGIN, &theSink.task);
}



/****************************************************************************
NAME
    displaySetState

DESCRIPTION
    Turn display off (state FALSE) or on (state TRUE)

RETURNS
    void
*/
void displaySetState(bool state)
{
    LOGD("DISPLAY: state %u\n", state);
    DisplaySetState(state);
}


/****************************************************************************
NAME
    displayShowText

DESCRIPTION
    Display text on display

RETURNS
    void
*/
void displayShowText(char* text,
                     u8 txtlen,
                     u8 line,
                     bool  scroll,
                     u16 scroll_update,
                     u16 scroll_pause,
                     bool  flash,
                     u16  display_time  )
{
    LOGD("DISPLAY: text %u \"%*s\" @ %u\n", txtlen, txtlen, text, line);
    DisplaySetText(text, txtlen, line, scroll, scroll_update, scroll_pause, flash, display_time);
}


/****************************************************************************
NAME
    displayShowSimpleText

DESCRIPTION
    Simplified interface to display text

RETURNS
    void
*/
void displayShowSimpleText(char* text, u8 line)
{
    displayShowText(text,  strlen(text), line, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
}


/****************************************************************************
NAME
    displayUpdateIcon

DESCRIPTION
    Updates the state of an icon on the display

RETURNS
    void
*/
void displayUpdateIcon(u8 icon, bool state)
{
    LOGD("DISPLAY: icon %u = %u\n", icon, state);
    DisplaySetIcon(icon, state);
}


/****************************************************************************
NAME
    displayUpdateVolume

DESCRIPTION
    Updates the state of the volume on the display

RETURNS
    void
*/
void displayUpdateVolume( i16 vol )
{
    LOGD("DISPLAY: vol %u\n", vol);
    DisplaySetVolume(vol);
}


/****************************************************************************
NAME
    displayUpdateBatteryLevel

DESCRIPTION
    Updates the state of the battery level on the display

RETURNS
    void
*/
void displayUpdateBatteryLevel(bool charging)
{
    if (charging)
    {
        LOGD("DISPLAY: batt charging\n");
        DisplaySetBatteryLevel(0xff);
    }
    else
    {   /* update battery display */
        voltage_reading reading;
        PowerBatteryGetVoltage(&reading);
        LOGD("DISPLAY: batt %u", reading.level);
        DisplaySetBatteryLevel(reading.level);
    }
}

/****************************************************************************
NAME
    displayUpdateAppState

DESCRIPTION
    Updates the the display with the application state

RETURNS
    void
*/
void displayUpdateAppState (sinkState newState)
{
    LOGD("DISPLAY: state %u\n", newState);

    switch (newState)
    {
        case deviceLimbo:
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
            break;
        case deviceConnectable:
            break;
        case deviceConnDiscoverable:
            displayShowSimpleText(DISPLAYSTR_PAIRING,2);
            break;
        case deviceConnected:
            {
                /* only display connected if moving out of limbo/connectable/conn-disc */
                if (newState > stateManagerGetState())
                    displayShowText(DISPLAYSTR_CONNECTED,  strlen(DISPLAYSTR_CONNECTED), 2, DISPLAY_TEXT_SCROLL_SCROLL, 500, 1000, FALSE, 20);
            }
            break;
        case deviceOutgoingCallEstablish:
            displayShowSimpleText(DISPLAYSTR_OUTGOINGCALL,2);
            break;
        case deviceIncomingCallEstablish:
             displayShowSimpleText(DISPLAYSTR_INCOMINGCALL,2);
            break;
        case deviceActiveCallSCO:
            displayShowSimpleText(DISPLAYSTR_ACTIVECALL,2);
            break;
        case deviceTestMode:
            displayShowSimpleText(DISPLAYSTR_TESTMODE,2);
            break;
        case deviceThreeWayCallWaiting:
            displayShowSimpleText(DISPLAYSTR_TWCWAITING,2);
            break;
        case deviceThreeWayCallOnHold:
            displayShowSimpleText(DISPLAYSTR_TWCONHOLD,2);
            break;
        case deviceThreeWayMulticall:
            displayShowSimpleText(DISPLAYSTR_TWCMULTI,2);
            break;
        case deviceIncomingCallOnHold:
            displayShowSimpleText(DISPLAYSTR_INCOMINGONHOLD,2);
            break;
        case deviceActiveCallNoSCO:
            displayShowSimpleText(DISPLAYSTR_ACTIVECALLNOSCO,2);
            break;
        case deviceA2DPStreaming:
            break;
        case deviceLowBattery:
            displayShowSimpleText(DISPLAYSTR_LOWBATTERY,2);
            break;

        default:
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
    }
}

#endif /*ENABLE_DISPLAY*/
