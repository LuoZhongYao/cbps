/****************************************************************************
Copyright (C) Qualcomm Technologies International, Ltd. 2005-2015

FILE NAME
    main.c

DESCRIPTION
    This is main file for the application software for a sink device

NOTES

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_init.h"
#include "sink_auth.h"
#include "sink_scan.h"
#include "sink_slc.h"
#include "sink_inquiry.h"
#include "sink_devicemanager.h"
#include "sink_link_policy.h"
#include "sink_indicators.h"
#include "sink_dut.h"
#include "sink_pio.h"
#include "sink_multipoint.h"
#include "sink_led_manager.h"
#include "sink_buttonmanager.h"
#include "sink_configmanager.h"
#include "sink_events.h"
#include "sink_statemanager.h"
#include "sink_states.h"
#include "sink_powermanager.h"
#include "sink_callmanager.h"
#include "sink_csr_features.h"
#include "sink_usb.h"
#include "sink_display.h"
#include "sink_speech_recognition.h"
#include "sink_a2dp.h"
#include "sink_config.h"
#include "sink_audio_routing.h"
#include "sink_partymode.h"
#include "sink_leds.h"
#include "sink_fm.h"
#include "sink_anc.h"

/* BLE related headers */
#include "sink_ble.h"
#include "sink_ble_gap.h"
#include "sink_ble_advertising.h"
#include "sink_ble_scanning.h"

#include "sink_gatt_server_ias.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_server_lls.h"
#include "sink_gatt_client_ias.h"

#ifdef ENABLE_IR_REMOTE
#include "sink_ir_remote_control.h"
#endif
#ifdef ENABLE_PBAP
#include "sink_pbap.h"
#endif
#ifdef ENABLE_MAPC
#include "sink_mapc.h"
#endif
#ifdef ENABLE_AVRCP
#include "sink_avrcp.h"
#endif
#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif
#ifdef ENABLE_PEER
#include "sink_peer.h"
#endif
#include "sink_linkloss.h"

#include "sink_avrcp_qualification.h"
#include "sink_peer_qualification.h"


#include "sink_volume.h"
#include "sink_tones.h"
#include "sink_audio_prompts.h"

#include "sink_audio.h"
#include "sink_at_commands.h"
#include "vm.h"

#ifdef TEST_HARNESS
#include "test_sink.h"
#include "vm2host_connection.h"
#include "vm2host_hfp.h"
#include "vm2host_a2dp.h"
#include "vm2host_avrcp.h"
#endif

#include <library.h>
#include <bdaddr.h>
#include <connection.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <codec.h>
#include <boot.h>
#include <string.h>
#include <audio.h>
#include <sink.h>
#include <kalimba_standard_messages.h>
#include <audio_plugin_if.h>
#include <print.h>
#include <loader.h>
#include <pio_common.h>

#ifdef ENABLE_DISPLAY
#include <display.h>
#include <display_plugin_if.h>
#include <display_example_plugin.h>
#endif /* ENABLE_DISPLAY */


#ifdef DEBUG_MAIN
    #define TRUE_OR_FALSE(x)  ((x) ? 'T':'F')
#else
#endif

#if defined ENABLE_PEER && defined PEER_TWS
static const u16 tws_audio_routing[4] =
{
    (PEER_TWS_ROUTING_STEREO << 2) | (PEER_TWS_ROUTING_STEREO),  /* Master stereo, Slave stereo */
    (  PEER_TWS_ROUTING_LEFT << 2) | ( PEER_TWS_ROUTING_RIGHT),
    ( PEER_TWS_ROUTING_RIGHT << 2) | (  PEER_TWS_ROUTING_LEFT),
    (  PEER_TWS_ROUTING_DMIX << 2) | (  PEER_TWS_ROUTING_DMIX)

};
#endif

#define IS_SOURCE_CONNECTED ((!usbIsAttached()) && (!analogAudioConnected()) && \
                             (!spdifAudioConnected()) && (!sinkFmIsFmRxOn()))


/* Single instance of the device state */
hsTaskData theSink;

static void handleHFPStatusCFM ( hfp_lib_status pStatus ) ;

#ifdef HOSTED_TEST_ENVIRONMENT
extern void _sink_init(void);
#else
extern void _init(void);
#endif
static void sinkInitCodecTask( void ) ;
static void IndicateEvent(MessageId id);

/*************************************************************************
NAME
    sinkSendLater
    
DESCRIPTION
    Send an event to the application main task after a delay, cancelling
    any already queued

RETURNS
    void
*/
void sinkSendLater(sinkEvents_t event, u32 delay)
{
    MessageCancelAll(&theSink.task, event);
    MessageSendLater(&theSink.task, event, NULL, delay);
}


/*************************************************************************
NAME    
    handleCLMessage

DESCRIPTION
    Function to handle the CL Lib messages - these are independent of state

RETURNS

*/
static void handleCLMessage ( Task task, MessageId id, Message message )
{
    LOGD("CL = [%x]\n", id);

        /* Handle incoming message identified by its message ID */
    switch(id)
    {
        case CL_INIT_CFM:
            LOGD("CL_INIT_CFM [%d]\n" , ((CL_INIT_CFM_T*)message)->status );
            if(((CL_INIT_CFM_T*)message)->status == success)
            {
                /* Initialise the codec task */
                sinkInitCodecTask();

#ifdef ENABLE_GAIA
                /* Initialise Gaia with a concurrent connection limit of 1 */
                GaiaInit(task, 1);
#endif

            }
            else
            {
                Panic();
            }
        break;
        case CL_DM_WRITE_INQUIRY_MODE_CFM:
            /* Read the local name to put in our EIR data */
            ConnectionReadInquiryTx(&theSink.task);
        break;
        case CL_DM_READ_INQUIRY_TX_CFM:
            theSink.inquiry_tx = ((CL_DM_READ_INQUIRY_TX_CFM_T*)message)->tx_power;
            ConnectionReadLocalName(&theSink.task);
        break;
        case CL_DM_LOCAL_NAME_COMPLETE:
            LOGD("CL_DM_LOCAL_NAME_COMPLETE\n");
            /* Write EIR data and initialise the codec task */
            sinkWriteEirData((CL_DM_LOCAL_NAME_COMPLETE_T*)message);
            break;

        case CL_SM_SEC_MODE_CONFIG_CFM:
            LOGD("CL_SM_SEC_MODE_CONFIG_CFM\n");
            /* Remember if debug keys are on or off */
            theSink.debug_keys_enabled = ((CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
        break;
        case CL_SM_PIN_CODE_IND:
            LOGD("CL_SM_PIN_IND\n");
            sinkHandlePinCodeInd((CL_SM_PIN_CODE_IND_T*) message);
        break;
        case CL_SM_USER_CONFIRMATION_REQ_IND:
            LOGD("CL_SM_USER_CONFIRMATION_REQ_IND\n");
            sinkHandleUserConfirmationInd((CL_SM_USER_CONFIRMATION_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_REQ_IND:
            LOGD("CL_SM_USER_PASSKEY_REQ_IND\n");
            sinkHandleUserPasskeyInd((CL_SM_USER_PASSKEY_REQ_IND_T*) message);
        break;
        case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
            LOGD("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n");
            sinkHandleUserPasskeyNotificationInd((CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*) message);
        break;
        case CL_SM_KEYPRESS_NOTIFICATION_IND:
        break;
        case CL_SM_REMOTE_IO_CAPABILITY_IND:
            LOGD("CL_SM_IO_CAPABILITY_IND\n");
            sinkHandleRemoteIoCapabilityInd((CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
        break;
        case CL_SM_IO_CAPABILITY_REQ_IND:
            LOGD("CL_SM_IO_CAPABILITY_REQ_IND\n");
            sinkHandleIoCapabilityInd((CL_SM_IO_CAPABILITY_REQ_IND_T*) message);
        break;
        case CL_SM_AUTHORISE_IND:
            LOGD("CL_SM_AUTHORISE_IND\n");
            sinkHandleAuthoriseInd((CL_SM_AUTHORISE_IND_T*) message);
        break;
        case CL_SM_AUTHENTICATE_CFM:
            LOGD("CL_SM_AUTHENTICATE_CFM\n");
            sinkHandleAuthenticateCfm((CL_SM_AUTHENTICATE_CFM_T*) message);
        break;
#ifdef ENABLE_SUBWOOFER
        case CL_SM_GET_AUTH_DEVICE_CFM: /* This message should only be sent for subwoofer devices */
            LOGD("CL_SM_GET_AUTH_DEVICE_CFM\n");
            handleSubwooferGetAuthDevice((CL_SM_GET_AUTH_DEVICE_CFM_T*) message);
#endif
        break;

#ifdef ENABLE_PEER
        case CL_SM_GET_AUTH_DEVICE_CFM:
            LOGD("CL_SM_GET_AUTH_DEVICE_CFM\n");
            handleGetAuthDeviceCfm((CL_SM_GET_AUTH_DEVICE_CFM_T *)message);
        break;
        case CL_SM_ADD_AUTH_DEVICE_CFM:
            LOGD("CL_SM_ADD_AUTH_DEVICE_CFM\n");
            handleAddAuthDeviceCfm((CL_SM_ADD_AUTH_DEVICE_CFM_T *)message);
        break;
#endif

        case CL_DM_REMOTE_FEATURES_CFM:
            LOGD("Supported Features\n");
        break ;
        case CL_DM_INQUIRE_RESULT:
            LOGD("Inquiry Result\n");
            inquiryHandleResult((CL_DM_INQUIRE_RESULT_T*)message);
        break;
        case CL_SM_GET_ATTRIBUTE_CFM:
            LOGD("CL_SM_GET_ATTRIBUTE_CFM Vol:%d \n",((CL_SM_GET_ATTRIBUTE_CFM_T *)(message))->psdata[0]);
        break;
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
            LOGD("CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , ((CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T*)message)->status);
        break ;

        case CL_DM_LOCAL_BD_ADDR_CFM:
            DutHandleLocalAddr((CL_DM_LOCAL_BD_ADDR_CFM_T *)message);
        break ;
        
        
        case CL_DM_ROLE_IND:
            linkPolicyHandleRoleInd((CL_DM_ROLE_IND_T *)message);
        break;
        case CL_DM_ROLE_CFM:
            linkPolicyHandleRoleCfm((CL_DM_ROLE_CFM_T *)message);
        break;
        case CL_SM_SET_TRUST_LEVEL_CFM:
            LOGD("CL_SM_SET_TRUST_LEVEL_CFM status %x\n",((CL_SM_SET_TRUST_LEVEL_CFM_T*)message)->status);
        break;
        case CL_DM_ACL_OPENED_IND:
            LOGD("ACL Opened\n");
        break;
        case CL_DM_ACL_CLOSED_IND:
            LOGD("ACL Closed\n");
#ifdef ENABLE_AVRCP
            if(theSink.features.avrcp_enabled)
            {
                sinkAvrcpAclClosed(((CL_DM_ACL_CLOSED_IND_T *)message)->taddr.addr);
            }
#endif
        break;

/* BLE Messages */
        case CL_DM_BLE_ADVERTISING_REPORT_IND:
        {
            LOGD("CL_DM_BLE_ADVERTISING_REPORT_IND\n");
            bleHandleScanResponse((CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message);
        }
        break;
        case CL_DM_BLE_SET_ADVERTISING_DATA_CFM:
        {
            bleHandleSetAdvertisingData( (CL_DM_BLE_SET_ADVERTISING_DATA_CFM_T*)message );
        }
        break;
        case CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM:
        {
            LOGD("CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM\n");
        }
        break;
        case CL_DM_BLE_SECURITY_CFM:
        {
            LOGD("CL_DM_BLE_SECURITY_CFM [%x]\n", ((CL_DM_BLE_SECURITY_CFM_T*)message)->status);
        }
        break;
        case CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM:
        {
            LOGD("CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM [%x]\n", ((CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM_T*)message)->status);
        }
        break;
        case CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM:
        {
            LOGD("CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM [%x]\n", ((CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM_T*)message)->status);
        }
        break;
        case CL_DM_BLE_SET_SCAN_PARAMETERS_CFM:
        {
            LOGD("CL_DM_BLE_SET_SCAN_PARAMETERS_CFM [%x]\n", ((CL_DM_BLE_SET_SCAN_PARAMETERS_CFM_T*)message)->status);
        }
        break;
        case CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM:
        {
            LOGD("CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM\n");
        }
        break;
        case CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM:
        {
            LOGD("CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM num[%d]\n", ((CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM_T*)message)->white_list_size);
        }
        break;
        case CL_DM_BLE_CLEAR_WHITE_LIST_CFM:
        {
            LOGD("CL_DM_BLE_CLEAR_WHITE_LIST_CFM\n");
        }
        break;
        case CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM:
        {
            LOGD("CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM status[%u]\n", ((CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T*)message)->status);
            sinkBleGapAddDeviceWhiteListCfm((CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T*)message);
        }
        break;
        case CL_DM_BLE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM:
        {
            LOGD("CL_DM_BLE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM\n");
        }
        break;
        case CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM:
        {
            LOGD("CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM\n");
        }
        break;
        case CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND:
        {
            CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T * req = (CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T*)message;
            bool accept = TRUE;         /* Accept or reject the parameter update; will be rejected if minimum latency requested is lower than minimum allowed */

            /* TODO : Handle for role : master */
            /* Respond to the parameter update request */
            ConnectionDmBleAcceptConnectionParUpdateResponse(accept, &req->taddr, req->id, req->conn_interval_min, req->conn_interval_max, req->conn_latency, req->supervision_timeout);
        }
        break;
        case CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND:
        {
            LOGD("CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND [%x]\n", ((CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T*)message)->status);
            
            sinkBleSimplePairingCompleteInd((CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T*)message);
        }
        break;
        case CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND:
        {
            LOGD("CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND\n");
        }
        break;

            /* filter connection library messages */
        case CL_SDP_REGISTER_CFM:
        case CL_SM_ENCRYPT_CFM:
        case CL_DM_WRITE_INQUIRY_ACCESS_CODE_CFM:
        case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        case CL_SM_ENCRYPTION_CHANGE_IND:
            break;
        break;
        
        /*all unhandled connection lib messages end up here*/
        default :
            LOGD("Sink - Unhandled CL msg[%x]\n", id);
        break ;
    }

}

static bool processEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource)
{
    bool success = TRUE;

    if(stateManagerGetState() == deviceLimbo)
    {
        success = FALSE;
    }
    else
    {
        success = audioSourceSelectHandleEvent(EventUsrSelectAudioSource);

        if(success)
        {
            sinkVolumeResetVolumeAndSourceSaveTimeout();
        }
    }

    return success;
}

static bool eventToBeIndicatedBeforeProcessing(const MessageId id)
{
    if(id == EventUsrMainOutMuteOn)
    {
        return TRUE;
    }
    return FALSE;
}

/*************************************************************************
NAME
    handleUEMessage

DESCRIPTION
    handles messages from the User Events

RETURNS

*/
static void handleUEMessage  ( Task task, MessageId id, Message message )
{
    /* Event state control is done by the config - we will be in the right state for the message
    therefore messages need only be passed to the relative handlers unless configurable */
    sinkState lState = stateManagerGetState() ;

    /*if we do not want the event received to be indicated then set this to FALSE*/
    bool lIndicateEvent = TRUE ;
    bool lResetAutoSwitchOff = FALSE;
    
    /* Reset the auto switch off timer when either BT device disconnects */
    if((id == EventSysPrimaryDeviceDisconnected) || (id == EventSysSecondaryDeviceDisconnected))
    {
        /* postpone auto switch-off */
        lResetAutoSwitchOff = TRUE;       
    }

    /* Deal with user generated Event specific actions*/
    if (id < EVENTS_SYS_MESSAGE_BASE)
    {
        /*cancel any missed call indicator on a user event (button press)*/
        MessageCancelAll(task , EventSysMissedCall ) ;

        /* postpone auto switch-off */
        lResetAutoSwitchOff = TRUE;
            
        /* check for led timeout/re-enable */
        LEDManagerCheckTimeoutState();
            
#ifdef ENABLE_GAIA
        gaiaReportUserEvent(id);
#endif
    }

    if(eventToBeIndicatedBeforeProcessing(id))
    {    
        IndicateEvent(id);
    }

/*    LOGD("UE[%x]\n", id ); */

    /* The configurable Events*/
    switch ( id )
    {
        case (EventUsrDebugKeysToggle):
            LOGD("Toggle Debug Keys\n");
            /* If the device has debug keys enabled then toggle on/off */
            ConnectionSmSecModeConfig(&theSink.task, cl_sm_wae_acl_none, !theSink.debug_keys_enabled, TRUE);
        break;
        case (EventUsrPowerOn):
        case (EventSysPowerOnPanic):
            LOGD("Power On\n" );

            /* if this init occurs and in limbo wait for the display init */
            if (stateManagerGetState() == deviceLimbo)
            {
                displaySetState(TRUE);
                displayShowText(DISPLAYSTR_HELLO,  strlen(DISPLAYSTR_HELLO), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 10);
                displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * theSink.features.DefaultVolume)/sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps);
#ifdef ENABLE_SUBWOOFER
                updateSwatVolume((theSink.features.DefaultVolume * sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps)/VOLUME_NUM_VOICE_STEPS);
#endif

                /* update battery display */
                displayUpdateBatteryLevel(powerManagerIsChargerConnected());
            }
                        
            /* Power on ANC */
            sinkAncHandlePowerOn();

            /*we have received the power on event- we have fully powered on*/
            stateManagerPowerOn();
            
            /* Indicate now "Power On" voice prompt before audio
               plugins kick in, derisking playing audio with low volume.*/
            IndicateEvent(id);
            /* Clear flag since events have been indicated.*/
            lIndicateEvent = FALSE;
            
            /* Don't route audio when in DUT mode */
            if(!checkForDUTModeEntry())
            {
                audioHandleRouting(audio_source_none);
            }
            
            /* Read and configure the volume orientation, LED Disable state, and tts_language */
            configManagerReadSessionData();

            sinkFmSetFmRxOn(FALSE);

            /* set flag to indicate just powered up and may use different led pattern for connectable state
               if configured, flag is cleared on first successful connection */
            theSink.powerup_no_connection = TRUE;

            /* If critical temperature immediately power off */
            if(powerManagerIsVthmCritical())
                MessageSend(&theSink.task, EventUsrPowerOff, 0);

            /* Take an initial battery reading (and power off if critical) */
            powerManagerReadVbat(battery_level_initial_reading);

            if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                MessageSendLater(&theSink.task, EventSysRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));

            if ( theSink.features.DisablePowerOffAfterPowerOn )
            {
                theSink.PowerOffIsEnabled = FALSE ;
                LOGD("DIS[%x]\n" , theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s  );
                MessageSendLater ( &theSink.task , EventSysEnablePowerOff , 0 , D_SEC ( theSink.conf1->timeouts.DisablePowerOffAfterPowerOnTime_s ) ) ;
            }
            else
            {
                theSink.PowerOffIsEnabled = TRUE ;
            }

#ifdef ENABLE_SUBWOOFER
            /* check to see if there is a paired subwoofer device, if not kick off an inquiry scan */
            MessageSend(&theSink.task, EventSysSubwooferCheckPairing, 0);
#endif

            /* kick off a timer to check the PS store fragmentation status */
            if(theSink.conf1->timeouts.DefragCheckTimer_s)
            {
                MessageSendLater(&theSink.task, EventSysCheckDefrag, 0, D_SEC(theSink.conf1->timeouts.DefragCheckTimer_s));
            }

            /* Power on BLE */
            sinkBlePowerOnEvent();
            
            if(theSink.conf2->audio_routing_data.PluginFeatures.manual_source_selection)
            {
                /* set the active routed source to the last used source stored in session data */
                audioSwitchToAudioSource(theSink.rundata->requested_audio_source);
            }


        break ;
        case (EventUsrPowerOff):
            LOGD("PowerOff - En[%c]\n" , ((theSink.PowerOffIsEnabled) ? 'T':'F') );
#ifdef ENABLE_PEER
            if(theSink.PowerOffIsEnabled)
            {
                u16 peerIndex = 0;
                /* If  a TWS peer device is connected, the TWS single device operation is enabled and the power off flag is not set,
                    send the power off command to the peer */
                if(a2dpGetPeerIndex(&peerIndex) &&(theSink.a2dp_link_data->peer_features[peerIndex] & (remote_features_tws_a2dp_sink|remote_features_tws_a2dp_source))
                    && theSink.features.TwsSingleDeviceOperation && !(theSink.a2dp_link_data->local_peer_status[peerIndex] & PEER_STATUS_POWER_OFF) )
                {
                    theSink.a2dp_link_data->local_peer_status[peerIndex] |= PEER_STATUS_POWER_OFF;
                    sinkAvrcpPowerOff();
                    lIndicateEvent = FALSE ;
                    break;
                }
            }
#endif
            /* don't indicate event if already in limbo state */
            if(lState == deviceLimbo) lIndicateEvent = FALSE ;

            /* only power off if timer has expired or battery is low and the charger isn't connected or temperature high */
            if ( theSink.PowerOffIsEnabled || (!powerManagerIsChargerConnected() && powerManagerIsVbatCritical()) || powerManagerIsVthmCritical())
            {
                lResetAutoSwitchOff = FALSE;
                
                /* store current volume levels for non bluetooth volumes */
                configManagerWriteSessionData () ;

                /* Store DSP data */
                configManagerWriteDspData();

                stateManagerEnterLimboState();
                AuthResetConfirmationFlags();

                VolumeUpdateMuteStatusAllOutputs(FALSE);
                VolumeSetHfpMicrophoneGain(hfp_invalid_link, MICROPHONE_MUTE_OFF);

                sinkClearQueueudEvent();

                if(theSink.conf1->timeouts.EncryptionRefreshTimeout_m != 0)
                    MessageCancelAll ( &theSink.task, EventSysRefreshEncryption) ;

                MessageCancelAll (&theSink.task, EventSysCheckDefrag);
                MessageCancelAll (&theSink.task, EventSysDefrag);

                MessageCancelAll (&theSink.task, EventSysPairingFail);
#ifdef ENABLE_PEER
                MessageCancelAll(&theSink.task , EventSysA2DPPeerLinkLossTimeout);
                theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE;
#endif

#ifdef ENABLE_AVRCP
                /* cancel any queued ff or rw requests */
                MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
#endif
#ifdef ENABLE_SPEECH_RECOGNITION
                /* if speech recognition is in tuning mode stop it */
                if(theSink.csr_speech_recognition_tuning_active)
                {
                    speechRecognitionStop();
                    theSink.csr_speech_recognition_tuning_active = FALSE;
                }
#endif
                if(sinkFmIsFmRxOn())
                {
                    MessageSend(&theSink.task, EventUsrFmRxOff, 0);
                }
                /* keep the display on if charging */
                if (powerManagerIsChargerConnected() && (stateManagerGetState() == deviceLimbo) )
                {
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_CHARGING,  strlen(DISPLAYSTR_CHARGING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
                    displayUpdateVolume(0);
                    displayUpdateBatteryLevel(TRUE);
                }
                else
                {
                    displaySetState(FALSE);
                }
                if(theSink.conf2->audio_routing_data.PluginFeatures.manual_source_selection)
                {
                    /* set the active routed source back to none */
                    audioSwitchToAudioSource(audio_source_none);
                }
#ifdef ENABLE_GAIA
                if (!theSink.features.GaiaRemainConnected)
                    gaiaDisconnect();
#endif

                /* Power off BLE */
                sinkBlePowerOffEvent();

                /* Power off ANC */
                sinkAncHandlePowerOff();
            }
            else
            {
                lIndicateEvent = FALSE ;
            }


        break ;
        case (EventUsrInitateVoiceDial):
            LOGD("InitVoiceDial [%d]\n", theSink.VoiceRecognitionIsActive);
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {

                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_primary_link) ;
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */
                    audioHandleRouting(audio_source_none);
                }
                else
                {
                    sinkInitiateVoiceDial (hfp_primary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrInitateVoiceDial_AG2):
            LOGD("InitVoiceDial AG2[%d]\n", theSink.VoiceRecognitionIsActive);
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
            if ( theSink.PowerOffIsEnabled )
            {

                if (theSink.VoiceRecognitionIsActive)
                {
                    sinkCancelVoiceDial(hfp_secondary_link) ;
                    lIndicateEvent = FALSE ;
                    /* replumb any existing audio connections */
                    audioHandleRouting(audio_source_none);
                }
                else
                {
                    sinkInitiateVoiceDial(hfp_secondary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrLastNumberRedial):
            LOGD("LNR\n" );

            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 1 */
                        sinkInitiateLNR(hfp_primary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 1 */
                    sinkInitiateLNR(hfp_primary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrLastNumberRedial_AG2):
            LOGD("LNR AG2\n" );
            if ( theSink.PowerOffIsEnabled )
            {
                if (theSink.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theSink.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 2 */
                        sinkInitiateLNR(hfp_secondary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 2 */
                   sinkInitiateLNR(hfp_secondary_link) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventUsrAnswer):
            LOGD("Answer\n" );
            /* don't indicate event if not in incoming call state as answer event is used
               for some of the multipoint three way calling operations which generate unwanted
               tones */
            if(stateManagerGetState() != deviceIncomingCallEstablish) lIndicateEvent = FALSE ;

            /* Call the HFP lib function, this will determine the AT cmd to send
               depending on whether the profile instance is HSP or HFP compliant. */
            sinkAnswerOrRejectCall( TRUE );
        break ;
        case (EventUsrReject):
            LOGD("Reject\n" );
            /* Reject incoming call - only valid for instances of HFP */
            sinkAnswerOrRejectCall( FALSE );
        break ;
        case (EventUsrCancelEnd):
            LOGD("CancelEnd\n" );
            /* Terminate the current ongoing call process */
            sinkHangUpCall();

        break ;
        case (EventUsrTransferToggle):
            LOGD("Transfer\n" );
            sinkTransferToggle(id);
        break ;
        case EventSysCheckForAudioTransfer :
            LOGD("Check Aud Tx\n");
            sinkCheckForAudioTransfer();
            break ;

        case EventUsrMicrophoneMuteToggle:
        case EventUsrMicrophoneMuteOn:
        case EventUsrMicrophoneMuteOff:
        case EventUsrVolumeOrientationNormal:
        case EventUsrVolumeOrientationInvert:
        case EventUsrVolumeOrientationToggle:
        case EventUsrMainOutVolumeUp:
        case EventUsrMainOutVolumeDown:
        case EventUsrAuxOutVolumeUp:
        case EventUsrAuxOutVolumeDown:
        case EventUsrMainOutMuteOn:
        case EventUsrMainOutMuteOff:
        case EventUsrMainOutMuteToggle:
        case EventUsrAuxOutMuteOn:
        case EventUsrAuxOutMuteOff:
        case EventUsrAuxOutMuteToggle:
        case EventSysVolumeMax:
        case EventSysVolumeMin:
        case EventSysVolumeAndSourceChangeTimer:
            LOGD("Sys/Usr Volume Event\n");
            id = sinkVolumeModifyEventAccordingToVolumeOrientation(id);
            lIndicateEvent = sinkVolumeProcessEventVolume(id);
            break;

        case (EventSysEnterPairingEmptyPDL):
        case (EventUsrEnterPairing):
            LOGD("EnterPair [%d]\n" , lState );

            /*go into pairing mode*/
            if (( lState != deviceLimbo) && (lState != deviceConnDiscoverable ))
            {
                theSink.inquiry.session = inquiry_session_normal;
                stateManagerEnterConnDiscoverableState( TRUE );
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventSysPairingFail):
            /*we have failed to pair in the alloted time - return to the connectable state*/
            LOGD("Pairing Fail\n");
            if (lState != deviceTestMode)
            {
                switch (theSink.features.PowerDownOnDiscoTimeout)
                {
                    case PAIRTIMEOUT_POWER_OFF:
                    {
                        if (!sinkFmIsFmRxOn())
                        {
                            MessageSend ( task , EventUsrPowerOff , 0) ;
                        }
                    }
                        break;
                    case PAIRTIMEOUT_POWER_OFF_IF_NO_PDL:
                        /* Power off if no entries in PDL list */
                        if (ConnectionTrustedDeviceListSize() == 0)
                        {
                            if (!sinkFmIsFmRxOn())
                            {
                                MessageSend ( task , EventUsrPowerOff , 0) ;
                            }
                        }
                        else
                        {
                            /* when not configured to stay disconverable at all times */
                            if(!theSink.features.RemainDiscoverableAtAllTimes)
                            {
                                /* enter connectable mode */
                                stateManagerEnterConnectableState(TRUE);
                            }
#ifdef ENABLE_PEER
                                /* Attempt to establish connection with Peer */
                            peerConnectPeer();
#endif
                        }
                        break;
                    case PAIRTIMEOUT_CONNECTABLE:
                    default:
                        /* when not configured to stay disconverable at all times */
                        if(!theSink.features.RemainDiscoverableAtAllTimes)
                        {
							/* Check if we were connected before*/
                            if(deviceManagerNumConnectedDevs() == 0)
                            {
                            /* enter connectable state */
                            stateManagerEnterConnectableState(TRUE);
                        }
                            else
                            {
                                /* return to connected mode */
                                stateManagerEnterConnectedState();
                            }      
                        }
#ifdef ENABLE_PEER
                            /* Attempt to establish connection with Peer */
                        peerConnectPeer();
#endif
                        break;
                }
            }
            /* have attempted to connect following a power on and failed so clear the power up connection flag */
            theSink.powerup_no_connection = FALSE;

        break ;
        case ( EventSysPairingSuccessful):
            LOGD("Pairing Successful\n");
            if (lState == deviceConnDiscoverable)
            {
                stateManagerEnterConnectableState(FALSE);
            }
        break ;

        case EventUsrEstablishPeerConnection:
            LOGD("Establish peer Connection\n");
#ifdef ENABLE_PEER
                /* Attempt to establish connection with Peer */
                peerConnectPeer();
#endif
        break ;

        case ( EventUsrConfirmationAccept ):
            LOGD("Pairing Correct Res\n" );
            sinkPairingAcceptRes();
        break;
        case ( EventUsrConfirmationReject ):
            LOGD("Pairing Reject Res\n" );
            sinkPairingRejectRes();
        break;
        case ( EventUsrEstablishSLC ) :
                /* Make sure we're not using the Panic action */
                theSink.panic_reconnect = FALSE;
                /* Fall through */
        case ( EventSysEstablishSLCOnPanic ):

#ifdef ENABLE_SUBWOOFER
            /* if performing a subwoofer inquiry scan then cancel the SLC connect request
               this will resume after the inquiry scan has completed */
            if(theSink.inquiry.action == rssi_subwoofer)
            {
                lIndicateEvent = FALSE;
                break;
            }
#endif
            /* check we are not already connecting before starting */
            {
                LOGD("EventUsrEstablishSLC\n");

                slcEstablishSLCRequest() ;

                /* don't indicate the event at first power up if the use different event at power on
                   feature bit is enabled, this enables the establish slc event to be used for the second manual
                   connection request */
                if(stateManagerGetState() == deviceConnectable)
                {
                    /* send message to do indicate a start of paging process when in connectable state */
                    MessageSend(&theSink.task, EventSysStartPagingInConnState ,0);
                }
            }
        break ;
        case ( EventUsrRssiPair ):
            LOGD("RSSI Pair\n");
            lIndicateEvent = inquiryPair( inquiry_session_normal, TRUE );
        break;
        case ( EventSysRssiResume ):
            LOGD("RSSI Resume\n");
            inquiryResume();
        break;
        case ( EventSysRssiPairReminder ):
            LOGD("RSSI Pair Reminder\n");
            if (stateManagerGetState() != deviceLimbo )
                MessageSendLater(&theSink.task, EventSysRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
            else
                lIndicateEvent = FALSE;

        break;
        case ( EventSysRssiPairTimeout ):
            LOGD("RSSI Pair Timeout\n");
            inquiryTimeout();
        break;
        case ( EventSysRefreshEncryption ):
            LOGD("Refresh Encryption\n");
            {
                u8 k;
                Sink sink;
                Sink audioSink;
                /* For each profile */
                for(k=0;k<MAX_PROFILES;k++)
                {
                    LOGD("Profile %d: ",k);
                    /* If profile is connected */
                    if((HfpLinkGetSlcSink((k + 1), &sink))&&(sink))
                    {
                        /* If profile has no valid SCO sink associated with it */
                        HfpLinkGetAudioSink((k + hfp_primary_link), &audioSink);
                        if(!SinkIsValid(audioSink))
                        {
                            LOGD("Key Refreshed\n");
                            /* Refresh the encryption key */
                            ConnectionSmEncryptionKeyRefreshSink(sink);
                        }
#ifdef DEBUG_MAIN
                        else
                        {
                            LOGD("Key Not Refreshed, SCO Active\n");
                        }
                    }
                    else
                    {
                        LOGD("Key Not Refreshed, SLC Not Active\n");
#endif
                    }
                }
                MessageSendLater(&theSink.task, EventSysRefreshEncryption, 0, D_MIN(theSink.conf1->timeouts.EncryptionRefreshTimeout_m));
            }
        break;

        /* 60 second timer has triggered to disable connectable mode in multipoint
            connection mode */
        case ( EventSysConnectableTimeout ) :
#ifdef ENABLE_SUBWOOFER
            if(!SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
            {
                LOGD("SM: disable Connectable Cancelled due to lack of subwoofer\n" );
                break;
            }
#endif
#ifdef ENABLE_PARTYMODE
            /* leave headset connectable when using party mode */
            if(!(theSink.PartyModeEnabled))
#endif
            {
                /* only disable connectable mode if at least one hfp instance is connected */
                if(deviceManagerNumConnectedDevs())
                {
                    LOGD("SM: disable Connectable \n" );
                    /* disable connectability */
                    sinkDisableConnectable();
                }
            }
        break;

        case ( EventSysLEDEventComplete ) :
            /*the message is a ptr to the event we have completed*/
            LOGD("LEDEvCmp[%x]\n" ,  (( LMEndMessage_t *)message)->Event  );

            switch ( (( LMEndMessage_t *)message)->Event )
            {
                case (EventUsrResetPairedDeviceList) :
                {      /*then the reset has been completed*/
                    MessageSend(&theSink.task , EventSysResetComplete , 0 ) ;

                        /*power cycle if required*/
                    if ((theSink.features.PowerOffAfterPDLReset )&&
                        (stateManagerGetState() > deviceLimbo ))
                    {
                        LOGD("Reboot After Reset\n");
                        if (!sinkFmIsFmRxOn())
                        {
                            MessageSend ( &theSink.task , EventUsrPowerOff , 0 ) ;
                        }
                    }
                }
                break ;

                case EventUsrPowerOff:
                {
                    /* Determine if a reset is required because the PS needs defragmentation */
                    configManagerDefragIfRequired();

                    /*allows a reset of the device for those designs which keep the chip permanently powered on*/
                    if (theSink.features.ResetAfterPowerOffComplete )
                    {
                        LOGD("Reset\n");
                        /* Reboot always - set the same boot mode; this triggers the target to reboot.*/
                        BootSetMode(BootGetMode());
                    }

                    if(powerManagerIsVthmCritical())
                        stateManagerUpdateLimboState();
                }
                break ;

                default:
                break ;
            }

            if (theSink.features.QueueLEDEvents )
            {
                    /*if there is a queueud event*/
                if (LedManagerQueuedEvent())
                {
                    LOGD("Play Q'd Ev [%x]\n", LedManagerQueuedEvent());
                    LedManagerIndicateQueuedEvent();
                }
                else
                {
                    /* restart state indication */
                    LEDManagerIndicateState ( stateManagerGetState () ) ;
                }
            }
            else
                LEDManagerIndicateState ( stateManagerGetState () ) ;

        break ;
        case (EventSysAutoSwitchOff):
        {
            LOGD("Auto S Off[%d] sec elapsed\n" , theSink.conf1->timeouts.AutoSwitchOffTime_s );
            
            if ( (!gattClientHasNoClients()) || (!gattServerIsFullyDisconnected()) )
            {
                LOGD("Auto Switch Off cancelled due to active BLE connection \n");
                break; 			
            }

            switch ( lState )
            {
                case deviceLimbo:
                case deviceConnectable:
                case deviceConnDiscoverable:
                    if(IS_SOURCE_CONNECTED)
                    {
                        MessageSend (task, EventUsrPowerOff , 0);
                    }
                    else
                    {
                        lResetAutoSwitchOff = TRUE;
                    }
                    break;
                case deviceConnected:
                    if(deviceManagerNumConnectedDevs() == deviceManagerNumConnectedPeerDevs())
                    {   /* Only connected to peer devices, so allow auto-switchoff to occur unless 
                            any source is plugged in. */
                        if(IS_SOURCE_CONNECTED)
#ifdef ENABLE_PEER
                        /* Before Powering off make sure that there is no Audio source connected to the remote peer*/
                        if(theSink.remote_peer_audio_conn_status == 0)
#endif
                        {
                            MessageSend (task, EventUsrPowerOff , 0);
                            break;
                        }
                    }
                    lResetAutoSwitchOff = TRUE;
                    break;
                case deviceOutgoingCallEstablish:
                case deviceIncomingCallEstablish:
                case deviceActiveCallSCO:
                case deviceActiveCallNoSCO:
                case deviceTestMode:
                    break ;
                default:
                    LOGD("UE ?s [%d]\n", lState);
                    break ;
            }
        }
        break;
        case (EventUsrChargerConnected):
        {
            LOGD("Charger Connected\n");
            powerManagerChargerConnected();
            if ( lState == deviceLimbo )
            {
                stateManagerUpdateLimboState();
            }

            /* indicate battery charging on the display */
            displayUpdateBatteryLevel(TRUE);
        }
        break;
        case (EventUsrChargerDisconnected):
        {
            LOGD("Charger Disconnected\n");
            powerManagerChargerDisconnected();

            /* if in limbo state, schedule a power off event */
            if (lState == deviceLimbo )
            {
                /* cancel existing limbo timeout and rescheduled another limbo timeout */
                sinkSendLater(EventSysLimboTimeout, D_SEC(theSink.conf1->timeouts.LimboTimeout_s));

                /* turn off the display if in limbo and no longer charging */
                displaySetState(FALSE);
            }
            else
            {
                /* update battery display */
                displayUpdateBatteryLevel(FALSE);
            }
        }
        break;
        case (EventUsrResetPairedDeviceList):
            {
			/* NOTE: For Soundbar application,
			 * devices considered as "protected" (i.e BLE
			 * remote or subwoofer) will not be deleted in this
			 * event.*/

                LOGD("--Reset PDL--");
                if ( stateManagerIsConnected () )
                {
                   /*disconnect any connected HFP profiles*/
                   sinkDisconnectAllSlc();
                   /*disconnect any connected A2DP/AVRCP profiles*/
                   disconnectAllA2dpAvrcp(TRUE);
                }

                deviceManagerRemoveAllDevices();

#ifdef ENABLE_PEER
                /* Ensure permanently paired Peer device is placed back into PDL */
                AuthInitPermanentPairing();
#endif

                if(INQUIRY_ON_PDL_RESET)
                    MessageSend(&theSink.task, EventUsrRssiPair, 0);
            }
        break ;
        case ( EventSysLimboTimeout ):
            {
                /*we have received a power on timeout - shutdown*/
                LOGD("EvLimbo TIMEOUT\n");
                if (lState != deviceTestMode)
                {
                    stateManagerUpdateLimboState();
                }
            }
        break ;
        case EventSysSLCDisconnected:
                LOGD("EvSLCDisconnect\n");
            {
                theSink.VoiceRecognitionIsActive = FALSE ;
                MessageCancelAll ( &theSink.task , EventSysNetworkOrServiceNotPresent ) ;
            }
        break ;
        case (EventSysLinkLoss):
            LOGD("Link Loss\n");
            {
                conn_mask mask;

                /* should the device have been powered off prior to a linkloss event being
                   generated, this can happen if a link loss has occurred within 5 seconds
                   of power off, ensure the device does not attempt to reconnet from limbo mode */
                if(stateManagerGetState()== deviceLimbo)
                    lIndicateEvent = FALSE;

                /* Only get the profiles for the device which has a link loss.
                   If it is not specified fallback to all connected profiles. */
                if (theSink.linkloss_bd_addr && !BdaddrIsZero(theSink.linkloss_bd_addr))
                {
                    mask = deviceManagerProfilesConnected(theSink.linkloss_bd_addr);
                }
                else
                {
                    mask = deviceManagerGetProfilesConnected();
                }

                LOGD("mask 0x%x linkLossReminderTime %u protection %u routed_audio %x\n", 
                     mask, theSink.linkLossReminderTime,
                     theSink.stream_protection_state == linkloss_stream_protection_on,
                     (uintptr_t)theSink.routed_audio);

                /* The hfp library will generate repeat link loss events but
                   the a2dp library doesn't, so only repeat it here if:
                   a2dp only is in use, peer link loss stream protection is
                   not on (or there is no routed audio), or the link loss interval is > 0. */
                if (((!theSink.hfp_profiles) || (!(mask & conn_hfp) && (mask & conn_a2dp)))
                    && (theSink.linkloss_bd_addr 
                        && !(linklossIsStreamProtected(theSink.linkloss_bd_addr) && (theSink.routed_audio != 0)))
                    && (theSink.linkLossReminderTime != 0))
                {
                    linklossSendLinkLossTone(theSink.linkloss_bd_addr, D_SEC(theSink.linkLossReminderTime));
                }
            }
        break ;
        case (EventSysMuteReminder) :
            LOGD("Mute Remind\n");
            /*arrange the next mute reminder tone*/
            MessageSendLater( &theSink.task , EventSysMuteReminder , 0 ,D_SEC(theSink.conf1->timeouts.MuteRemindTime_s ) )  ;

            /* only play the mute reminder tone if AG currently having its audio routed is in mute state */
            if(!VolumePlayMuteToneQuery())
                lIndicateEvent = FALSE;
        break;

        case EventUsrBatteryLevelRequest:
          LOGD("EventUsrBatteryLevelRequest\n");
          powerManagerReadVbat(battery_level_user);
        break;

        case EventSysBatteryCritical:
            LOGD("EventSysBatteryCritical\n");
        break;

        case EventSysBatteryLow:
            LOGD("EventSysBatteryLow\n");
        break;

        case EventSysGasGauge0 :
        case EventSysGasGauge1 :
        case EventSysGasGauge2 :
        case EventSysGasGauge3 :
            LOGD("EventSysGasGauge%d\n", id - EventSysGasGauge0);
        break ;

        case EventSysBatteryOk:
            LOGD("EventSysBatteryOk\n");
        break;

        case EventSysChargeInProgress:
            LOGD("EventSysChargeInProgress\n");
        break;

        case EventSysChargeComplete:
            LOGD("EventSysChargeComplete\n");
        break;

        case EventSysChargeDisabled:
            LOGD("EventSysChargeDisabled\n");
        break;

        case EventUsrEnterDUTState :
        {
            LOGD("EnterDUTState \n");
            stateManagerEnterTestModeState();
        }
        break;
        case EventUsrEnterDutMode :
        {
            LOGD("Enter DUT Mode \n");
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventUsrEnterDUTState, 0 ) ;
            }
            enterDutMode () ;
        }
        break;
        case EventUsrEnterTXContTestMode :
        {
            LOGD("Enter TX Cont \n");
            if (lState !=deviceTestMode)
            {
                MessageSend( task , EventUsrEnterDUTState , 0 ) ;
            }
            enterTxContinuousTestMode() ;
        }
        break ;

        case EventSysNetworkOrServiceNotPresent:
            {       /*only bother to repeat this indication if it is not 0*/
                if ( theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s )
                {       /*make sure only ever one in the system*/
                    sinkSendLater(EventSysNetworkOrServiceNotPresent,
                                        D_SEC(theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s) ) ;
                }
                LOGD("NO NETWORK [%d]\n", theSink.conf1->timeouts.NetworkServiceIndicatorRepeatTime_s );
            }
        break ;
        case EventSysNetworkOrServicePresent:
            {
                MessageCancelAll ( task , EventSysNetworkOrServiceNotPresent ) ;
                LOGD("YES NETWORK\n");
            }
        break ;
        case EventUsrLedsOnOffToggle  :
            LOGD("Toggle EN_DIS LEDS ");
            LOGD("Tog Was[%c]\n" , theSink.theLEDTask->gLEDSEnabled ? 'T' : 'F');

            LedManagerToggleLEDS();
            LOGD("Tog Now[%c]\n" , theSink.theLEDTask->gLEDSEnabled ? 'T' : 'F');

            break ;
        case EventUsrLedsOn:
            LOGD("Enable LEDS\n");
            LedManagerEnableLEDS ( ) ;
                /* also include the led disable state as well as orientation, write this to the PSKEY*/
            configManagerWriteSessionData ( ) ;
#ifdef ENABLE_PEER                
            sinkAvrcpUpdateLedIndicationOnOff(TRUE);
#endif
            break ;
        case EventUsrLedsOff:
            LOGD("Disable LEDS\n");
            LedManagerDisableLEDS ( ) ;

                /* also include the led disable state as well as orientation, write this to the PSKEY*/
            configManagerWriteSessionData ( ) ;
#ifdef ENABLE_PEER                
            sinkAvrcpUpdateLedIndicationOnOff(FALSE);
#endif
            break ;
        case EventSysCancelLedIndication:
            LOGD("Disable LED indication\n");
            LedManagerResetLEDIndications ( ) ;
            break ;
        case EventSysCallAnswered:
            LOGD("EventSysCallAnswered\n");
        break;
        
        case EventSysSLCConnected:
        case EventSysSLCConnectedAfterPowerOn:
            
            LOGD("EventSysSLCConnected\n");
            /*if there is a queued event - we might want to know*/
            sinkRecallQueuedEvent();
            
            /* postpone auto switch-off */
            lResetAutoSwitchOff = TRUE;
        break;
        
        case EventSysPrimaryDeviceConnected:
        case EventSysSecondaryDeviceConnected:
            /*used for indication purposes only*/
            LOGD("Device Connected [%c]\n " , (id - EventSysPrimaryDeviceConnected)? 'S' : 'P'  ); 
            break;
        case EventSysPrimaryDeviceDisconnected:
            /*used for indication purposes only*/
            LOGD("Device Disconnected [%c]\n " , (id - EventSysPrimaryDeviceDisconnected)? 'S' : 'P'  );
            break;
        case EventSysSecondaryDeviceDisconnected:
            /*used for indication purposes only*/  
            LOGD("Device Disconnected [%c]\n " , (id - EventSysPrimaryDeviceDisconnected)? 'S' : 'P'  );
        break;       
        case EventSysVLongTimer:
        case EventSysLongTimer:
           if (lState == deviceLimbo)
           {
               lIndicateEvent = FALSE ;
           }
        break ;
            /*these events have no required action directly associated with them  */
             /*they are received here so that LED patterns and Tones can be assigned*/
        case EventSysSCOLinkOpen :
            LOGD("EventSysSCOLinkOpen\n");
        break ;
        case EventSysSCOLinkClose:
            LOGD("EventSysSCOLinkClose\n");
        break ;
        case EventSysEndOfCall :
            LOGD("EventSysEndOfCall\n");
#ifdef ENABLE_DISPLAY
            displayShowSimpleText(DISPLAYSTR_CLEAR,1);
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
#endif
        break;
        case EventSysResetComplete:
            LOGD("EventSysResetComplete\n");
        break ;
        case EventSysError:
            LOGD("EventSysError\n");
        break;
        case EventSysReconnectFailed:
            LOGD("EventSysReconnectFailed\n");
        break;

#ifdef THREE_WAY_CALLING
        case EventUsrThreeWayReleaseAllHeld:
            LOGD("RELEASE ALL\n");
            /* release the held call */
            MpReleaseAllHeld();
        break;
        case EventUsrThreeWayAcceptWaitingReleaseActive:
            LOGD("ACCEPT & RELEASE\n");
            MpAcceptWaitingReleaseActive();
        break ;
        case EventUsrThreeWayAcceptWaitingHoldActive  :
            LOGD("ACCEPT & HOLD\n");
            /* three way calling not available in multipoint usage */
            MpAcceptWaitingHoldActive();
        break ;
        case EventUsrThreeWayAddHeldTo3Way  :
            LOGD("ADD HELD to 3WAY\n");
            /* check to see if a conference call can be created, more than one call must be on the same AG */
            MpHandleConferenceCall(TRUE);
        break ;
        case EventUsrThreeWayConnect2Disconnect:
            LOGD("EXPLICIT TRANSFER\n");
            /* check to see if a conference call can be created, more than one call must be on the same AG */
            MpHandleConferenceCall(FALSE);
        break ;
#endif
        case (EventSysEnablePowerOff):
        {
            LOGD("EventSysEnablePowerOff \n");
            theSink.PowerOffIsEnabled = TRUE ;
        }
        break;
        case EventUsrPlaceIncomingCallOnHold:
            sinkPlaceIncomingCallOnHold();
        break ;

        case EventUsrAcceptHeldIncomingCall:
            sinkAcceptHeldIncomingCall();
        break ;
        case EventUsrRejectHeldIncomingCall:
            sinkRejectHeldIncomingCall();
        break;

        case EventUsrEnterDFUMode:
        {
            LOGD("EventUsrEnterDFUMode\n");
            BootSetMode(BOOTMODE_DFU);
        }
        break;

        case EventUsrEnterServiceMode:
        {
            LOGD("Enter Service Mode \n");

            enterServiceMode();
        }
        break ;
        case EventSysServiceModeEntered:
        {
            LOGD("Service Mode!!!\n");
        }
        break;

        case EventSysAudioMessage1:
        case EventSysAudioMessage2:
        case EventSysAudioMessage3:
        case EventSysAudioMessage4:
        {
            if (theSink.routed_audio)
            {
                u16 * lParam = mallocPanic( sizeof(u16)) ;
                *lParam = (id -  EventSysAudioMessage1) ; /*0,1,2,3*/
                if(!AudioSetMode ( AUDIO_MODE_CONNECTED , (void *) lParam) )
                    freePanic(lParam);
            }
        }
        break ;

        case EventUsrUpdateStoredNumber:
            sinkUpdateStoredNumber();
        break;

        case EventUsrDialStoredNumber:
            LOGD("EventUsrDialStoredNumber\n");
            sinkDialStoredNumber();

        break;
        case EventUsrRestoreDefaults:
            LOGD("EventUsrRestoreDefaults\n");
            configManagerRestoreDefaults();

        break;

        case EventSysTone1:
        case EventSysTone2:
            LOGD("EventTone[%d]\n" , (id - EventSysTone1 + 1) );
        break;

        case EventUsrSelectAudioPromptLanguageMode:
            if(theSink.audio_prompts_enabled)
            {
                LOGD("EventUsrSelectAudioPromptLanguageMode");
                AudioPromptSelectLanguage();
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break;

        case EventSysStoreAudioPromptLanguage:
            if(theSink.audio_prompts_enabled)
            {
                /* Store Prompt language in PS */
                configManagerWriteSessionData () ;
            }
        break;

        
        
        /* disabled leds have been re-enabled by means of a button press or a specific event */
        case EventSysResetLEDTimeout:
            LOGD("EventSysResetLEDTimeout\n");
            LEDManagerIndicateState ( lState ) ;
            theSink.theLEDTask->gLEDSStateTimeout = FALSE ;
        break;
        /* starting paging whilst in connectable state */
        case EventSysStartPagingInConnState:
            LOGD("EventSysStartPagingInConnState\n");
            /* set bit to indicate paging status */
            theSink.paging_in_progress = TRUE;
        break;

        /* paging stopped whilst in connectable state */
        case EventSysStopPagingInConnState:
            LOGD("EventSysStartPagingInConnState\n");
            /* set bit to indicate paging status */
            theSink.paging_in_progress = FALSE;
        break;

        /* continue the slc connection procedure, will attempt connection
           to next available device */
        case EventSysContinueSlcConnectRequest:
            /* don't continue connecting if in pairing mode */
            if(stateManagerGetState() != deviceConnDiscoverable)
            {
                LOGD("EventSysContinueSlcConnectRequest\n");
                /* attempt next connection */
                slcContinueEstablishSLCRequest();
            }
        break;

        /* indication of call waiting when using two AG's in multipoint mode */
        case EventSysMultipointCallWaiting:
            LOGD("EventSysMultipointCallWaiting\n");
        break;

        /* kick off a check the role of the device and make changes if appropriate by requesting a role indication */
        case EventSysCheckRole:
            linkPolicyCheckRoles();
        break;

        case EventSysMissedCall:
        {
            if(theSink.conf1->timeouts.MissedCallIndicateTime_s != 0)
            {
                MessageCancelAll(task , EventSysMissedCall ) ;

                theSink.MissedCallIndicated -= 1;
                if(theSink.MissedCallIndicated != 0)
                {
                    MessageSendLater( &theSink.task , EventSysMissedCall , 0 , D_SEC(theSink.conf1->timeouts.MissedCallIndicateTime_s) ) ;
                }
            }
        }
        break;

#ifdef ENABLE_PBAP
        case EventUsrPbapDialMch:
        {
            /* pbap dial from missed call history */
            LOGD("EventUsrPbapDialMch\n");

            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   &&
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {
                    pbapDialPhoneBook(pbap_mch);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;

        case EventUsrPbapDialIch:
        {
            /* pbap dial from incoming call history */
            LOGD("EventUsrPbapDialIch\n");

            if ( theSink.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theSink.features.LNRCancelsVoiceDialIfActive   &&
                    theSink.VoiceRecognitionIsActive)
                {
                    MessageSend(&theSink.task , EventUsrInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {
                    pbapDialPhoneBook(pbap_ich);
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;

        case EventSysEstablishPbap:
        {
            LOGD("EventSysEstablishPbap\n");

            /* Connect to the primary and secondary hfp link devices */
            theSink.pbapc_data.pbap_command = pbapc_action_idle;

            pbapConnect( hfp_primary_link );
            pbapConnect( hfp_secondary_link );
        }
        break;

        case EventUsrPbapSetPhonebook:
        {
            LOGD("EventUsrPbapSetPhonebook, active pb is [%d]\n", theSink.pbapc_data.pbap_active_pb);

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_command = pbapc_setting_phonebook;

            if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
            {
                pbapConnect( hfp_primary_link );
            }
            else
            {
                /* Set the link to active state */
                linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseEntry:
        {
            LOGD("EventUsrPbapBrowseEntry\n");

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                /* If Pbap profile does not connected, connect it first */
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                    theSink.pbapc_data.pbap_browsing_start_flag = 1;
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    if(theSink.pbapc_data.pbap_browsing_start_flag == 0)
                    {
                        theSink.pbapc_data.pbap_browsing_start_flag = 1;
                        PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                    }
                    else
                    {
                        MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_ENTRY , 0 ) ;
                    }
                }

                theSink.pbapc_data.pbap_command = pbapc_browsing_entry;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseList:
        /* EventUsrPbapBrowseListByName event is added for PTS qualification */
        case EventUsrPbapBrowseListByName:
        {
            LOGD("EventUsrPbapBrowseList%s\n",(id == EventUsrPbapBrowseListByName) ? "ByName" : "" );

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
                }

                theSink.pbapc_data.pbap_command = pbapc_browsing_list;

                if (id == EventUsrPbapBrowseListByName)
                {
                    theSink.pbapc_data.pbap_srch_attr = pbap_search_name;
                    /* for PTS qualification, search string expected is "PTS" */
                    theSink.pbapc_data.pbap_srch_val = (const u8*) "PTS";
                }
                else
                {
                    theSink.pbapc_data.pbap_srch_attr = pbap_search_number;
                    theSink.pbapc_data.pbap_srch_val = NULL;
                }
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapDownloadPhonebook:
        {
            LOGD("EventUsrPbapDownloadPhonebook\n");

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PULL_PHONE_BOOK , 0 ) ;
                }

                theSink.pbapc_data.pbap_command = pbapc_downloading;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapGetPhonebookSize:
        {
            LOGD("EventUsrPbapGetPhonebookSize");

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                if(theSink.pbapc_data.pbap_active_link == pbapc_invalid_link)
                {
                    pbapConnect( hfp_primary_link );
                }
                else
                {
                    /* Set the link to active state */
                    linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

                    MessageSend(&theSink.task , PBAPC_APP_PHONE_BOOK_SIZE , 0 ) ;
                }

                theSink.pbapc_data.pbap_command = pbapc_phonebooksize;
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapSelectPhonebookObject:
        {
            LOGD("EventUsrPbapSelectPhonebookObject\n");

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;

            if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
            {
                theSink.pbapc_data.pbap_active_pb += 1;

                if(theSink.pbapc_data.pbap_active_pb > pbap_cch)
                {
                    theSink.pbapc_data.pbap_active_pb = pbap_pb;
                }
            }

            lIndicateEvent = FALSE ;
        }
        break;

        case EventUsrPbapBrowseComplete:
        {
            LOGD("EventUsrPbapBrowseComplete\n");

            /* Set the link policy based on the HFP or A2DP state */
            linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

            theSink.pbapc_data.PbapBrowseEntryIndex = 0;
            theSink.pbapc_data.pbap_browsing_start_flag = 0;
            lIndicateEvent = FALSE ;

        }
        break;


#endif

#ifdef WBS_TEST
        /* TEST EVENTS for WBS testing */
        case EventUsrWbsTestSetCodecs:
            if(theSink.RenegotiateSco)
            {
                LOGD("AT+BAC = cvsd wbs\n");
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), FALSE);
            }
            else
            {
                LOGD("AT+BAC = cvsd only\n");
                theSink.RenegotiateSco = 1;
                HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , FALSE);
            }

        break;

        case EventUsrWbsTestSetCodecsSendBAC:
            if(theSink.RenegotiateSco)
            {
                LOGD("AT+BAC = cvsd wbs\n");
                theSink.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), TRUE);
            }
           else
           {
               LOGD("AT+BAC = cvsd only\n");
               theSink.RenegotiateSco = 1;
               HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , TRUE);
           }
           break;

         case EventUsrWbsTestOverrideResponse:

           if(theSink.FailAudioNegotiation)
           {
               LOGD("Fail Neg = off\n");
               theSink.FailAudioNegotiation = 0;
           }
           else
           {
               LOGD("Fail Neg = on\n");
               theSink.FailAudioNegotiation = 1;
           }
       break;

#endif

       case EventUsrCreateAudioConnection:
           LOGD("Create Audio Connection\n");

           CreateAudioConnection();
       break;

#ifdef ENABLE_MAPC
        case EventSysMapcMsgNotification:
            /* Generate a tone or audio prompt */
            LOGD("EventSysMapcMsgNotification\n");
		break;
        case EventSysMapcMnsSuccess:
            /* Generate a tone to indicate the mns service success */
            LOGD("EventSysMapcMnsSuccess\n");
        break;
        case EventSysMapcMnsFailed:
            /* Generate a tone to indicate the mns service failed */
            LOGD("EventSysMapcMnsFailed\n");
        break;
#endif

       case EventUsrIntelligentPowerManagementOn:
           LOGD("Enable LBIPM\n");
            /* enable LBIPM operation */
           theSink.lbipmEnable = 1;
           /* send plugin current power level */
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ;
       break;

       case EventUsrIntelligentPowerManagementOff:
           LOGD("Disable LBIPM\n");
            /* disable LBIPM operation */
           theSink.lbipmEnable = 0;
           /* notify the plugin Low power mode is no longer required */
           AudioSetPower(powerManagerGetLBIPM());
            /* and store in PS for reading at next power up */
           configManagerWriteSessionData () ;
       break;

       case EventUsrIntelligentPowerManagementToggle:
           LOGD("Toggle LBIPM\n");
           if(theSink.lbipmEnable)
           {
               MessageSend( &theSink.task , EventUsrIntelligentPowerManagementOff , 0 ) ;
           }
           else
           {
               MessageSend( &theSink.task , EventUsrIntelligentPowerManagementOn , 0 ) ;
           }

       break;

        case EventUsrUsbPlayPause:
        case EventUsrUsbStop:
        case EventUsrUsbFwd:
        case EventUsrUsbBack:
        case EventUsrUsbMute:
        case EventUsrUsbLowPowerMode:
        case EventSysUsbDeadBatteryTimeout:
        case EventSysAllowUSBVolEvents:
            lIndicateEvent = sinkUsbProcessEventUsb(id);
            break;

        case EventUsrAnalogAudioConnected:
            /* Start the timer here to turn off the device if this feature is enabled by the user*/
            if(theSink.features.PowerOffOnWiredAudioConnected)
            {
                /* cancel existing limbo timeout and reschedule another limbo timeout */
                sinkSendLater(EventSysLimboTimeout, D_SEC(theSink.conf1->timeouts.WiredAudioConnectedPowerOffTimeout_s));
            }
            else
            {
#ifdef ENABLE_PEER
                /*If the Analog Audio has connected then notify this to the peer device */
                sinkAvrcpUpdatePeerWiredSourceConnected(ANALOG_AUDIO);
                peerClaimRelay(TRUE);
                PEER_UPDATE_REQUIRED_RELAY_STATE("ANALOG AUDIO CONNECTED");
#endif
                /* Update audio routing */
                audioHandleRouting(audio_source_none);
            }
            break;

        case EventUsrSpdifAudioConnected:
            /* Update audio routing */
            audioHandleRouting(audio_source_none);
        break;
        case EventUsrAnalogAudioDisconnected:
#ifdef ENABLE_PEER
            {
                /*If the Analog Audio has disconnected then notify this to the peer device */
                sinkAvrcpUpdatePeerSourceDisconnected(ANALOG_AUDIO);
                peerClaimRelay(FALSE);
                PEER_UPDATE_REQUIRED_RELAY_STATE("ANALOG AUDIO DISCONNECTED");
            }
#endif
            /* Update audio routing */
            audioHandleRouting(audio_source_none);
        break;

        case EventUsrSpdifAudioDisconnected:
            /* Update audio routing */
            audioHandleRouting(audio_source_none);
        break;


#ifdef ENABLE_AVRCP

       case EventUsrAvrcpPlayPause:
            LOGD("EventUsrAvrcpPlayPause\n");
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPlayPause();
       break;

      case EventUsrAvrcpPlay:
            LOGD("EventUsrAvrcpPlay\n");
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPlay();
       break;

       case EventUsrAvrcpPause:
            LOGD("EventUsrAvrcpPause\n");
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpPause();
       break;

       case EventUsrAvrcpStop:
            LOGD("EventUsrAvrcpStop\n");
            /* cancel any queued ff or rw requests */
            MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
            MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
            sinkAvrcpStop();
       break;

       case EventUsrAvrcpSkipForward:
           LOGD("EventUsrAvrcpSkipForward\n");
           sinkAvrcpSkipForward();
       break;

       case EventUsrEnterBootMode2:
            LOGD("Reboot into different bootmode [2]\n");
           BootSetMode(BOOTMODE_CUSTOM) ;
       break ;

       case EventUsrAvrcpSkipBackward:
           LOGD("EventUsrAvrcpSkipBackward\n");
           sinkAvrcpSkipBackward();
       break;

       case EventUsrAvrcpFastForwardPress:
           LOGD("EventUsrAvrcpFastForwardPress\n");
           sinkAvrcpFastForwardPress();
           /* rescehdule a repeat of this message every 1.5 seconds */
           MessageSendLater( &theSink.task , EventUsrAvrcpFastForwardPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
       break;

       case EventUsrAvrcpFastForwardRelease:
           LOGD("EventUsrAvrcpFastForwardRelease\n");
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
           sinkAvrcpFastForwardRelease();
       break;

       case EventUsrAvrcpRewindPress:
           LOGD("EventUsrAvrcpRewindPress\n");
           /* rescehdule a repeat of this message every 1.8 seconds */
           MessageSendLater( &theSink.task , EventUsrAvrcpRewindPress , 0 , AVRCP_FF_REW_REPEAT_INTERVAL) ;
           sinkAvrcpRewindPress();
       break;

       case EventUsrAvrcpRewindRelease:
           LOGD("EventUsrAvrcpRewindRelease\n");
           /* cancel any queued FF repeat requests */
           MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
           sinkAvrcpRewindRelease();
       break;

       case EventUsrAvrcpToggleActive:
           LOGD("EventUsrAvrcpToggleActive\n");
           if (sinkAvrcpGetNumberConnections() > 1)
               sinkAvrcpToggleActiveConnection();
           else
               lIndicateEvent = FALSE;
       break;

       case EventUsrAvrcpNextGroupPress:
           LOGD("EventUsrAvrcpNextGroupPress\n");
           sinkAvrcpNextGroupPress();
       break;

        case EventUsrAvrcpNextGroupRelease:
           LOGD("EventUsrAvrcpNextGroupRelease\n");
           sinkAvrcpNextGroupRelease();
       break;

       case EventUsrAvrcpPreviousGroupPress:
           LOGD("EventUsrAvrcpPreviousGroupPress\n");
           sinkAvrcpPreviousGroupPress();
       break;

       case EventUsrAvrcpPreviousGroupRelease:
           LOGD("EventUsrAvrcpPreviousGroupRelease\n");
           sinkAvrcpPreviousGroupRelease();
       break;

       case EventUsrAvrcpShuffleOff:
           LOGD("EventUsrAvrcpShuffleOff\n");
           sinkAvrcpShuffle(AVRCP_SHUFFLE_OFF);
        break;

       case EventUsrAvrcpShuffleAllTrack:
           LOGD("EventUsrAvrcpShuffleAllTrack\n");
           sinkAvrcpShuffle(AVRCP_SHUFFLE_ALL_TRACK);
        break;

       case EventUsrAvrcpShuffleGroup:
           LOGD("EventUsrAvrcpShuffleGroup\n");
           sinkAvrcpShuffle(AVRCP_SHUFFLE_GROUP);
        break;

       case EventUsrAvrcpRepeatOff:
           LOGD("EventUsrAvrcpRepeatOff\n");
           sinkAvrcpRepeat(AVRCP_REPEAT_OFF);
        break;

       case EventUsrAvrcpRepeatSingleTrack:
           LOGD("EventUsrAvrcpRepeatSingleTrack\n");
           sinkAvrcpRepeat(AVRCP_REPEAT_SINGLE_TRACK);
        break;

       case EventUsrAvrcpRepeatAllTrack:
           LOGD("EventUsrAvrcpRepeatAllTrack\n");
           sinkAvrcpRepeat(AVRCP_REPEAT_ALL_TRACK);
        break;

       case EventUsrAvrcpRepeatGroup:
           LOGD("EventUsrAvrcpRepeatGroup\n");
           sinkAvrcpRepeat(AVRCP_REPEAT_GROUP);
        break;
        case EventSysSetActiveAvrcpConnection:
        {
            sinkAvrcpSetActiveConnectionFromBdaddr(&((UpdateAvrpcMessage_t *)message)->bd_addr);
        }
        break;
       case EventSysResetAvrcpMode:
        {
            u16 index = *(u16 *) message;
            lIndicateEvent = FALSE ;
            theSink.avrcp_link_data->link_active[index] =  FALSE;
        }
        break;

        case EventUsrTwsQualificationVolUp:
            LOGD("TWS Qualification Volume Up\n" );
            handleAvrcpQualificationVolumeUp();
            break;

        case EventUsrTwsQualificationVolDown:
            LOGD("TWS Qualification Volume Down\n" );
            handleAvrcpQualificationVolumeDown();
            break;

        case EventUsrTwsQualificationSetAbsVolume:
            LOGD("TWS SetAbsoluteVolume\n" );
            handleAvrcpQualificationSetAbsoluteVolume();
            break;
			
        case EventUsrTwsQualificationPlayTrack:
            LOGD("TWS Qualification Play Track\n" );
            handleAvrcpQualificationPlayTrack();
            break;

        case EventUsrTwsQualificationAVRCPConfigureDataSize:
            LOGD("TWS Qualification AVRCP Configure Data Size\n" );
            handleAvrcpQualificationConfigureDataSize();
            break;

#endif

        case EventUsrSwitchAudioMode:
        {
            AUDIO_MODE_T mode = AUDIO_MODE_CONNECTED;
            /* If USB in use set current USB mode */
            usbAudioGetMode(&mode);
            /* cycle through EQ modes */
            theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = A2DP_MUSIC_PROCESSING_FULL_NEXT_EQ_BANK;
            LOGD("EventUsrSwitchAudioMode %x\n", theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing );
            AudioSetMode(mode, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
        break;

       case EventUsrButtonLockingToggle:
            LOGD("EventUsrButtonLockingToggle (%d)\n",theSink.buttons_locked);
            if (theSink.buttons_locked)
            {
                MessageSend( &theSink.task , EventUsrButtonLockingOff , 0 ) ;
            }
            else
            {
                MessageSend( &theSink.task , EventUsrButtonLockingOn , 0 ) ;
            }
        break;

        case EventUsrButtonLockingOn:
            LOGD("EventUsrButtonLockingOn\n");
            theSink.buttons_locked = TRUE;
        break;

        case EventUsrButtonLockingOff:
            LOGD("EventUsrButtonLockingOff\n");
            theSink.buttons_locked = FALSE;
        break;

        case EventUsrAudioPromptsOff:
            LOGD("EventUsrAudioPromptsOff");
            /* disable audio prompts */

            /* Play the disable audio prompts prompt before actually disabling them */
            if(theSink.audio_prompts_enabled == TRUE) /* Check if audio prompts are already enabled */
            {
                TonesPlayEvent( id );
            }

            theSink.audio_prompts_enabled = FALSE;
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;
        break;

        case EventUsrAudioPromptsOn:
            LOGD("EventUsrAudioPromptsOn");
            /* enable audio prompts */
            theSink.audio_prompts_enabled = TRUE;
            /* write enable state to pskey user 12 */
            configManagerWriteSessionData () ;
        break;

        case EventUsrTestModeAudio:
            LOGD("EventUsrTestModeAudio\n");
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterAudioTestMode();
        break;

        case EventUsrTestModeTone:
            LOGD("EventUsrTestModeTone\n");
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterToneTestMode();
        break;

        case EventUsrTestModeKey:
            LOGD("EventUsrTestModeKey\n");
            if (lState != deviceTestMode)
            {
                MessageSend(task, EventUsrEnterDUTState, 0);
            }
            enterKeyTestMode();
        break;

#ifdef ENABLE_SPEECH_RECOGNITION
        case EventSysSpeechRecognitionStart:
        {

            if ( speechRecognitionIsEnabled() )
                speechRecognitionStart() ;
            else
                lIndicateEvent = FALSE;
        }
        break ;
        case EventSysSpeechRecognitionStop:
        {
            if(speechRecognitionIsEnabled() )
                speechRecognitionStop() ;
            else
                lIndicateEvent = FALSE;
        }
        break ;
        /* to tune the Speech Recognition using the UFE generate this event */
        case EventUsrSpeechRecognitionTuningStart:
        {
            /* ensure speech recognition is enabled */
            if ( speechRecognitionIsEnabled() )
            {
                /* ensure not already in tuning mode */
                if(!theSink.csr_speech_recognition_tuning_active)
                {
                    theSink.csr_speech_recognition_tuning_active = TRUE;
                    speechRecognitionStart() ;
                }
            }
        }
        break;

        case EventSysSpeechRecognitionTuningYes:
        break;

        case EventSysSpeechRecognitionTuningNo:
        break;

        case EventSysSpeechRecognitionFailed:
        break;
#endif

        case EventUsrTestDefrag:
            LOGD("EventUsrTestDefrag\n");
            configManagerFillPs();
        break;

        case EventSysStreamEstablish:
            LOGD("EventSysStreamEstablish[%u]\n", ((EVENT_STREAM_ESTABLISH_T *)message)->priority);
            connectA2dpStream( ((EVENT_STREAM_ESTABLISH_T *)message)->priority, 0 );
        break;

        case EventSysA2dpConnected:
            LOGD("EventSysA2dpConnected\n");
        break;

        case EventSysUpdateAttributes:
            deviceManagerDelayedUpdateAttributes((EVENT_UPDATE_ATTRIBUTES_T*)message);
        break;

        case EventUsrPeerSessionConnDisc:
            LOGD("PeerSessionConnDisc [%d]\n" , lState );
            /*go into pairing mode*/
            if ( lState != deviceLimbo)
            {
                /* ensure there is only one device connected to allow peer dev to connect */
                if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
                {
#ifdef ENABLE_PEER
                    u16 index;
                    u16 srcIndex;
                    u16 avrcpIndex;
                    /* check whether the a2dp connection is present and streaming data and that the audio is routed, if thats true then pause the stream */
                    if(theSink.routed_audio && getA2dpIndexFromSink(theSink.routed_audio, &index)
                        && (A2dpMediaGetState(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index]) == a2dp_stream_streaming)
                        && a2dpGetSourceIndex(&srcIndex) && (srcIndex == index)
                        && sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[index], &avrcpIndex, TRUE))
                    {
                        /* cancel any queued ff or rw requests and then pause the streaming*/
                        MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                        MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
                        sinkAvrcpPlayPauseRequest(avrcpIndex,AVRCP_PAUSE);
                    }
#endif

                    theSink.inquiry.session = inquiry_session_peer;
                    stateManagerEnterConnDiscoverableState( FALSE );
                }
                /* no free connections, indicate an error condition */
                else
                {
                    lIndicateEvent = FALSE;
                    MessageSend ( &theSink.task , EventSysError , 0 ) ;
                }
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;

        case ( EventUsrPeerSessionInquire ):
            LOGD("PeerSessionInquire\n");

            /* ensure there is only one device connected to allow peer dev to connect */
            if(deviceManagerNumConnectedDevs() < MAX_A2DP_CONNECTIONS)
            {
#ifdef ENABLE_PEER
                u16 index;
                u16 srcIndex;
                u16 avrcpIndex;
                /* check whether the a2dp connection is present and streaming data and that the audio is routed, if thats true then pause the stream */
                if(theSink.routed_audio && getA2dpIndexFromSink(theSink.routed_audio, &index)
                    && (A2dpMediaGetState(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index]) == a2dp_stream_streaming)
                    && a2dpGetSourceIndex(&srcIndex) && (srcIndex == index)
                    && sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[index], &avrcpIndex, TRUE))
                {
                    /* cancel any queued ff or rw requests and then pause the streaming*/
                    MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                    MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);
                    sinkAvrcpPlayPauseRequest(avrcpIndex,AVRCP_PAUSE);
                }
#endif
                lIndicateEvent = inquiryPair( inquiry_session_peer, FALSE );
            }
            /* no free connections, indicate an error condition */
            else
            {
                lIndicateEvent = FALSE;
                MessageSend ( &theSink.task , EventSysError , 0 ) ;
            }

        break;

        case EventUsrPeerSessionEnd:
        {
#ifdef PEER_SCATTERNET_DEBUG   /* Scatternet debugging only */
            u16 i;
            for_all_a2dp(i)
            {
                if (theSink.a2dp_link_data)
                {
                    theSink.a2dp_link_data->invert_ag_role[i] = !theSink.a2dp_link_data->invert_ag_role[i];
                    LOGD("invert_ag_role[%u] = %u\n",i,theSink.a2dp_link_data->invert_ag_role[i]);

                    if (theSink.a2dp_link_data->connected[i] && (theSink.a2dp_link_data->peer_device[i] != remote_device_peer))
                    {
                        linkPolicyUseA2dpSettings( theSink.a2dp_link_data->device_id[i],
                                                   theSink.a2dp_link_data->stream_id[i],
                                                   A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[i]) );
                    }
                }
            }
#else   /* Normal operation */
            LOGD("EventUsrPeerSessionEnd\n");
            lIndicateEvent = disconnectAllA2dpPeerDevices();
#endif
        }
        break;

        case EventUsrPeerReserveLinkOn:
#ifdef ENABLE_PEER
            peerReserveLink(TRUE);  
#endif              
        break;

        case EventUsrPeerReserveLinkOff:
#ifdef ENABLE_PEER
            peerReserveLink(FALSE);
#endif
        break;        
        
        case EventUsrSwapA2dpMediaChannel:
            /* attempt to swap media channels, don't indicate event if not successful */
            if(!audioSwapMediaChannel())
                lIndicateEvent = FALSE;
        break;

        /* bass boost enable disable toggle */
        case EventUsrBassBoostEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_BASS_BOOST_BYPASS)
            {
                /* disable bass boost */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,FALSE);
            }
            else
            {
                /* enable bass boost */
                sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,TRUE);
            }
        break;

        /* bass boost enable indication */
        case EventUsrBassBoostOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable bass boost */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,TRUE);
        break;

        /* bass boost disable indication */
        case EventUsrBassBoostOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable bass boost */
            sinkAudioSetEnhancement(MUSIC_CONFIG_BASS_BOOST_BYPASS,FALSE);
        break;

        /* 3D enhancement enable disable toggle */
        case EventUsr3DEnhancementEnableDisableToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_SPATIAL_BYPASS)
            {
                /* disable 3d */
                sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,FALSE);
            }
            else
            {
                /* enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,TRUE);
            }
        break;

        /* 3D enhancement enable indication */
        case EventUsr3DEnhancementOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,TRUE);
        break;

        /* 3D enhancement disable indication */
        case EventUsr3DEnhancementOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_SPATIAL_BYPASS,FALSE);
        break;

         /* User EQ enable disable toggle indication */
        case EventUsrUserEqOnOffToggle:
            if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & MUSIC_CONFIG_USER_EQ_BYPASS)
            {
                /* disable User EQ */
                sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, FALSE);
            }
            else
            {
                /* enable User EQ */
                sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, TRUE);
            }
        break;

       /* User EQ enable indication */
        case EventUsrUserEqOn:
            /* logic inverted in a2dp plugin lib, disable bypass to enable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, TRUE);
        break;

        /* User EQ disable indication */
        case EventUsrUserEqOff:
            /* logic inverted in a2dp plugin lib, enable bypass to disable 3d */
            sinkAudioSetEnhancement(MUSIC_CONFIG_USER_EQ_BYPASS, FALSE);
        break;

        /* check whether the Audio Amplifier drive can be turned off after playing
           a tone or voice prompt */
        case EventSysCheckAudioAmpDrive:
            /* cancel any pending messages */
            MessageCancelAll( &theSink.task , EventSysCheckAudioAmpDrive);
            /* when the device is no longer routing audio tot he speaker then
               turn off the audio amplifier */
            if((!IsAudioBusy()) && (!theSink.routed_audio))
            {
                LOGD("EventSysCheckAudioAmpDrive turn off amp\n");
                PioDrivePio(PIO_AUDIO_ACTIVE, FALSE);
            }
            /* audio is still busy, check again later */
            else
            {
                LOGD("EventSysCheckAudioAmpDrive still busy, reschedule\n" );
                MessageSendLater(&theSink.task , EventSysCheckAudioAmpDrive, 0, CHECK_AUDIO_AMP_PIO_DRIVE_DELAY);
            }
        break;

        /* external microphone has been connected */
        case EventUsrExternalMicConnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        break;

        /* external microphone has been disconnected */
        case EventUsrExternalMicDisconnected:
            theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_NOT_FITTED;
            /* if routing audio update the mic source for dsp apps that support it */
            if(theSink.routed_audio)
                AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
       break;

       /* event to enable the simple speech recognition functionality, persistant over power cycle */
       case EventUsrSSROn:
            theSink.ssr_enabled = TRUE;
       break;

       /* event to disable the simple speech recognition functionality, persistant over power cycle */
       case EventUsrSSROff:
            theSink.ssr_enabled = FALSE;
       break;

       /* NFC tag detected, determine action based on current connection state */
       case EventUsrNFCTagDetected:
            /* if not connected to an AG, go straight into pairing mode */
            if(stateManagerGetState() < deviceConnected)
                stateManagerEnterConnDiscoverableState( TRUE );
            /* otherwise see if audio is on the device and attempt
               to transfer audio if not present */
            else
               sinkCheckForAudioTransfer();
       break;

       /* check whether the current routed audio is still the correct one and
          change sources if appropriate */
       case EventSysCheckAudioRouting:
            /* check audio routing */
            audioHandleRouting(audio_source_none);
            /* don't indicate event as may be generated by USB prior to configuration
               being loaded */
            lIndicateEvent = FALSE;
       break;
       

            
	   /* Audio amplifier is to be shut down by PIO for power saving purposes */
       case EventSysAmpPowerDown:
            LOGD("EventSysAmpPowerDown\n");
            stateManagerAmpPowerControl(POWER_DOWN);
       break;
               
       case EventSysAmpPowerUp:
            LOGD("EventSysAmpPowerUp\n");
            stateManagerAmpPowerControl(POWER_UP);
            break;

       case EventUsrFmRxOn:
       case EventUsrFmRxOff:
       case EventUsrFmRxTuneUp:
       case EventUsrFmRxTuneDown:
       case EventUsrFmRxStore:
       case EventUsrFmRxTuneToStore:
       case EventUsrFmRxErase:
           lIndicateEvent = sinkFmProcessEventUsrFmRx(id);
           break;

       case EventUsrSelectAudioSourceAnalog:
       case EventUsrSelectAudioSourceSpdif:
       case EventUsrSelectAudioSourceUSB:
       case EventUsrSelectAudioSourceAG1:
       case EventUsrSelectAudioSourceAG2:
       case EventUsrSelectAudioSourceFM:
       case EventUsrSelectAudioSourceNone:
       case EventUsrSelectAudioSourceNext:
           lIndicateEvent = processEventUsrSelectAudioSource(id);
           break;



#ifdef ENABLE_SUBWOOFER
       case EventUsrSubwooferStartInquiry:
            handleEventUsrSubwooferStartInquiry();
       break;

       case EventSysSubwooferCheckPairing:
            handleEventSysSubwooferCheckPairing();
       break;

       case EventSysSubwooferOpenLLMedia:
            /* open a Low Latency media connection */
            handleEventSysSubwooferOpenLLMedia();
       break;

       case EventSysSubwooferOpenStdMedia:
            /* open a standard latency media connection */
            handleEventSysSubwooferOpenStdMedia();
       break;

       case EventUsrSubwooferVolumeUp:
            handleEventUsrSubwooferVolumeUp();
       break;

       case EventUsrSubwooferVolumeDown:
            handleEventUsrSubwooferVolumeDown();
       break;

       case EventSysSubwooferCloseMedia:
            handleEventSysSubwooferCloseMedia();
       break;

       case EventSysSubwooferStartStreaming:
            handleEventSysSubwooferStartStreaming();
       break;

       case EventSysSubwooferSuspendStreaming:
            handleEventSysSubwooferSuspendStreaming();
       break;

       case EventUsrSubwooferDisconnect:
            handleEventUsrSubwooferDisconnect();
       break;

       case EventUsrSubwooferDeletePairing:
            handleEventUsrSubwooferDeletePairing();
       break;

       /* set subwoofer volume level by message to maintain synchronisation with
          audio plugins */
       case EventSysSubwooferSetVolume:
            /* send volume level change to subwoofer */
            updateSwatVolumeNow(((SWAT_VOLUME_CHANGE_MSG_T*)message)->new_volume);
       break;
#endif

       case EventUsrEnterDriverlessDFUMode:
            LoaderModeEnter();
       break;

#ifdef ENABLE_PARTYMODE
       /* enabling the runtime control of the party mode feature */
       case EventUsrPartyModeOn:
       {
            /* ensure a party mode operating type has been selected in configuration 
               before enabling the feature */
            if(theSink.features.PartyMode)   
            {    
               /* turn party mode on */
               theSink.PartyModeEnabled = TRUE;

			   /* if HFP channels are connected, disconnect them, it's 
		        * more code efficient to just call to disconnect both 
		        * links and let it fail if they're not already connected 
		        */

               HfpSlcDisconnectRequest(hfp_primary_link);
		       HfpSlcDisconnectRequest(hfp_secondary_link);

               /* if more than one device connected, drop the second device */
               if( deviceManagerNumConnectedDevs() > 1 )
               {
                   /* but allow a second one to be connected */
                   sinkPartyModeDisconnectDevice(a2dp_secondary);
               }
               else
               {
                   /* ensure headset is discoverable
                      and connectable once enabled */
                   sinkEnableConnectable();
               }
               /* ensure pairable */
               sinkEnableDiscoverable();
           }
       }
       break;

       /* disabling the runtime control of the party mode feature */
       case EventUsrPartyModeOff:
       {
           /* no need to ensure a party mode operating type has been selected
              in configuration before disabling the feature */
           if(theSink.PartyModeEnabled)
           {
               /* turn party mode off */
               theSink.PartyModeEnabled = FALSE;
			   
			   /* Shutting down all connections as it's impossible to re-connect
			    * after party mode ... because the connection and pairing tables
				* are not accurately maintained after so many connections and 
				* pairings while in Party Mode. */

			   sinkDisconnectAllSlc();
               disconnectAllA2dpAvrcp(TRUE);

               
               /* ensure pairable */
               sinkEnableDiscoverable();
           }
       }
       break;
	   
       /* connected device hasn't played music before timeout, disconnect it to allow
          another device to connect */
       case EventSysPartyModeTimeoutDevice1:
            sinkPartyModeDisconnectDevice(a2dp_primary);
       break;

       /* connected device hasn't played music before timeout, disconnect it to allow
          another device to connect */
       case EventSysPartyModeTimeoutDevice2:
            sinkPartyModeDisconnectDevice(a2dp_secondary);
       break;
#endif

#ifdef ENABLE_GAIA
        case EventUsrGaiaDFURequest:
        /*  GAIA DFU requires that audio not be busy, so disallow any tone  */
            lIndicateEvent = FALSE;
            gaiaDfuRequest();
        break;
#endif

        case EventSysRemoteControlCodeReceived:
             /* Display a led pattern*/
        break;

#ifdef ENABLE_IR_REMOTE
        case EventSysIRCodeLearnSuccess:
        {
            /* TODO : Play a tone or something */
        }
        break;
        case EventSysIRCodeLearnFail:
        {
            /* TODO : Play a tone or something */
        }
        break;
        case EventSysIRLearningModeTimeout:
        {
            irStopLearningMode();
        }
        break;
        case EventSysIRLearningModeReminder:
        {
            handleIrLearningModeReminder();
        }
        break;
        case EventUsrStartIRLearningMode:
        {
            irStartLearningMode();
        }
        break;
        case EventUsrStopIRLearningMode:
        {
            irStopLearningMode();
        }
        break;
        case EventUsrClearIRCodes:
        {
            irClearLearntCodes();
        }
        break;
#endif

#if defined ENABLE_PEER && defined PEER_TWS
        case EventUsrMasterDeviceTrimVolumeUp:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(increase_volume, tws_master);
            break;

        case EventUsrMasterDeviceTrimVolumeDown:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(decrease_volume, tws_master);
            break;

        case EventUsrSlaveDeviceTrimVolumeUp:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(increase_volume, tws_slave);
            break;

        case EventUsrSlaveDeviceTrimVolumeDown:
            VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(decrease_volume, tws_slave);
            break;

        case EventUsrChangeAudioRouting:
        {
            if(theSink.features.PeerPermittedRouting != 0)
            {
                u16 current_routing = ((theSink.a2dp_link_data->a2dp_audio_mode_params.master_routing_mode & 0x3) << 2) | (theSink.a2dp_link_data->a2dp_audio_mode_params.slave_routing_mode & 0x3);
                u16 distance = 16;
                u16 index = 0;
                u16 i;
            
                /* Find entry in tws_audio_routing table which is closest to current routing mode */
                for(i = 0; i < sizeof(tws_audio_routing); i++)
                {
                    if (tws_audio_routing[i] < current_routing)
                    {
                        if ((current_routing - tws_audio_routing[i]) < distance)
                        {
                            distance = current_routing - tws_audio_routing[i];
                            index = i;
                        }
                    }
                    else
                    {
                        if ((tws_audio_routing[i] - current_routing) < distance)
                        {
                            distance = tws_audio_routing[i] - current_routing;
                            index = i;
                        }
                    }
                }

                do{
                    /* Select next routing mode in table */
                    index = (index + 1) % sizeof(tws_audio_routing);
                }while(!((1 << index) & theSink.features.PeerPermittedRouting));
            
                sinkA2dpSetPeerAudioRouting((tws_audio_routing[index] >> 2) & 0x3, tws_audio_routing[index] & 0x3);
            }
            break;
        }
#endif

#ifdef ENABLE_GAIA_PERSISTENT_USER_EQ_BANK

        /* When timeout occurs, session data needs to be updated for new EQ settings */
        case EventSysGaiaEQChangesStoreTimeout:
            configManagerWriteSessionData ();
            break;
#endif

        case EventSysCheckDefrag:
        case EventSysDefrag:
            configManagerProcessEventSysDefrag(id);
            break;

        case EventSysToneDigit0:
        case EventSysToneDigit1:
        case EventSysToneDigit2:
        case EventSysToneDigit3:
        case EventSysToneDigit4:
        case EventSysToneDigit5:
        case EventSysToneDigit6:
        case EventSysToneDigit7:
        case EventSysToneDigit8:
        case EventSysToneDigit9:
            break;

        /* event to start an LED state indication, called by
           message to reduce maximum stack usage */
        case EventSysLEDIndicateState:
            {
               /* use LEDS to indicate current state */
               LEDManagerIndicateState ( ((LED_INDICATE_STATE_T*)message)->state ) ;
            }
            break;

        case EventUsrBleStartBonding:
            {
                LOGD("BLE Bondable\n");
                sinkBleBondableEvent();
            }
            break;

        case EventUsrBleDeleteDevice:
            {
                LOGD("Delete Marked LE device \n" );
                sinkBleDeleteMarkedDevices();
            }
            break;

        case EventSysBleBondablePairingTimeout:
            {
                LOGD("BLE Bondable Pairing Timeout\n");
                sinkBleBondablePairingTimeoutEvent();
            }
            break;

        case EventSysBleBondableConnectionTimeout:
            {
                LOGD("BLE Bondable Connection Timeout\n");
                sinkBleBondableConnectionTimeoutEvent();
            }
            break;

        case EventUsrBleSwitchPeripheral:
        case EventSysBleSwitchPeripheral:
            {
                LOGD("BLE Switch Peripheral\n");
                sinkBleSwitchPeripheralEvent();
            }
            break;

        case EventUsrBleSwitchCentral:
        case EventSysBleSwitchCentral:
            {
                LOGD("BLE Switch Central\n");
                sinkBleSwitchCentralEvent();
            }
            break;
            
       case EventUsrFindMyRemoteImmAlertMild:
            {
                LOGD("Find my remote Imm Alert Mild\n" );
                sinkGattIasClientSetAlert(gatt_imm_alert_level_mild, sink_gatt_ias_alert_remote);
            }
            break;
            
        case EventUsrFindMyRemoteImmAlertHigh:
            {
                LOGD("Find my remote Imm Alert High\n" );
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_remote);
            }
            break;

       case EventUsrFindMyPhoneImmAlertMild:
            {
                LOGD("Find my phone Imm Alert Mild\n" );
                sinkGattIasClientSetAlert(gatt_imm_alert_level_mild, sink_gatt_ias_alert_phone);
            }
            break;

        case EventUsrFindMyPhoneImmAlertHigh:
            {
                LOGD("Find my phone Imm Alert High \n" );
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_phone);
            }
            break;

        /*Alert the phone from remote control using HID. This event needs to be mapped to the HID code for alert*/
        case EventUsrFindMyPhoneRemoteImmAlertHigh:
            {
                LOGD("Find my phone Imm Alert High - Triggered from HID Remote\n" );
                sinkGattImmAlertLocalAlert(gatt_imm_alert_level_high);
                sinkGattIasClientSetAlert(gatt_imm_alert_level_high, sink_gatt_ias_alert_phone);
            }
            break;

        case EventUsrImmAlertStop:
        case EventSysImmAlertTimeout:
            {
                LOGD("IAS : Stop Alert/Timeout\n" );

                /*Local Alert*/
                sinkGattServerImmAlertStopAlert();

                /*Remote Alert*/
                if (sinkGattIasClientEnabled())
                {
                    sinkGattIasClientSetAlert(gatt_imm_alert_level_no, sink_gatt_ias_alert_none);
                }
            }
            break;

        case EventSysImmAlertMild:
            {
                LOGD("Mild Alert \n" );
                sinkGattServerImmAlertMild(theSink.conf1->timeouts.ImmediateAlertTimer_s);
            }
            break;

        case EventSysImmAlertHigh:
            {
                LOGD("High Alert \n" );
                sinkGattServerImmAlertHigh(theSink.conf1->timeouts.ImmediateAlertTimer_s);
            }
        break;

        case EventSysLlsAlertTimeout:
        case EventUsrLlsAlertStop:
        {
            LOGD("Link Loss Stop Alert \n" );
            sinkGattLinkLossAlertStop();
        }
        break;
        case EventSysLlsAlertMild:
        {
            LOGD("Link Loss Mild Alert \n" );
            sinkGattLinkLossAlertMild(theSink.conf1->timeouts.LinkLossTimer_s);
        }
        break;
        case EventSysLlsAlertHigh:
        {
            LOGD("Link Loss High Alert \n" );
            sinkGattLinkLossAlertHigh(theSink.conf1->timeouts.LinkLossTimer_s);
        }
        break;

        case EventSysAncsEmailAlert:
            LOGD("Recieved ANCS Email Alert\n" );
        break;

        case EventUsrBleHidExtraConfig:
            LOGD("BLE HID Extra configuration for qualification\n" );
            sinkGattHIDClientExtraConfig();
        break;
        
        

        case EventSysReboot:
            BootSetMode(BootGetMode());
            break;

        case EventSysBleGapRoleTimeout:
            LOGD("Recieved BLE GAP Role Timeout\n" );
            sinkBleRoleTimeoutEvent();
            break;

        case EventUsrAncOn:
            sinkAncEnable();
            break;

        case EventUsrAncOff:
            sinkAncDisable();
            break;

        case EventUsrAncToggleOnOff:
            sinkAncToggleEnable();
            break;

        case EventUsrAncLeakthroughMode:
            sinkAncSetLeakthroughMode();
            break;

        case EventUsrAncActiveMode:
            sinkAncSetActiveMode();
            break;

        case EventUsrAncNextMode:
            sinkAncSetNextMode();
            break;

        case EventUsrAncVolumeDown:
            sinkAncVolumeDown();
            break;

        case EventUsrAncVolumeUp:
            sinkAncVolumeUp();
            break;

        case EventSysAncDisabled:
            LOGD("ANC Disabled\n" );
            break;

        case EventSysAncActiveModeEnabled:
            LOGD("ANC Enabled in Active Mode\n" );
            break;

        case EventSysAncLeakthroughModeEnabled:
            LOGD("ANC Enabled in Leakthrough Mode\n" );
            break;

        case EventUsrStartA2DPStream:
            LOGD("A2DP Start streaming media\n" );
            audioA2dpStartStream();
            break;

#ifdef ENABLE_PEER
        case EventUsrTwsQualificationEnablePeerOpen:
            LOGD("TWS Qualification Enable Opening of Peer Media Channel\n" );
            handlePeerQualificationEnablePeerOpen();
            break;
        case EventSysA2DPPeerLinkLossTimeout:
            theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE; 
            LOGD("EventSysA2DPPeerLinkLossTimeout\n");
            break;         
        case EventSysRemovePeerTempPairing:
            HandlePeerRemoveAuthDevice((bdaddr*)message); 
            break; 
        case EventSysA2dpPauseSuspendTimeoutDevice1:
            /* Connected device hasn't suspended its a2dp media channel before timeout,try to force it. */
            a2dpSuspendNonRoutedStream(a2dp_primary);
            break;

        case EventSysA2dpPauseSuspendTimeoutDevice2:
            /* Connected device hasn't suspended its a2dp media channel before timeout, try to force it. */
            a2dpSuspendNonRoutedStream(a2dp_secondary);      
            break;
#endif
            

        default :
            LOGD("UE unhandled!! [%x]\n", id );
        break ;

    }

    if(lResetAutoSwitchOff && theSink.conf1->timeouts.AutoSwitchOffTime_s !=0)
    {
        /*LOGD("AUTOSent Ev[%x] Time[%d]\n",id , theSink.conf1->timeouts.AutoSwitchOffTime_s );*/
        sinkSendLater(EventSysAutoSwitchOff, D_SEC(theSink.conf1->timeouts.AutoSwitchOffTime_s));
    }
	
        /* Inform the event indications that we have received a user event*/
        /* For all events except the end event notification as this will just end up here again calling itself...*/
    if ( lIndicateEvent && (!eventToBeIndicatedBeforeProcessing(id)))
    {
    	IndicateEvent(id);
    }

#ifdef ENABLE_GAIA
    gaiaReportEvent(id);
#endif

#ifdef TEST_HARNESS
    vm2host_send_event(id);
#endif

#ifdef DEBUG_MALLOC
    printf("Event [%x] Available SLOTS:[%d]\n" ,id, VmGetAvailableAllocations() ) ;
#endif

}


/*************************************************************************
NAME
    handleHFPMessage

DESCRIPTION
    handles the messages from the user events

RETURNS

*/
static void handleHFPMessage  ( Task task, MessageId id, Message message )
{
    LOGD("HFP = [%x]\n", id);

    switch(id)
    {
        /* -- Handsfree Profile Library Messages -- */
    case HFP_INIT_CFM:
        {
            /* Init configuration that is required now */


            InitEarlyUserFeatures();
            LOGD("HFP_INIT_CFM - enable streaming[%x]\n", theSink.features.EnableA2dpStreaming);

            sinkInitConfigureDeviceClass();

            if  ( stateManagerGetState() == deviceLimbo )
            {
                if ( ((HFP_INIT_CFM_T*)message)->status == hfp_success )
                    sinkInitComplete( (HFP_INIT_CFM_T*)message );
                else
                    Panic();
            }
        }

    break;

    case HFP_SLC_CONNECT_IND:
        LOGD("HFP_SLC_CONNECT_IND\n");
        if (stateManagerGetState() != deviceLimbo)
        {
            sinkHandleSlcConnectInd((HFP_SLC_CONNECT_IND_T *) message);
        }
    break;

    case HFP_SLC_CONNECT_CFM:
        LOGD("HFP_SLC_CONNECT_CFM [%x]\n", ((HFP_SLC_CONNECT_CFM_T *) message)->status );
        if (stateManagerGetState() == deviceLimbo)
        {
            if ( ((HFP_SLC_CONNECT_CFM_T *) message)->status == hfp_success )
            {
                /*A connection has been made and we are now  logically off*/
                sinkDisconnectAllSlc();
            }
        }
        else
        {
            sinkHandleSlcConnectCfm((HFP_SLC_CONNECT_CFM_T *) message);
#ifdef ENABLE_PEER            
            if(!peerLinkReservedCanDeviceConnect(&(((HFP_SLC_CONNECT_CFM_T *)message)->bd_addr)))
            {  /* Another link is reserved for a peer device to connect, disconnect the second AG.*/ 
                sinkDisconnectSlcFromDevice(&(((HFP_SLC_CONNECT_CFM_T *)message)->bd_addr));
            }
#endif
#ifdef ENABLE_PARTYMODE
            if( theSink.PartyModeEnabled )
			{
			    /* if an HFP channel is connected, disconnect it 
				 * immediately, the identity of the conenction is 
				 * found in the message confirming the connection */
				
                HfpSlcDisconnectRequest( ((HFP_SLC_CONNECT_CFM_T *)message)->priority );
            }
#endif
        }
        break;

    case HFP_SLC_LINK_LOSS_IND:
        LOGD("HFP_SLC_LINK_LOSS_IND\n");
        slcHandleLinkLossInd((HFP_SLC_LINK_LOSS_IND_T*)message);
    break;

    case HFP_SLC_DISCONNECT_IND:
        LOGD("HFP_SLC_DISCONNECT_IND\n");
        LOGD("Handle Disconnect\n");
        sinkHandleSlcDisconnectInd((HFP_SLC_DISCONNECT_IND_T *) message);
    break;
    case HFP_SERVICE_IND:
        LOGD("HFP_SERVICE_IND [%x]\n" , ((HFP_SERVICE_IND_T*)message)->service  );
        indicatorsHandleServiceInd ( ((HFP_SERVICE_IND_T*)message) ) ;
    break;
    /* indication of call status information, sent whenever a change in call status
       occurs within the hfp lib */
    case HFP_CALL_STATE_IND:
        /* the Call Handler will perform device state changes and be
           used to determine multipoint functionality */
        /* don't process call indications if in limbo mode */
        if(stateManagerGetState()!= deviceLimbo)
            sinkHandleCallInd((HFP_CALL_STATE_IND_T*)message);
    break;

    case HFP_RING_IND:
        LOGD("HFP_RING_IND\n");
        sinkHandleRingInd((HFP_RING_IND_T *)message);
    break;
    case HFP_VOICE_TAG_NUMBER_IND:
        LOGD("HFP_VOICE_TAG_NUMBER_IND\n");
        sinkWriteStoredNumber((HFP_VOICE_TAG_NUMBER_IND_T*)message);
    break;
    case HFP_DIAL_LAST_NUMBER_CFM:
        LOGD("HFP_LAST_NUMBER_REDIAL_CFM\n");
        handleHFPStatusCFM (((HFP_DIAL_LAST_NUMBER_CFM_T*)message)->status ) ;
    break;
    case HFP_DIAL_NUMBER_CFM:
        LOGD("HFP_DIAL_NUMBER_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_NUMBER_CFM_T *) message)->status);
        handleHFPStatusCFM (((HFP_DIAL_NUMBER_CFM_T*)message)->status ) ;
    break;
    case HFP_DIAL_MEMORY_CFM:
        LOGD("HFP_DIAL_MEMORY_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_MEMORY_CFM_T *) message)->status);
    break ;
    case HFP_CALL_ANSWER_CFM:
        LOGD("HFP_ANSWER_CALL_CFM\n");
    break;
    case HFP_CALL_TERMINATE_CFM:
        LOGD("HFP_TERMINATE_CALL_CFM %d\n", stateManagerGetState());
    break;
    case HFP_VOICE_RECOGNITION_IND:
        LOGD("HFP_VOICE_RECOGNITION_IND_T [%c]\n" ,TRUE_OR_FALSE( ((HFP_VOICE_RECOGNITION_IND_T* )message)->enable) );
    /*update the state of the voice dialling on the back of the indication*/
        theSink.VoiceRecognitionIsActive = ((HFP_VOICE_RECOGNITION_IND_T* ) message)->enable ;
    break;
    case HFP_VOICE_RECOGNITION_ENABLE_CFM:
        LOGD("HFP_VOICE_RECOGNITION_ENABLE_CFM s[%d] w[%d]i", (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) , theSink.VoiceRecognitionIsActive);

            /*if the cfm is in error then we did not succeed - toggle */
        if  ( (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) )
            theSink.VoiceRecognitionIsActive = 0 ;

        LOGD("[%d]\n", theSink.VoiceRecognitionIsActive);

        handleHFPStatusCFM (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) ;
    break;
    case HFP_CALLER_ID_ENABLE_CFM:
        LOGD("HFP_CALLER_ID_ENABLE_CFM\n");
    break;
    case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
    {
        HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *) message;

        LOGD("HFP_VOLUME_SYNC_SPEAKER_GAIN_IND %d\n", ind->volume_gain);

        VolumeHandleSpeakerGainInd(ind);
    }
    break;
    case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
    {
        HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T*)message;
        LOGD("HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND %d\n", ind->mic_gain);
        if(theSink.features.EnableSyncMuteMicrophones)
        {
            VolumeSetHfpMicrophoneGainCheckMute(ind->priority, ind->mic_gain);
        }
    }

    break;

    case HFP_CALLER_ID_IND:
        {
            HFP_CALLER_ID_IND_T *ind = (HFP_CALLER_ID_IND_T *) message;

            /* ensure this is not a HSP profile */
            LOGD("HFP_CALLER_ID_IND number %s", ind->caller_info + ind->offset_number);
            LOGD(" name %s\n", ind->caller_info + ind->offset_name);

            /* Show name or number on display */
            if (ind->size_name)
                displayShowSimpleText((char *) ind->caller_info + ind->offset_name, 1);

            else
                displayShowSimpleText((char *) ind->caller_info + ind->offset_number, 1);

            /* Attempt to play caller name */
            if(!AudioPromptPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
            {
                /* Caller name not present or not supported, try to play number */
                AudioPromptPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
            }
        }

    break;

    case HFP_UNRECOGNISED_AT_CMD_IND:
    {
        sinkHandleUnrecognisedATCmd( (HFP_UNRECOGNISED_AT_CMD_IND_T*)message ) ;
    }
    break ;

    case HFP_HS_BUTTON_PRESS_CFM:
        {
            LOGD("HFP_HS_BUTTON_PRESS_CFM\n");
        }
    break ;
     /*****************************************************************/

#ifdef THREE_WAY_CALLING
    case HFP_CALL_WAITING_ENABLE_CFM :
            LOGD("HFP_CALL_WAITING_ENABLE_CFM_T [%c]\n", (((HFP_CALL_WAITING_ENABLE_CFM_T * )message)->status == hfp_success) ?'T':'F' );
    break ;
    case HFP_CALL_WAITING_IND:
        {
            /* pass the indication to the multipoint handler which will determine if the call waiting tone needs
               to be played, this will depend upon whether the indication has come from the AG with
               the currently routed audio */
            mpHandleCallWaitingInd((HFP_CALL_WAITING_IND_T *)message);
        }
    break;

#endif
    case HFP_SUBSCRIBER_NUMBERS_CFM:
        LOGD("HFP_SUBSCRIBER_NUMBERS_CFM [%c]\n" , (((HFP_SUBSCRIBER_NUMBERS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' );
    break ;
    case HFP_SUBSCRIBER_NUMBER_IND:
#ifdef DEBUG_MAIN
    {
        u16 i=0;

        LOGD("HFP_SUBSCRIBER_NUMBER_IND [%d]\n" , ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->service );
        for (i=0;i< ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->size_number ; i++)
        {
            LOGD("%c", ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->number[i]);
        }
        LOGD("\n");
    }
#endif
    break ;
    case HFP_CURRENT_CALLS_CFM:
        LOGD("HFP_CURRENT_CALLS_CFM [%c]\n", (((HFP_CURRENT_CALLS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' );
    break ;
    case HFP_CURRENT_CALLS_IND:
        LOGD("HFP_CURRENT_CALLS_IND id[%d] mult[%d] status[%d]\n" ,
             ((HFP_CURRENT_CALLS_IND_T*)message)->call_idx ,
             ((HFP_CURRENT_CALLS_IND_T*)message)->multiparty  ,
             ((HFP_CURRENT_CALLS_IND_T*)message)->status);
    break;
    case HFP_AUDIO_CONNECT_IND:
        LOGD("HFP_AUDIO_CONNECT_IND\n");
        audioHandleSyncConnectInd( (HFP_AUDIO_CONNECT_IND_T *)message ) ;
    break ;
    case HFP_AUDIO_CONNECT_CFM:
        LOGD("HFP_AUDIO_CONNECT_CFM[%x][%x][%s%s%s] r[%d]t[%d]\n", ((HFP_AUDIO_CONNECT_CFM_T *)message)->status ,
             ((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink ,
             ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_sco) ? "SCO" : "" )      ,
             ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_esco) ? "eSCO" : "" )    ,
             ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_unknown) ? "unk?" : "" ) ,
             ((HFP_AUDIO_CONNECT_CFM_T *)message)->rx_bandwidth ,
             ((HFP_AUDIO_CONNECT_CFM_T *)message)->tx_bandwidth
            );
        /* should the device receive a sco connect cfm in limbo state */
        if (stateManagerGetState() == deviceLimbo)
        {
            /* confirm that it connected successfully before disconnecting it */
            if (((HFP_AUDIO_CONNECT_CFM_T *)message)->status == hfp_audio_connect_no_hfp_link)
            {
                LOGD("HFP_AUDIO_CONNECT_CFM in limbo state, disconnect it\n" );
                ConnectionSyncDisconnect(((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink, hci_error_oetc_user);
            }
        }
        /* not in limbo state, process sco connect indication */
        else
        {
            audioHandleSyncConnectCfm((HFP_AUDIO_CONNECT_CFM_T *)message);
        }
    break ;
    case HFP_AUDIO_DISCONNECT_IND:
        LOGD("HFP_AUDIO_DISCONNECT_IND [%x]\n", ((HFP_AUDIO_DISCONNECT_IND_T *)message)->status);
        audioHandleSyncDisconnectInd ((HFP_AUDIO_DISCONNECT_IND_T *)message) ;
    break ;
    case HFP_SIGNAL_IND:
        LOGD("HFP_SIGNAL_IND [%d]\n", ((HFP_SIGNAL_IND_T* )message)->signal );
    break ;
    case HFP_ROAM_IND:
        LOGD("HFP_ROAM_IND [%d]\n", ((HFP_ROAM_IND_T* )message)->roam );
    break;
    case HFP_BATTCHG_IND:
        LOGD("HFP_BATTCHG_IND [%d]\n", ((HFP_BATTCHG_IND_T* )message)->battchg );
    break;

/*******************************************************************/

    case HFP_CSR_FEATURES_TEXT_IND:
        csr2csrHandleTxtInd () ;
    break ;

    case HFP_CSR_FEATURES_NEW_SMS_IND:
       csr2csrHandleSmsInd () ;
    break ;

    case HFP_CSR_FEATURES_GET_SMS_CFM:
       csr2csrHandleSmsCfm() ;
    break ;

    case HFP_CSR_FEATURES_BATTERY_LEVEL_REQUEST_IND:
       csr2csrHandleAgBatteryRequestInd() ;
    break ;

/*******************************************************************/

/*******************************************************************/

    /*******************************/

    default :
        LOGD("HFP ? [%x]\n",id);
    break ;
    }
}

/*************************************************************************
NAME
    handleCodecMessage

DESCRIPTION
    handles the codec Messages

RETURNS

*/
static void handleCodecMessage  ( Task task, MessageId id, Message message )
{
    LOGD("CODEC MSG received [%x]\n", id);

    if (id == CODEC_INIT_CFM )
    {       /* The codec is now initialised */

        if ( ((CODEC_INIT_CFM_T*)message)->status == codec_success)
        {
            LOGD("CODEC_INIT_CFM\n");
            sinkHfpInit();
            theSink.codec_task = ((CODEC_INIT_CFM_T*)message)->codecTask ;
        }
        else
        {
            Panic();
        }
    }
}

/* Handle any audio plugin messages */
static void handleAudioPluginMessage( Task task, MessageId id, Message message )
{
    switch (id)
    {
        case AUDIO_PLUGIN_DSP_IND:
            /* Clock mismatch rate, sent from the DSP via the a2dp decoder common plugin? */
            if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE)
            {
                handleA2DPStoreClockMismatchRate(((AUDIO_PLUGIN_DSP_IND_T*)message)->value[0]);
            }
            /* Current EQ bank, sent from the DSP via the a2dp decoder common plugin? */
            else if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == A2DP_MUSIC_MSG_CUR_EQ_BANK)
            {
                handleA2DPStoreCurrentEqBank(((AUDIO_PLUGIN_DSP_IND_T*)message)->value[0]);
            }
            /* Current enhancements, sent from the DSP via the a2dp decoder common plugin? */
            else if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == A2DP_MUSIC_MSG_ENHANCEMENTS)
            {
                handleA2DPStoreEnhancements(((~((AUDIO_PLUGIN_DSP_IND_T*)message)->value[1]) & (MUSIC_CONFIG_CROSSOVER_BYPASS|MUSIC_CONFIG_BASS_BOOST_BYPASS|MUSIC_CONFIG_SPATIAL_BYPASS|MUSIC_CONFIG_USER_EQ_BYPASS)));
            }
        break;

        /* indication that the DSP is ready to accept data ensuring no audio samples are disposed of */
        case AUDIO_PLUGIN_DSP_READY_FOR_DATA:
            /* ensure dsp is up and running */

#if defined ENABLE_GAIA && defined ENABLE_GAIA_PERSISTENT_USER_EQ_BANK
            handleA2DPUserEqBankUpdate();
#endif

            if(((AUDIO_PLUGIN_DSP_READY_FOR_DATA_T*)message)->dsp_status == DSP_RUNNING)
            {
                LOGD("DSP ready for data\n");
#ifdef ENABLE_PEER
                /*Request the connected peer device to send its current user EQ settings across if its a peer source.*/
                peerRequestUserEqSetings();
#endif
            }

#ifdef ENABLE_SUBWOOFER
            /* configure the subwoofer type when the dsp is up and running */
            if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_STANDARD)
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_L2CAP, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
            else if(SwatGetMediaType(theSink.rundata->subwoofer.dev_id) == SWAT_MEDIA_LOW_LATENCY)
                AudioConfigureSubWoofer(AUDIO_SUB_WOOFER_ESCO, SwatGetMediaSink(theSink.rundata->subwoofer.dev_id));
#endif
        break;

#ifdef ENABLE_GAIA

        case AUDIO_PLUGIN_DSP_GAIA_EQ_MSG:
        {
            u8 payload[4];
            payload[0] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[0]) >> 8;
            payload[1] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[0]) & 0x00ff;
            payload[2] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[1]) >> 8;
            payload[3] = (((AUDIO_PLUGIN_DSP_GAIA_EQ_MSG_T*)message)->value[1]) & 0x00ff;
            gaia_send_response(GAIA_VENDOR_CSR, GAIA_COMMAND_GET_USER_EQ_PARAMETER, GAIA_STATUS_SUCCESS, 4, payload);
        }
        break;

        case AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG:
        {
            u8 payloadSize = ((AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG_T*)message)->size_value;
            u16 *payload = mallocPanic(payloadSize);
            if (payload)
            {
                memcpy(payload,((AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG_T*)message)->value,payloadSize);
                gaia_send_response_16(GAIA_COMMAND_GET_USER_EQ_GROUP_PARAMETER,
                                  GAIA_STATUS_SUCCESS,
                                  payloadSize,
                                  payload);
                free(payload);
            }
        }
        break;
#endif    /* ENABLE_GAIA */

		case AUDIO_PLUGIN_LATENCY_REPORT:
			handleA2DPLatencyReport(((AUDIO_PLUGIN_LATENCY_REPORT_T *)message)->audio_plugin, ((AUDIO_PLUGIN_LATENCY_REPORT_T *)message)->estimated, ((AUDIO_PLUGIN_LATENCY_REPORT_T *)message)->latency);
		break;

        case AUDIO_PLUGIN_REFRESH_VOLUME:
        {
            LOGD("AUDIO Refresh volume\n");
            /* Refresh the volume and mute status for the routed audio */
            VolumeUpdateRoutedAudioMainAndAuxVolume();
            VolumeApplySoftMuteStates();
        }
        break;

        case AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG:
            PioDrivePio(PIO_AUDIO_ACTIVE,((AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG_T*)message)->signal_detected);
            break;

        default:
            LOGD("AUDIO ? [%x]\n",id);
        break ;
    }
}

#ifdef ENABLE_DISPLAY
/* Handle any display plugin messages */
static void handleDisplayPluginMessage( Task task, MessageId id, Message message )
{
    switch (id)
    {
    case DISPLAY_PLUGIN_INIT_IND:
        {
            DISPLAY_PLUGIN_INIT_IND_T *m = (DISPLAY_PLUGIN_INIT_IND_T *) message;
            LOGD("DISPLAY INIT: %u\n", m->result);
            if (m->result)
            {
                if (powerManagerIsChargerConnected() && (stateManagerGetState() == deviceLimbo) )
                {
                    /* indicate charging if in limbo */
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_CHARGING,  strlen(DISPLAYSTR_CHARGING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
                    displayUpdateVolume(0);
                    displayUpdateBatteryLevel(TRUE);
                }
                else if (stateManagerGetState() != deviceLimbo)
                {
                    /* if this init occurs and not in limbo, turn the display on */
                    displaySetState(TRUE);
                    displayShowText(DISPLAYSTR_HELLO,  strlen(DISPLAYSTR_HELLO), 2, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 10);
                    displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * theSink.features.DefaultVolume)/sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps);
                    /* update battery display */
                    displayUpdateBatteryLevel(FALSE);
                }
            }
        }
        break;

    default:
        LOGD("DISPLAY ? [%x]\n",id);
        break ;
    }
}
#endif /* ENABLE_DISPLAY */

/*************************************************************************
NAME
    app_handler

DESCRIPTION
    This is the main message handler for the Sink Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
static void app_handler(Task task, MessageId id, Message message)
{
/*    LOGD("MSG [%x][%x][%x]\n", (int)task , (int)id , (int)&message);*/

    /* determine the message type based on base and offset */
    if ( ( id >= EVENTS_MESSAGE_BASE ) && ( id < EVENTS_LAST_EVENT ) )
    {
        handleUEMessage(task, id,  message);
    }
#ifdef MESSAGE_EXE_FS_VALIDATION_STATUS
    else if (id == MESSAGE_EXE_FS_VALIDATION_STATUS)
    {
        /* sinkUpgradeMsgHandler(task, id, message); */
    }
#endif
    else  if ( (id >= CL_MESSAGE_BASE) && (id < CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_connection(task, id, message);
    #endif
    }
    else if ( (id >= HFP_MESSAGE_BASE ) && (id < HFP_MESSAGE_TOP) )
    {
        handleHFPMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_hfp(task, id, message);
    #endif
    }
    else if ( (id >= CODEC_MESSAGE_BASE ) && (id < CODEC_MESSAGE_TOP) )
    {
        handleCodecMessage (task, id, message) ;
    }
    else if ( (id >= POWER_MESSAGE_BASE ) && (id < POWER_MESSAGE_TOP) )
    {
        handlePowerMessage (task, id, message) ;
        /* sinkUpgradePowerEventHandler(); */
    }
#ifdef ENABLE_PBAP
    else if ( ((id >= PBAPC_MESSAGE_BASE ) && (id < PBAPC_MESSAGE_TOP)) ||
              ((id >= PBAPC_APP_MSG_BASE ) && (id < PBAPC_APP_MSG_TOP)) )
    {
        handlePbapMessages (task, id, message) ;
    }
#endif
#ifdef ENABLE_MAPC
    else if ( ((id >= MAPC_MESSAGE_BASE )    && (id < MAPC_API_MESSAGE_END)) ||
              ((id >= MAPC_APP_MESSAGE_BASE) && (id < MAPC_APP_MESSAGE_TOP)) )
    {
        handleMapcMessages (task, id, message) ;
    }
#endif
#ifdef ENABLE_AVRCP
    else if ( (id >= AVRCP_INIT_CFM ) && (id < SINK_AVRCP_MESSAGE_TOP) )
    {
        sinkAvrcpHandleMessage (task, id, message) ;
    #ifdef TEST_HARNESS
        vm2host_avrcp(task, id, message);
    #endif
    }
#endif

#ifdef CVC_PRODTEST
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        cvcProductionTestKalimbaMessage (task, id, message);
    }
#endif
    else if ( (id >= A2DP_MESSAGE_BASE ) && (id < A2DP_MESSAGE_TOP) )
    {
        handleA2DPMessage(task, id,  message);
    #ifdef TEST_HARNESS
        vm2host_a2dp(task, id, message);
    #endif
        return;
    }
    else if ( (id >= AUDIO_UPSTREAM_MESSAGE_BASE ) && (id < AUDIO_UPSTREAM_MESSAGE_TOP) )
    {
        handleAudioPluginMessage(task, id,  message);
        return;
    }
    else if( ((id >= MESSAGE_USB_ENUMERATED) && (id <= MESSAGE_USB_SUSPENDED)) ||
             ((id >= MESSAGE_USB_DECONFIGURED) && (id <= MESSAGE_USB_DETACHED)) ||
             ((id >= USB_DEVICE_CLASS_MSG_BASE) && (id < USB_DEVICE_CLASS_MSG_TOP)) )
    {
        handleUsbMessage(task, id, message);
        return;
    }
#ifdef ENABLE_GAIA
    else if ((id >= GAIA_MESSAGE_BASE) && (id < GAIA_MESSAGE_TOP))
    {
        handleGaiaMessage(task, id, message);
        return;
    }
#endif
#ifdef ENABLE_DISPLAY
    else if ( (id >= DISPLAY_UPSTREAM_MESSAGE_BASE ) && (id < DISPLAY_UPSTREAM_MESSAGE_TOP) )
    {
        handleDisplayPluginMessage(task, id,  message);
        return;
    }
#endif   /* ENABLE DISPLAY */
#ifdef ENABLE_SUBWOOFER
    else if ( (id >= SWAT_MESSAGE_BASE) && (id < SWAT_MESSAGE_TOP) )
    {
        handleSwatMessage(task, id, message);
        return;
    }
#endif /* ENABLE_SUBWOOFER */
    else if ( (id >= FM_UPSTREAM_MESSAGE_BASE ) && (id < FM_UPSTREAM_MESSAGE_TOP) )
    {
        sinkFmHandleFmPluginMessage(id, message);
        return;
    }

    else
    {
        LOGD("MSGTYPE ? [%x]\n", id);
    }
}

/* Time critical initialisation */
#ifdef HOSTED_TEST_ENVIRONMENT
void _sink_init(void)
#else
void sink_init(void)
#endif
{
    /* Set the application task */
    theSink.task.handler = app_handler;

    /* set flag to indicate that configuration is being read, use to prevent use of variables
       prior to completion of initialisation */
    theSink.SinkInitialising = TRUE;
    
    /* Read in any PIOs required */
    configManagerPioMap();
    
    /* Assert audio amplifier mute if confugured */
    PioDrivePio(PIO_AMP_MUTE, TRUE);

    /* Time critical USB setup */
    usbTimeCriticalInit();
}

/*************************************************************************
NAME
    sinkConnectionInit

DESCRIPTION
    Initialise the Connection library
*/
static void sinkConnectionInit(void)
{
    /* read the lengths key into a temporary malloc to get pdl length */
    lengths_config_type * lengths_key = PanicUnlessMalloc(sizeof(lengths_config_type));

    /* The number of paired devices can be restricted using pskey user 40,  a number between 1 and 8 is allowed */
    ConfigRetrieve(CONFIG_LENGTHS, lengths_key , sizeof(lengths_config_type) );
    LOGD("PDLSize[%d]\n" , lengths_key->pdl_size );

    /* Initialise the Connection Library with the options */
    ConnectionInitEx2(&theSink.task , NULL, lengths_key->pdl_size );

    /* free the malloc'd memory */
    free(lengths_key);
}


/* The Sink Application starts here...*/
#ifdef HOSTED_TEST_ENVIRONMENT
int sink_main(void)
#else
int main(void)
#endif
{
    LOGD("Main [%s]\n",__TIME__);

    /* check and update as necessary the software version pskey, this is used
       for ensuring maximum compatibility with the sink configuration tool */
    configManagerSetVersionNo();

    /* Initialise memory required early */
    configManagerInitMemory();

    /* initialise memory for the led manager */
    LedManagerMemoryInit();

    /* Initialise device state */
    AuthResetConfirmationFlags();

    /*the internal regs must be latched on (smps and LDO)*/
    PioSetPowerPin ( TRUE ) ;

    switch (BootGetMode() )
    {
#ifdef CVC_PRODTEST
        case BOOTMODE_CVC_PRODTEST:
            /*run the cvc prod test code and dont start the applicaiton */
            cvcProductionTestEnter() ;
        break ;
#endif
        case BOOTMODE_DFU:
            /*do nothing special for the DFU boot mode,
            This mode expects to have the appropriate host interfface enabled
            Don't start the application */

            /* Initializing only the system components required for flashing the led pattern in the DFU mode*/
            configManagerInit(FALSE);
            LEDManagerIndicateEvent(EventUsrEnterDFUMode);
        break ;

        case BOOTMODE_DEFAULT:
        case BOOTMODE_CUSTOM:
        case BOOTMODE_USB_LOW_POWER:
        case BOOTMODE_ALT_FSTAB:
        default:
        {
            /* Initialise the Connection lib */
            sinkConnectionInit();

            #ifdef TEST_HARNESS
                test_init();
            #endif
        }
        break ;
    }

    /* Make sure the mute states are correctly set up */
    VolumeSetInitialMuteState();

    /* Start the message scheduler loop */
    MessageLoop();

    /* Never get here...*/
    return 0;
}



#ifdef DEBUG_MALLOC

#include "vm.h"
void * MallocPANIC ( const char file[], int line , size_t pSize )
{
    static u16 lSize = 0 ;
    static u16 lCalls = 0 ;
    void * lResult;

    lCalls++ ;
    lSize += pSize ;
    printf("+%s,l[%d]c[%d] t[%d] a[%d] s[%d]",file , line ,lCalls, lSize , (u16)VmGetAvailableAllocations(), pSize );

    lResult = malloc ( pSize ) ;

    printf("@[0x%x]\n", (u16)lResult);

        /*and panic if the malloc fails*/
    if ( lResult == NULL )
    {
        printf("MA : !\n") ;
        Panic() ;
    }

    return lResult ;

}

void FreePANIC ( const char file[], int line, void * ptr )
{
    static u16 lCalls = 0 ;
    lCalls++ ;
    printf("-%s,l[%d]c[%d] a[%d] @[0x%x]\n",file , line ,lCalls, (u16)VmGetAvailableAllocations()-1, (u16)ptr);
    /* panic if attempting to free a null pointer*/
    if ( ptr == NULL )
    {
        printf("MF : !\n") ;
        Panic() ;
    }
    free( ptr ) ;
}
#endif

/*************************************************************************
NAME
    sinkInitCodecTask

DESCRIPTION
    Initialises the codec task

RETURNS

*/
static void sinkInitCodecTask ( void )
{
    /* The Connection Library has been successfully initialised,
       initialise the HFP library to instantiate an instance of both
       the HFP and the HSP */

    /*init the codec task*/
    CodecInitCsrInternal (&theSink.rundata->codec, &theSink.task) ;
}


/*************************************************************************
NAME
    handleHFPStatusCFM

DESCRIPTION
    Handles a status response from the HFP and sends an error message if one was received

RETURNS

*/
static void handleHFPStatusCFM ( hfp_lib_status pStatus )
{
    if (pStatus != hfp_success )
    {
        LOGD("HFP CFM Err [%d]\n" , pStatus);
        MessageSend ( &theSink.task , EventSysError , 0 ) ;
#ifdef ENABLE_PBAP
        if(theSink.pbapc_data.pbap_command == pbapc_dialling)
        {
            MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
        }
#endif
    }
    else
    {
         LOGD("HFP CFM Success [%d]\n" , pStatus);
    }

#ifdef ENABLE_PBAP
    theSink.pbapc_data.pbap_command = pbapc_action_idle;
#endif
}

/*************************************************************************
NAME
    IndicateEvent

DESCRIPTION
    Passes the msg Id to the relevant indication informers.

RETURNS None

*/
static void IndicateEvent(MessageId id)
{        
    if (id != EventSysLEDEventComplete)
    {
        LEDManagerIndicateEvent(id);
    }
    
    TonesPlayEvent(id);
    
    ATCommandPlayEvent(id) ;
}



