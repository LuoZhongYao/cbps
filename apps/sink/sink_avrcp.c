#ifdef ENABLE_AVRCP
/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_avrcp.c

DESCRIPTION
    Handles AVRCP control channel functionality. By default it supports AVRCP 1.5 mandatory features, operating as Category 1 Controller
    and Category 2 Target device.  When Peer device support is included, the application will operate as a Category 1 Target also.

    In order to support advanced optional features, additional defines need to be specified:
        - Defining ENABLE_AVRCP_NOW_PLAYING in sink_avrcp.h will enable Now Playing functionality of AVRCP 1.5 specification
        - Defining ENABLE_AVRCP_PLAYER_APP_SETTINGS in sink_avrcp.h will enable Player Application Settings functionality of AVRCP 1.5 specification
        - Browsing is handled separately in sink_avrcp_browsing and enabled by defining ENABLE_AVRCP_BROWSING in project properties
*/

#include "sink_avrcp.h"
#include "sink_avrcp_browsing.h"
#include "sink_avrcp_browsing_channel.h"
#include "sink_avrcp_qualification.h"
#include "sink_debug.h"
#include "sink_statemanager.h"
#include "sink_link_policy.h"
#include "sink_partymode.h"
#include "sink_a2dp.h"
#include "sink_peer.h"
#include "sink_events.h"
#include "sink_led_manager.h"

#ifdef ENABLE_DISPLAY
#include "sink_display.h"
#include "display_plugin_if.h"
#endif

#include <bdaddr.h>
#include <source.h>
#include <string.h>
#include <vm.h>


#ifdef DEBUG_AVRCP
#else
#endif


static bool isAvrcpPlaybackStatusSupported(u16 Index);
static void sinkAvrcpSetActiveConnectionFromIndex(u16 new_index);
static void sinkAvrcpFlushAndResetQueue(u16 index);

/*********************** Const data ***************************/

/* media attributes to retrieve from TG - this is only limited track data that can be filtered down to only return the required information */
const u8 avrcp_retrieve_media_attributes_basic[AVRCP_NUMBER_MEDIA_ATTRIBUTES_BASIC * 4] = {
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_TITLE,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_ARTIST,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_ALBUM};

/* media attributes to retrieve from TG - this is all the track data that can be requested */
const u8 avrcp_retrieve_media_attributes_full[AVRCP_NUMBER_MEDIA_ATTRIBUTES_FULL * 4] = {
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_TITLE,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_ARTIST,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_ALBUM,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_NUMBER,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_TOTAL_NUMBER,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_GENRE,
                                                        0, 0, 0, AVRCP_MEDIA_ATTRIBUTE_PLAYING_TIME};

/* response to get capability request from remote device, event supported in the sink device */
const u8 avrcp_capability_event[AVRCP_MAX_LENGTH_CAPABILITIES] = {
        avrcp_event_volume_changed
    };

#ifdef ENABLE_PEER
static const avrcp_controls avrcp_map_opid_to_ctrl[opid_backward-opid_power+1][2] =
{
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_power        */
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_volume_up    */
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_volume_down  */
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_mute         */
    { AVRCP_CTRL_PLAY_PRESS,     AVRCP_CTRL_PLAY_RELEASE     },    /* opid_play         */
    { AVRCP_CTRL_STOP_PRESS,     AVRCP_CTRL_STOP_RELEASE     },    /* opid_stop         */
    { AVRCP_CTRL_PAUSE_PRESS,    AVRCP_CTRL_PAUSE_RELEASE    },    /* opid_pause        */
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_record       */
    { AVRCP_CTRL_REW_PRESS,      AVRCP_CTRL_REW_RELEASE      },    /* opid_rewind       */
    { AVRCP_CTRL_FF_PRESS,       AVRCP_CTRL_FF_RELEASE       },    /* opid_fast_forward */
    { AVRCP_CTRL_NOP,            AVRCP_CTRL_NOP              },    /* opid_eject        */
    { AVRCP_CTRL_FORWARD_PRESS,  AVRCP_CTRL_FORWARD_RELEASE  },    /* opid_forward      */
    { AVRCP_CTRL_BACKWARD_PRESS, AVRCP_CTRL_BACKWARD_RELEASE }     /* opid_backward     */
};
#endif

/*********************** Helper Functions ***************************/
static bool getA2dpIndexFromAvrcp (u16 avrcpIndex, u16 *a2dpIndex)
{
    if (theSink.a2dp_link_data && theSink.avrcp_link_data && theSink.avrcp_link_data->connected[avrcpIndex])
    {
        for_all_a2dp(*a2dpIndex)
        {
            if (theSink.a2dp_link_data->connected[*a2dpIndex] &&
                BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[*a2dpIndex], &theSink.avrcp_link_data->bd_addr[avrcpIndex]))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    isAvrcpIndexPeer

DESCRIPTION

    Determine if the supplied AVRCP connection index is that of Peer device

**************************************************************************/
static bool isAvrcpIndexPeer (u16 avrcpIndex, u16 *peerIndex)
{
    u16 a2dpIndex;

    if (theSink.a2dp_link_data && theSink.avrcp_link_data && theSink.avrcp_link_data->connected[avrcpIndex])
    {
        for_all_a2dp(a2dpIndex)
        {
            if (theSink.a2dp_link_data->connected[a2dpIndex] &&
                (theSink.a2dp_link_data->peer_device[a2dpIndex] == remote_device_peer) &&
                BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dpIndex], &theSink.avrcp_link_data->bd_addr[avrcpIndex]))
            {
                if (peerIndex)
                {
                    *peerIndex = a2dpIndex;
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    getAvAvrcpIndex

DESCRIPTION

    Obtain index of AVRCP connection to a connected AV Source (AG)

**************************************************************************/
static bool getAvAvrcpIndex (u16 *avIndex)
{
    u16 a2dpIndex;
    u16 avrcp_index;

    if (theSink.a2dp_link_data && theSink.avrcp_link_data)
    {
        for_all_a2dp(a2dpIndex)
        {
            if (theSink.a2dp_link_data->connected[a2dpIndex] && (theSink.a2dp_link_data->peer_device[a2dpIndex] == remote_device_nonpeer))
            {
                for_all_avrcp(avrcp_index)
                {
                    if (theSink.avrcp_link_data->connected[avrcp_index] && (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dpIndex], &theSink.avrcp_link_data->bd_addr[avrcp_index])))
                    {
                        if(avIndex)
                        {
                            *avIndex = avrcp_index;
                        }
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    avrcpGetPeerIndex

DESCRIPTION

    Obtain index of AVRCP connection to a connected Peer device

**************************************************************************/
bool avrcpGetPeerIndex (u16 *peer_index)
{
    u16 a2dp_index;
    u16 avrcp_index;

    if (theSink.a2dp_link_data && theSink.avrcp_link_data)
    {
        for_all_a2dp(a2dp_index)
        {
            if (theSink.a2dp_link_data->connected[a2dp_index] && (theSink.a2dp_link_data->peer_device[a2dp_index] == remote_device_peer))
            {
                for_all_avrcp(avrcp_index)
                {
                    if (theSink.avrcp_link_data->connected[avrcp_index] && (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_index], &theSink.avrcp_link_data->bd_addr[avrcp_index])))
                    {
                        if (peer_index)
                        {
                            *peer_index = avrcp_index;
                        }

                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    getNextConnectedAvrcpIndex

DESCRIPTION
    Returns the next connected AVRCP index if found.

**************************************************************************/
static bool getNextConnectedAvrcpIndex(u16 *IndexToUse , u16 currentIndex)
{
    if (theSink.avrcp_link_data)
    {
        for_all_avrcp(*IndexToUse)
        {
            if ((*IndexToUse != currentIndex) && theSink.avrcp_link_data->connected[*IndexToUse])
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    getValidAvrcpIndex

DESCRIPTION
    Returns the valid AVRCP index if found.

**************************************************************************/

static bool getValidAvrcpIndex(u16 *IndexToUse , u16 currentIndex)
{
    u16 peerIndex;
    bool control_peer_audio = TRUE;

    if (theSink.features.TwsSingleDeviceOperation)
    {
        /* For TWS Single Device Mode it is only valid to send an AVRCP command to the remote peer device
           for A2DP and USB audio */
        control_peer_audio = theSink.remote_peer_audio_conn_status & (A2DP_AUDIO | USB_AUDIO);
    }

    if( theSink.avrcp_link_data->connected[currentIndex] &&
       ((!isAvrcpIndexPeer(currentIndex, &peerIndex)) || control_peer_audio))
    {
        /* Use this index as we have made sure that either it's a true AV device or a remote device with a true Av connected*/
        *IndexToUse = currentIndex;
    }
    else
    {
        /* Find another suitable index and update the active connection */
        if(!getNextConnectedAvrcpIndex(IndexToUse , currentIndex))
        {
            /* Couldn't find next valid index */
            LOGD("AVRCP: Unable to find IndexToUse\n");
            return FALSE;
        }
    }

    LOGD("AVRCP: IndexToUse=%u\n",*IndexToUse);
    return TRUE;
}


/*************************************************************************
NAME
    updatePeerSourceConnectionStatus

DESCRIPTION
    Updates the Audio connection status to the peer device

**************************************************************************/
static void updatePeerSourceConnectionStatus (AudioSrc audio_source, bool is_connected, bdaddr * bd_addr)
{
    u16 peerAvrcpIndex;

    /* If a peer is connected, then send the avrcp vendor dependant command to the peer device updating the audio connection status */
    if(avrcpGetPeerIndex (&peerAvrcpIndex) && theSink.avrcp_link_data->connected[peerAvrcpIndex])
    {
        audio_src_conn_state_t  audioSrcStatus;

        memset(&audioSrcStatus, 0, sizeof(audio_src_conn_state_t));
        audioSrcStatus.src = audio_source;
        audioSrcStatus.isConnected = is_connected;

        if (bd_addr)
        {
            /* Convert the BD Addess from the type bdaddr to byte_aligned_bd_addr_t for sending */
            /* it across to the peer device through the vendor unique AVRCP passthrough command */
            audioSrcStatus.bd_addr.lap[0] = bd_addr->lap & 0xFF;
            audioSrcStatus.bd_addr.lap[1] = (bd_addr->lap >>8)  & 0xFF;
            audioSrcStatus.bd_addr.lap[2] = (bd_addr->lap >>16) & 0xFF;
            audioSrcStatus.bd_addr.uap = bd_addr->uap;
            audioSrcStatus.bd_addr.nap[0] = bd_addr->nap & 0xFF;
            audioSrcStatus.bd_addr.nap[1] = (bd_addr->nap >> 8) & 0xFF ;
        }

        /* Inform remote peer of change to local devices status */
        sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex,
                                                AVRCP_PEER_CMD_AUDIO_CONNECTION_STATUS,
                                                sizeof(audio_src_conn_state_t),
                                                (const u8 *)&audioSrcStatus );
    }
}



/*************************************************************************
NAME
    sinkAvrcpUpdatePeerWirelessSourceConnected

DESCRIPTION
    Informs a remote Peer that a local wireless audio source has connected

**************************************************************************/
void sinkAvrcpUpdatePeerWirelessSourceConnected (AudioSrc audio_source, bdaddr * bd_addr)
{
    updatePeerSourceConnectionStatus( audio_source, TRUE, bd_addr);
}

/*************************************************************************
NAME
    sinkAvrcpUpdatePeerWiredSourceConnected

DESCRIPTION
    Informs a remote Peer that a local wired audio source has connected

**************************************************************************/
void sinkAvrcpUpdatePeerWiredSourceConnected (AudioSrc audio_source)
{
    updatePeerSourceConnectionStatus( audio_source, TRUE, NULL);
}

/*************************************************************************
NAME
    sinkAvrcpUpdatePeerSourceDisconnected

DESCRIPTION
    Informs a remote Peer that a local audio source has disconnected

**************************************************************************/
void sinkAvrcpUpdatePeerSourceDisconnected (AudioSrc audio_source)
{
    updatePeerSourceConnectionStatus( audio_source, FALSE, NULL);
}

/*************************************************************************
NAME
    sinkAvrcpUpdatePeerMute

DESCRIPTION
    Updates mute status on a remote Peer

**************************************************************************/
void sinkAvrcpUpdatePeerMute(bool OnOrOff)
{

    u16 peerAvrcpIndex;

    /* If a peer is connected, then send the avrcp vendor dependant command to the peer device updating the mute status */
    if(avrcpGetPeerIndex (&peerAvrcpIndex) )
    {
        /* Inform remote peer about mute toggle */
        sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex,
                                                AVRCP_PEER_CMD_UPDATE_MUTE,
                                                AVRCP_PAYLOAD_PEER_CMD_UPDATE_MUTE,
                                                (const u8 *)&OnOrOff );
    }
}

/*************************************************************************
NAME
    sinkAvrcpUpdateLedIndicationOnOff

DESCRIPTION
    Informs the remote peer of a change in LED indication state.

**************************************************************************/
void sinkAvrcpUpdateLedIndicationOnOff(bool OnorOff)
{
    u16 peerAvrcpIndex;

    if(avrcpGetPeerIndex (&peerAvrcpIndex) )
    {
        sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex,
                                                AVRCP_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF,
                                                AVRCP_PAYLOAD_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF,
                                                (const u8 *)&OnorOff );
    }
}


#endif

/*************************************************************************
NAME
    avrcpSendControlMessage

DESCRIPTION
    Queue up message to send an AVRCP command based on pending_cmd flag.
    The flag is used so that a response to an AVRCP command must be received
    before another AVRCP command can be sent.

**************************************************************************/
void avrcpSendControlMessage(avrcp_controls control, u16 Index)
{
    if (control != AVRCP_CTRL_NOP)
    {
        if( (control == AVRCP_CTRL_FF_RELEASE)  || (control == AVRCP_CTRL_REW_RELEASE) )
        {
            /* Check the playstatus of the currently active connection before issuing these commands */
            if( ! ((theSink.avrcp_link_data->play_status[Index] & (1<<avrcp_play_status_fwd_seek)) ||
                (theSink.avrcp_link_data->play_status[Index] & (1<<avrcp_play_status_rev_seek) )))
            {
                /*If there is no suitable AVRCP device to process this command then just return */
                if (!isAvrcpPlaybackStatusSupported(Index))
                {
                    theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_error;
                    return;
                }
            }
        }

        {
            MAKE_AVRCP_MESSAGE(AVRCP_CONTROL_SEND);
            message->control = control;
            message->index = Index;
            MessageSendConditionally(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CONTROL_SEND,
                message, &theSink.avrcp_link_data->pending_cmd[Index]);
        }

        if (theSink.avrcp_link_data->cmd_queue_size[Index] < AVRCP_MAX_PENDING_COMMANDS)
            theSink.avrcp_link_data->cmd_queue_size[Index]++;

        LOGD("    AVRCP pending commands %d\n", theSink.avrcp_link_data->cmd_queue_size[Index]);
    }
}


/*************************************************************************
NAME
    sendAvrcpPassthroughCmd

DESCRIPTION
    Sends an AVRCP PASSTHROUGH command. Sets the pending_cmd flag so that the device waits
    for a response before sending another command.

**************************************************************************/
static void sendAvrcpPassthroughCmd(avc_operation_id op_id, u8 state, u16 Index)
{
    theSink.avrcp_link_data->pending_cmd[Index] = TRUE;
    if (theSink.avrcp_link_data->cmd_queue_size[Index])
        theSink.avrcp_link_data->cmd_queue_size[Index]--;
    /* Send a key press */
    AvrcpPassthroughRequest(theSink.avrcp_link_data->avrcp[Index], subunit_panel, 0, state, op_id, 0, 0);
}

/*************************************************************************
NAME
    sendAvrcpGroupCmd

DESCRIPTION
    Sends an AVRCP Group Navigation command. Sets the pending_cmd flag so that the device waits
    for a response before sending another command.

**************************************************************************/
static void sendAvrcpGroupCmd(bool forward, u16 Index, bool state)
{
    theSink.avrcp_link_data->pending_cmd[Index] = TRUE;
    if (theSink.avrcp_link_data->cmd_queue_size[Index])
        theSink.avrcp_link_data->cmd_queue_size[Index]--;
    /* Send appropriate group command */
    if (forward)
        AvrcpNextGroupRequest(theSink.avrcp_link_data->avrcp[Index], state);
    else
        AvrcpPreviousGroupRequest(theSink.avrcp_link_data->avrcp[Index], state);
}

/*************************************************************************
NAME
    sendAvrcpAbortContinuingCmd

DESCRIPTION
    Sends an AVRCP Abort Continuation command. There is no response so this command is just sent directly.

**************************************************************************/
static void sendAvrcpAbortContinuingCmd(u16 Index, u16 pdu_id)
{
    AvrcpAbortContinuingResponseRequest(theSink.avrcp_link_data->avrcp[Index], pdu_id);

    /* An AVRCP fragmented response has been aborted. This will need to be handled if expecting more data to arrive */
}

/*************************************************************************
NAME
    getAvrcpQueueSpace

DESCRIPTION
    Gets the free space in the AVRCP command queue

RETURNS
    Returns the number of AVRCP commands that can be queued.
**************************************************************************/
u16 getAvrcpQueueSpace(u16 Index)
{
    return AVRCP_MAX_PENDING_COMMANDS - theSink.avrcp_link_data->cmd_queue_size[Index];
}

/*************************************************************************
NAME
    isAvrcpCategory1MetadataEnabled

DESCRIPTION
    Checks that the remote device has Category 1 Metadata enabled. This covers the Metadata
    that was introduced in the AVRCP 1.3 specification.

RETURNS
    Returns TRUE if Category 1 Metadata enabled, FALSE otherwise.
**************************************************************************/
static bool isAvrcpCategory1MetadataEnabled(u16 Index)
{
    if ((theSink.avrcp_link_data->features[Index] & AVRCP_CATEGORY_1) && (theSink.avrcp_link_data->extensions[Index] & AVRCP_VERSION_1_3))
        return TRUE;

    return FALSE;
}

/*************************************************************************
NAME
    isAvrcpPlaybackStatusSupported

DESCRIPTION
    Checks that the remote device supports playback status changed notifications.

RETURNS
    Returns TRUE if supported, FALSE otherwise.
**************************************************************************/
static bool isAvrcpPlaybackStatusSupported(u16 Index)
{
    if (theSink.avrcp_link_data->registered_events[Index] & (1 << avrcp_event_playback_status_changed))
        return TRUE;

    return FALSE;
}

/*************************************************************************
NAME
    isAvrcpGroupNavigationEnabled

DESCRIPTION
    Checks that the remote device has Group Navigation enabled.

RETURNS
    Returns TRUE if Group Navigation enabled, FALSE otherwise.
**************************************************************************/
static bool isAvrcpGroupNavigationEnabled(u16 Index)
{
    if ((theSink.avrcp_link_data->features[Index] & AVRCP_GROUP_NAVIGATION) == AVRCP_GROUP_NAVIGATION)
    {
        if(sinkAvrcpBrowsingIsSupported(Index))
        {
            if(sinkAvrcpBrowsingIsGroupNavigationSupported(Index))
                return TRUE;
        }
        else
        {
            return TRUE;
        }
    }
    return FALSE;
}

#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
/*************************************************************************
NAME
    isAvrcpPlayerApplicationSettingsSupported

DESCRIPTION
    Checks that both ends support Player Application Settings.

RETURNS
    Returns TRUE if supported, FALSE otherwise.
**************************************************************************/
static bool isAvrcpPlayerApplicationSettingsSupported(u16 Index)
{

    if ((theSink.avrcp_link_data->features[Index] & AVRCP_PLAYER_APPLICATION_SETTINGS) == AVRCP_PLAYER_APPLICATION_SETTINGS)
        return TRUE;

    return FALSE;
}
#endif


/*********************** AVRCP Local Message Handling ***************************/


/*************************************************************************
NAME
    avrcpControlHandler

DESCRIPTION
    Handles all application created AVRCP messages associated with a certain AVRCP connection.

**************************************************************************/
static void avrcpControlHandler(Task task, MessageId id, Message message)
{
    if (id == AVRCP_CREATE_CONNECTION)
    {
        u16 Index;
        bdaddr addr = ((AVRCP_CREATE_CONNECTION_T *)message)->bd_addr;
        LOGD("AVRCP_CREATE_CONNECTION :\n");
        if (sinkAvrcpGetIndexFromBdaddr(&addr, &Index, FALSE))
        {
            /* reset bdaddr so the connection can be made to this device */
            BdaddrSetZero(&theSink.avrcp_link_data->bd_addr[Index]);
        }
        sinkAvrcpConnect(&addr, DEFAULT_AVRCP_NO_CONNECTION_DELAY);
    }
    else if (id == AVRCP_CONTROL_SEND)
    {
        u16 Index = ((AVRCP_CONTROL_SEND_T *)message)->index;
        Sink sinkAG1,sinkAG2 = NULL;

        /* obtain any sco sinks */
        HfpLinkGetAudioSink(hfp_primary_link, &sinkAG1);
        HfpLinkGetAudioSink(hfp_secondary_link, &sinkAG2);

        /* ignore if not connected */
        if (!theSink.avrcp_link_data->connected[Index])
            return;

        /*Need not do power mode settings if within AVRCP_ACTIVE_MODE_INTERVAL
         assuming user is continuously sending controls */
        if(!(theSink.avrcp_link_data->link_active[Index]))
        {
            /*Check for active AVRCP alone scenarios and change to active mode*/
            if((!sinkAG1 && !sinkAG2)&&
            (A2dpMediaGetState(theSink.a2dp_link_data->device_id[a2dp_primary], theSink.a2dp_link_data->stream_id[a2dp_primary])!= a2dp_stream_streaming)&&
            (A2dpMediaGetState(theSink.a2dp_link_data->device_id[a2dp_secondary], theSink.a2dp_link_data->stream_id[a2dp_secondary])!= a2dp_stream_streaming))
            {
                u16 *message = PanicUnlessNew(u16);

                /* set AVRCP link policy settings */
                linkPolicyUseAvrcpSettings(AvrcpGetSink(theSink.avrcp_link_data->avrcp[Index]));
                theSink.avrcp_link_data->link_active[Index] =  TRUE;
                *message = Index;
                MessageSendLater(&theSink.task, EventSysResetAvrcpMode, message, D_SEC(AVRCP_ACTIVE_MODE_INTERVAL));
            }
        }


        LOGD("AVRCP_CONTROL_SEND :\n");

        switch (((AVRCP_CONTROL_SEND_T *)message)->control)
        {
            /* passthrough commands */
            case AVRCP_CTRL_PAUSE_PRESS:
                LOGD("  Sending Pause Pressed\n");
                sendAvrcpPassthroughCmd(opid_pause, 0, Index);
                break;
            case AVRCP_CTRL_PAUSE_RELEASE:
                LOGD("  Sending Pause Released\n");
                sendAvrcpPassthroughCmd(opid_pause, 1, Index);
                break;
            case AVRCP_CTRL_PLAY_PRESS:
                LOGD("  Sending Play Pressed\n");
                sendAvrcpPassthroughCmd(opid_play, 0, Index);
                break;
            case AVRCP_CTRL_PLAY_RELEASE:
                LOGD("  Sending Play Released\n");
                sendAvrcpPassthroughCmd(opid_play, 1, Index);
                break;
            case AVRCP_CTRL_FORWARD_PRESS:
                LOGD("  Sending Forward Pressed\n");
                sendAvrcpPassthroughCmd(opid_forward, 0, Index);
                break;
            case AVRCP_CTRL_FORWARD_RELEASE:
                LOGD("  Sending Forward Released\n");
                sendAvrcpPassthroughCmd(opid_forward, 1, Index);
                break;
            case AVRCP_CTRL_BACKWARD_PRESS:
                LOGD("  Sending Backward Pressed\n");
                sendAvrcpPassthroughCmd(opid_backward, 0, Index);
                break;
            case AVRCP_CTRL_BACKWARD_RELEASE:
                LOGD("  Sending Backward Released\n");
                sendAvrcpPassthroughCmd(opid_backward, 1, Index);
                break;
            case AVRCP_CTRL_STOP_PRESS:
                LOGD("  Sending Stop Pressed\n");
                sendAvrcpPassthroughCmd(opid_stop, 0, Index);
                break;
            case AVRCP_CTRL_STOP_RELEASE:
                LOGD("  Sending Stop Released\n");
                sendAvrcpPassthroughCmd(opid_stop, 1, Index);
                break;
            case AVRCP_CTRL_FF_PRESS:
                LOGD("  Sending FF Pressed\n");
                sendAvrcpPassthroughCmd(opid_fast_forward, 0, Index);
                break;
            case AVRCP_CTRL_FF_RELEASE:
                LOGD("  Sending FF Released\n");
                sendAvrcpPassthroughCmd(opid_fast_forward, 1, Index);
                break;
            case AVRCP_CTRL_REW_PRESS:
                LOGD("  Sending REW Pressed\n");
                sendAvrcpPassthroughCmd(opid_rewind, 0, Index);
                break;
            case AVRCP_CTRL_REW_RELEASE:
                LOGD("  Sending REW Released\n");
                sendAvrcpPassthroughCmd(opid_rewind, 1, Index);
                break;
            case AVRCP_CTRL_NEXT_GROUP_PRESS:
                LOGD("  Sending Next Group Press\n");
                sendAvrcpGroupCmd(TRUE, Index, 0);
                break;
            case AVRCP_CTRL_NEXT_GROUP_RELEASE:
                LOGD("  Sending Next Group Release\n");
                sendAvrcpGroupCmd(TRUE, Index, 1);
                break;
            case AVRCP_CTRL_PREVIOUS_GROUP_PRESS:
                LOGD("  Sending Previous Group Press\n");
                sendAvrcpGroupCmd(FALSE, Index, 0);
                break;
            case AVRCP_CTRL_PREVIOUS_GROUP_RELEASE:
                LOGD("  Sending Previous Group Release\n");
                sendAvrcpGroupCmd(FALSE, Index, 1);
                break;
#ifdef ENABLE_PEER
            case AVRCP_CTRL_VOLUME_UP_PRESS:
                LOGD("  Sending Volume Up Pressed\n");
                sendAvrcpPassthroughCmd(opid_volume_up, 0, Index);
                break;
            case AVRCP_CTRL_VOLUME_UP_RELEASE:
                LOGD("  Sending Volume Up Released\n");
                sendAvrcpPassthroughCmd(opid_volume_up, 1, Index);
                break;
            case AVRCP_CTRL_VOLUME_DOWN_PRESS:
                LOGD("  Sending Volume Down Pressed\n");
                sendAvrcpPassthroughCmd(opid_volume_down, 0, Index);
                break;
            case AVRCP_CTRL_VOLUME_DOWN_RELEASE:
                LOGD("  Sending Volume Down Released\n");
                sendAvrcpPassthroughCmd(opid_volume_down, 1, Index);
                break;
             case AVRCP_CTRL_POWER_OFF:
                LOGD("  Sending Power Off\n");
                sendAvrcpPassthroughCmd(opid_power, 1, Index);
                break;
#endif
            case AVRCP_CTRL_ABORT_CONTINUING_RESPONSE:
                LOGD("  Sending Abort Continuing Response\n");
                sendAvrcpAbortContinuingCmd(Index, ((AVRCP_CTRL_ABORT_CONTINUING_RESPONSE_T *)message)->pdu_id);
                break;
            default:
                break;
        }
    }
}


/*********************** AVRCP Library Message Handling Functions ***************************/

/******************/
/* INITIALISATION */
/******************/

/*************************************************************************
NAME
    sinkAvrcpInitComplete

DESCRIPTION
    Functionality as a result of receiving AVRCP_INIT_CFM from the AVRCP library.

**************************************************************************/
static void sinkAvrcpInitComplete(const AVRCP_INIT_CFM_T *msg)
{
    /* check for successful initialisation of AVRCP libraray */
    if(msg->status == avrcp_success)
    {
        LOGD("  AVRCP Init Success [SDP handle 0x%lx]\n", msg->sdp_handle);
    }
    else
    {
        LOGD("    AVRCP Init Failed [Status %d]\n", msg->status);
        Panic();
    }
}

/******************************/
/* CONNECTION / DISCONNECTION */
/******************************/

/*************************************************************************
NAME
    handleAvrcpConnectCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_CONNECT_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpConnectCfm(AVRCP_CONNECT_CFM_T *msg)
{
    u16 avrcpIndex = 0;
    u16 a2dpIndex = 0;

#ifdef ENABLE_PEER
    u16 avAvrcpIndex = 0;
    u16 peerAvrcpIndex = 0;
#endif

    if(msg->status == avrcp_success)
    {
        AVRCP_DEBUG(("  AVRCP Connect Success [avrcp 0x%x][bdaddr %x:%x:%lx][sink 0x%x]\n",
                     (u16)msg->avrcp,
                     msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap,
                     (u16)msg->sink));

        /* If the device is off then disconnect */
        if (stateManagerGetState() == deviceLimbo)
        {
               AvrcpDisconnectRequest(msg->avrcp);
        }
        else
        {
            u16 a2dp_volume;

            /* Ensure the underlying ACL is encrypted */
            ConnectionSmEncrypt(&theSink.task , msg->sink , TRUE);

            /* Get the supported profile extensions */
            AvrcpGetProfileExtensions(msg->avrcp);

            /* find the instance to use for this connection */
            if (!sinkAvrcpGetIndexFromBdaddr(&msg->bd_addr, &avrcpIndex, FALSE))
            {
                /* address not in the list
                 * NB we don't expect this to happen because address will have
                 * been set in sinkAvrcpConnect()
                 */
                u16 i = 0;
                LOGD("  can't find bdaddr\n");
                for_all_avrcp(i)
                {
                    if (!theSink.avrcp_link_data->connected[i])
                    {
                        LOGD("  found free %d\n", i);
                        break;
                    }
                }
                avrcpIndex = i; /* replace the disconnected device */
            }

            LOGD("  found index %d\n", avrcpIndex);

            if ((avrcpIndex < MAX_AVRCP_CONNECTIONS) && !theSink.avrcp_link_data->connected[avrcpIndex])
            {
                theSink.avrcp_link_data->connected[avrcpIndex] = TRUE;
                theSink.avrcp_link_data->bd_addr[avrcpIndex] = msg->bd_addr;
                theSink.avrcp_link_data->play_status[avrcpIndex] = avrcp_play_status_stopped;
                theSink.avrcp_link_data->avrcp[avrcpIndex] = msg->avrcp;

                sinkAvrcpUpdateActiveConnection();

                if (!getA2dpVolume(&theSink.avrcp_link_data->bd_addr[avrcpIndex], &a2dp_volume))
                    a2dp_volume = ((sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps * theSink.features.DefaultA2dpVolLevel)/ VOLUME_NUM_VOICE_STEPS);

                sinkAvrcpSetLocalVolume(avrcpIndex, a2dp_volume);

                /* set play status depending on streaming state */
                for_all_a2dp(a2dpIndex)
                {
                    if (theSink.a2dp_link_data->connected[a2dpIndex] && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dpIndex], &theSink.avrcp_link_data->bd_addr[avrcpIndex]))
                    {
                        theSink.a2dp_link_data->avrcp_support[a2dpIndex] = avrcp_support_supported;
                        /* check whether the a2dp connection is streaming data */
                        if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[a2dpIndex], theSink.a2dp_link_data->stream_id[a2dpIndex]) == a2dp_stream_streaming)
                        {
                            sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[a2dpIndex], avrcp_play_status_playing);
                        }
#ifdef ENABLE_PEER
                        if (theSink.a2dp_link_data->peer_device[a2dpIndex] == remote_device_peer)
                        {
                            sinkA2dpHandlePeerAvrcpConnectCfm(a2dpIndex, TRUE);
                        }
#endif
                        break;
                    }
                }

#ifdef ENABLE_PEER
                /* Find if there is a true AV device (non peer) device connected, if yes then send the notification to the peer*/
                if(avrcpGetPeerIndex (&peerAvrcpIndex))
                {
                    if(getAvAvrcpIndex(&avAvrcpIndex))
                    {
                        sinkAvrcpUpdatePeerWirelessSourceConnected(A2DP_AUDIO, &msg->bd_addr);
                    }
                    /* Check if any USB audio source is connected and notify the peer*/
                    if(usbIsAttached())
                    {
                        /*If the USB source has connected then notify this to the peer device */
                        sinkAvrcpUpdatePeerWiredSourceConnected(USB_AUDIO);
                    }
                    /*Check if any Analog audio source is connected and notify the peer*/
                    if(analogAudioConnected())
                    {
                        /*If the Analogue wired audio has connected then notify this to the peer device */
                        sinkAvrcpUpdatePeerWiredSourceConnected(ANALOG_AUDIO);
                    }
                }
#endif

            }
        }
    }
    else
    {
        LOGD("  AVRCP Connect Fail [status %d]\n", msg->status);

        if (sinkAvrcpGetIndexFromBdaddr(&msg->bd_addr, &avrcpIndex, FALSE))
        {
            BdaddrSetZero(&theSink.avrcp_link_data->bd_addr[avrcpIndex]);
        }

        for_all_a2dp(a2dpIndex)
        {
            /* test to see if A2DP connection exists so that AVRCP status can be updated */
            if (theSink.a2dp_link_data->connected[a2dpIndex] && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dpIndex], &msg->bd_addr))
            {
                if (theSink.a2dp_link_data->avrcp_support[a2dpIndex] == avrcp_support_unknown)
                {
                    /* try AVRCP connection again */
                    theSink.a2dp_link_data->avrcp_support[a2dpIndex] = avrcp_support_second_attempt;
                    sinkAvrcpConnect(&msg->bd_addr, DEFAULT_AVRCP_2ND_CONNECTION_DELAY);
                    LOGD("      AVRCP 2nd attempt\n");
                    return;
                }
                else
                {
                    /* give up on connecting AVRCP */
                    theSink.a2dp_link_data->avrcp_support[a2dpIndex] = avrcp_support_unsupported;
                    LOGD("      AVRCP unsupported\n");
                }

#ifdef ENABLE_PEER
                if (theSink.a2dp_link_data->peer_device[a2dpIndex] == remote_device_peer)
                {
                    sinkA2dpHandlePeerAvrcpConnectCfm(a2dpIndex, FALSE);
                }
#endif
                break;
            }
        }
    }

    if (theSink.avrcp_link_data->avrcp_manual_connect && BdaddrIsSame(&theSink.avrcp_link_data->avrcp_play_addr, &msg->bd_addr))
    {
        if (msg->status == avrcp_success)
        {
            LOGD("AVRCP manual connect - send play\n");
            sinkAvrcpPlay();
        }
        theSink.avrcp_link_data->avrcp_manual_connect = FALSE;
        BdaddrSetZero(&theSink.avrcp_link_data->avrcp_play_addr);
    }
}

/*************************************************************************
NAME
    handleAvrcpConnectInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_CONNECT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpConnectInd(AVRCP_CONNECT_IND_T *msg)
{
    u8 i = 0;
    bool accept = FALSE;

    for_all_avrcp(i)
    {
        LOGD("  index %d\n", i);
        if (!BdaddrIsZero(&theSink.avrcp_link_data->bd_addr[i]))
        {
            AVRCP_DEBUG(("  bdaddr not zero [bdaddr inst %x:%x:%lx] [bdaddr inc %x:%x:%lx]\n",
                         theSink.avrcp_link_data->bd_addr[i].nap,
                         theSink.avrcp_link_data->bd_addr[i].uap,
                         theSink.avrcp_link_data->bd_addr[i].lap,
                         msg->bd_addr.nap,
                         msg->bd_addr.uap,
                         msg->bd_addr.lap));
            if (BdaddrIsSame(&theSink.avrcp_link_data->bd_addr[i], &msg->bd_addr))
            {
                LOGD("  bdaddr same\n");
                if (!theSink.avrcp_link_data->connected[i])
                {
                    LOGD("  not connected\n");
                    /* if an AVRCP connection has been queued, remove our attempt from the queue and accept this connection */
                    accept = TRUE;
                    MessageCancelAll(&theSink.avrcp_link_data->avrcp_ctrl_handler[i], AVRCP_CREATE_CONNECTION);
                    break;
                }
            }
        }
        else
        {
            LOGD("  bdaddr zero\n");
            /* there is space for a new connection so accept it and store device address */
            accept = TRUE;
            theSink.avrcp_link_data->bd_addr[i] = msg->bd_addr;
            break;
        }
    }

    LOGD("  AVRCP Connect Response [accept %d]\n", accept);

    AvrcpConnectResponse(&theSink.task, msg->connection_id, msg->signal_id, accept);
}


/*************************************************************************
NAME
    resetAvrcpConnection

DESCRIPTION
    Set AVRCP instance back to disconnected state

**************************************************************************/
static void resetAvrcpConnection(u16 Index)
{
    /* delete all messages for this connection */
    sinkAvrcpFlushAndResetQueue(Index);

    theSink.avrcp_link_data->connected[Index] = FALSE;
    BdaddrSetZero(&theSink.avrcp_link_data->bd_addr[Index]);
    theSink.avrcp_link_data->registered_events[Index] = 0;
    theSink.avrcp_link_data->features[Index] = 0;
    theSink.avrcp_link_data->event_capabilities[Index] = 0;
    theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_stopped;
}


/*************************************************************************
NAME
    handleAvrcpDisconnectInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_DISCONNECT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpDisconnectInd(AVRCP_DISCONNECT_IND_T *msg)
{
    u16 Index;

#ifdef ENABLE_PEER
    u16 peerAvrcpIndex;
#endif
    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
#ifdef ENABLE_PEER
        /* Find if there is a true AV device (non peer) device connected, if yes then send the notification to the peer*/
        if(theSink.a2dp_link_data)
        {
            if (isAvrcpIndexPeer(Index, &peerAvrcpIndex))
            {
                 theSink.remote_peer_audio_conn_status = 0;
            }
        }
#endif

        sinkAvrcpBrowsingChannelDisconnectRequest(theSink.avrcp_link_data->avrcp[Index]);
        if (sinkAvrcpBrowsingChannelIsDisconnected(Index))
        {
            theSink.avrcp_link_data->avrcp[Index] = NULL;
        }

        resetAvrcpConnection(Index);

        sinkAvrcpUpdateActiveConnection();
    }
}

/*****************/
/* AV/C COMMANDS */
/*****************/

/*************************************************************************
NAME
    handleAvrcpPassthroughCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_PASSTHROUGH_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpPassthroughCfm(AVRCP *avrcp)
{
    /*
        Clearing the pending flag should allow another
        pending event to be delivered to controls_handler
    */
    u16 Index;

    if (sinkAvrcpGetIndexFromInstance(avrcp, &Index))
    {
        theSink.avrcp_link_data->pending_cmd[Index] = FALSE;

        if (theSink.avrcp_link_data->vendor_data[Index])
        {   /* Data has been sent, free memory holding it */
            free(theSink.avrcp_link_data->vendor_data[Index]);
            theSink.avrcp_link_data->vendor_data[Index] = NULL;
        }
    }
}

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    handleAvrcpVendorUniquePassthroughInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_PASSTHROUGH_IND from the AVRCP library
    with an opid_vendor_unique operation id.

**************************************************************************/
static bool handleAvrcpVendorUniquePassthroughInd (AVRCP_PASSTHROUGH_IND_T *ind)
{
    LOGD("handleAvrcpVendorUniquePassthroughInd\n");

    if (ind->size_op_data >= VENDOR_CMD_HDR_SIZE)
    {
        u16 avrcpIndex;
        u16 peerIndex;
        u16 cmd_id = (ind->op_data[0] << 8) + ind->op_data[1];

        switch (cmd_id)
        {
        case AVRCP_PEER_CMD_NOP:
            LOGD("AVRCP_PEER_CMD_NOP\n");
            AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
            return TRUE;
            break;

        case AVRCP_PEER_CMD_PEER_STATUS_CHANGE:
            LOGD("AVRCP_PEER_CMD_PEER_STATUS_CHANGE\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_PEER_STATUS_CHANGE))
            {
                if (sinkAvrcpGetIndexFromInstance(ind->avrcp, &avrcpIndex) && isAvrcpIndexPeer(avrcpIndex, &peerIndex))
                {
                    peerHandleStatusChangeCmd( (PeerStatusChange)ind->op_data[2] );
                    LOGD("   peer status change = 0x%X\n",ind->op_data[2]);
                }

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_TWS_AUDIO_ROUTING:
            LOGD("AVRCP_PEER_CMD_TWS_AUDIO_ROUTING\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_TWS_AUDIO_ROUTING))
            {
                if (sinkAvrcpGetIndexFromInstance(ind->avrcp, &avrcpIndex) && isAvrcpIndexPeer(avrcpIndex, &peerIndex))
                {
                    peerHandleAudioRoutingCmd( (PeerTwsAudioRouting)ind->op_data[2], (PeerTwsAudioRouting)ind->op_data[3] );
                    LOGD("   peer audio routing = 0x%X 0x%X\n",ind->op_data[2],ind->op_data[3]);
                }

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_TWS_VOLUME:
            LOGD("AVRCP_PEER_CMD_TWS_VOLUME\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_TWS_VOLUME))
            {
                if (sinkAvrcpGetIndexFromInstance(ind->avrcp, &avrcpIndex) && isAvrcpIndexPeer(avrcpIndex, &peerIndex))
                {
                    peerHandleVolumeCmd( ind->op_data[2] );
                    LOGD("   peer volume = %u\n",ind->op_data[2]);
                }

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_AUDIO_CONNECTION_STATUS:
            LOGD("AVRCP_PEER_CMD_AUDIO_CONNECTION_STATUS\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_AUDIO_CONNECTION_STATUS))
            {
                audio_src_conn_state_t new_state;

                memcpy(&new_state , &ind->op_data[2] , AVRCP_PAYLOAD_PEER_CMD_AUDIO_CONNECTION_STATUS);

                if(new_state.isConnected)
                {
                    theSink.remote_peer_audio_conn_status |= new_state.src;

                    if(new_state.src == A2DP_AUDIO)
                    {
                        /*Pack the received BD address back into the bdaddr format defined in bdaddr_.h */
                        theSink.remote_peer_ag_bd_addr->lap = ((u32)new_state.bd_addr.lap[2] << 16) | ((u16)new_state.bd_addr.lap[1] << 8) | new_state.bd_addr.lap[0];
                        theSink.remote_peer_ag_bd_addr->uap = new_state.bd_addr.uap;
                        theSink.remote_peer_ag_bd_addr->nap = (new_state.bd_addr.nap[1] << 8) | new_state.bd_addr.nap[0] ;

                        peerHandleRemoteAgConnected();
                    }
                }
                else
                {
                    theSink.remote_peer_audio_conn_status &= ~(new_state.src) ;

                    if(new_state.src == A2DP_AUDIO)
                    {
                        BdaddrSetZero(theSink.remote_peer_ag_bd_addr);
                    }

                    /* Clear and restart the auto switchoff timer */
                    if (theSink.conf1->timeouts.AutoSwitchOffTime_s)
                    {
                        LOGD("AVRCP : reset auto off\n");
                        sinkSendLater(EventSysAutoSwitchOff, D_SEC(theSink.conf1->timeouts.AutoSwitchOffTime_s));
                    }
                }

                AvrcpVendorDependentResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_UPDATE_AUDIO_ENHANCEMENT_SETTINGS:
            LOGD("AVRCP_PEER_CMD_AUDIO_ENHANCEMENT_SETTINGS\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_AUDIO_ENHANCEMENT_SETTINGS))
            {
                u16 audioEnhancement = (ind->op_data[3] << 8) | ind->op_data[2];

                if(theSink.a2dp_link_data)
                {
                    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing =
                    (A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0 + (audioEnhancement & A2DP_MUSIC_CONFIG_USER_EQ_SELECT));

                    theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements = (audioEnhancement & ~A2DP_MUSIC_CONFIG_USER_EQ_SELECT);

                    LOGD("   music_mode_enhancements = 0x%X \n",theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements);

                     /* Send the message to DSP to use these audio enhancement parameters */
                    AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
                }

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

    case AVRCP_PEER_CMD_UPDATE_TRIM_VOLUME:
        LOGD("AVRCP_PEER_CMD_UPDATE_TRIM_VOLUME\n");
        if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_TRIM_VOLUME))
        {
            if (sinkAvrcpGetIndexFromInstance(ind->avrcp, &avrcpIndex) && isAvrcpIndexPeer(avrcpIndex, &peerIndex))
            {
                peerUpdateTrimVolume((PeerTrimVolChangeCmd) ind->op_data[2] );
            }

            AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
            return TRUE;
        }
        break;

        case AVRCP_PEER_CMD_UPDATE_USER_EQ_SETTINGS:
            LOGD("AVRCP_PEER_CMD_UPDATE_USER_EQ_SETTINGS\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_USER_EQ_SETTINGS))
            {
                if(theSink.PEQ)
                {
                    u16 i;
                    u8 *user_eq_params = &ind->op_data[VENDOR_CMD_HDR_SIZE];
                    theSink.PEQ->preGain = MAKEWORD(user_eq_params[PRE_GAIN_LO_OFFSET] , user_eq_params[PRE_GAIN_HI_OFFSET]);

                    /*Increment the data pointer with the no. of bytes required to store the Pre-gain value */
                    user_eq_params += USER_EQ_PARAM_PRE_GAIN_SIZE;

                    for(i=0; i<NUM_USER_EQ_BANDS; i++)
                    {
                        theSink.PEQ->bands[i].filter = user_eq_params[BAND_FILTER_OFFSET];
                        theSink.PEQ->bands[i].freq = MAKEWORD(user_eq_params[BAND_FREQ_LO_OFFSET], user_eq_params[BAND_FREQ_HI_OFFSET]);
                        theSink.PEQ->bands[i].gain = MAKEWORD(user_eq_params[BAND_GAIN_LO_OFFSET], user_eq_params[BAND_GAIN_HI_OFFSET]);
                        theSink.PEQ->bands[i].Q = MAKEWORD(user_eq_params[BAND_Q_LO_OFFSET], user_eq_params[BAND_Q_HI_OFFSET]);

                        /*Increment the data pointer with the no. of bytes required to store all the band parameters */
                        user_eq_params += USER_EQ_BAND_PARAMS_SIZE ;
                    }

                    handleA2DPUserEqBankUpdate();
                }

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_REQUEST_USER_EQ_SETTINGS:
            LOGD("AVRCP_PEER_CMD_REQUEST_USER_EQ_SETTINGS\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_REQUEST_USER_EQ_SETTINGS))
            {
                /*This pull command is sent by a TWS peer device acting as a sink, to receive the current user EQ settings.
                    So just send the AVRCP_PEER_CMD_UPDATE_USER_EQ_SETTINGS with curent EQ settings as a response. */
                peerSendUserEqSettings();

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;
#ifdef ENABLE_PEER
        case AVRCP_PEER_CMD_UPDATE_MUTE:
            LOGD("AVRCP_PEER_CMD_TOGGLE_MUTE\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_MUTE))
            {
                VolumeUpdateMuteState(audio_mute_group_main, (bool)ind->op_data[2]);

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;

        case AVRCP_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF:
            LOGD("AVRCP_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF))
            {
                if(ind->op_data[2])
                {
                    LedManagerEnableLEDS();
                }
                else
                {
                    LedManagerDisableLEDS();
                }

                configManagerWriteSessionData();

                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;


        case AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL:
            LOGD("AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL\n");
            if (ind->size_op_data == VENDOR_CMD_TOTAL_SIZE(AVRCP_PAYLOAD_PEER_CMD_UPDATE_BATTERY_LEVEL))
            {
                if(!peerUpdateBatteryLevel((ind->op_data[2] << 8) | ind->op_data[3]))
                {
                    LOGD("AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL Peer battery not supported\n");
                }
                AvrcpPassthroughResponse(ind->avrcp, avctp_response_accepted);
                return TRUE;
            }
            break;
#endif

        default:
            break;
        }
    }

    return FALSE;
}
#endif

/*************************************************************************
NAME
    handleAvrcpPassthroughInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_PASSTHROUGH_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpPassthroughInd(AVRCP_PASSTHROUGH_IND_T *msg)
{
    u16 avrcpIndex;
    u16 a2dpIndex;

    /* Acknowledge the request */
    if ((msg->opid == opid_volume_up) || (msg->opid == opid_volume_down))
    {
        LOGD("Recieved %d passthrough command\n", msg->opid);
        /* The device should accept volume up commands as it supports AVRCP TG category 2. */
        AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);

        /* Adjust the local volume only if it is a press command and the A2DP is streaming */
        if (!msg->state && sinkAvrcpGetIndexFromInstance(msg->avrcp, &avrcpIndex))
        {
            for_all_a2dp(a2dpIndex)
            {
                if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dpIndex], &theSink.avrcp_link_data->bd_addr[avrcpIndex]))
                {
#ifdef ENABLE_PEER
                    if (theSink.a2dp_link_data->peer_device[a2dpIndex] == remote_device_peer)
                    {   /* Volume change request from a Peer - we could have any type of source */
                        if (msg->opid == opid_volume_up)
                        {
                            LOGD("   received vol up from Peer\n");
                            VolumeModifyAndUpdateRoutedAudioMainVolume(increase_volume);
                        }
                        else
                        {
                            LOGD("   received vol down from Peer\n");
                            VolumeModifyAndUpdateRoutedAudioMainVolume(decrease_volume);
                        }
                    }
                    else
#endif
                    {
                        /* Volume change request from an A2DP source */
                        if(theSink.routed_audio && (theSink.routed_audio == A2dpMediaGetSink(theSink.a2dp_link_data->device_id[a2dpIndex], theSink.a2dp_link_data->stream_id[a2dpIndex])))
                        {
                            if(msg->opid == opid_volume_up)
                            {
                                LOGD("   received vol up\n");
                                VolumeModifyAndUpdateRoutedA2DPAudioMainVolume(increase_volume);
                            }
                            else
                            {
                                LOGD("   received vol down\n");
                                VolumeModifyAndUpdateRoutedA2DPAudioMainVolume(decrease_volume);
                            }
                        }
                    }
                }
            }
        }
    }
#ifdef ENABLE_PEER
    else if (msg->opid == opid_vendor_unique)
    {
        if (!handleAvrcpVendorUniquePassthroughInd(msg))
        {   /* An invalid vendor unique command has been received */
            LOGD("   received invalid vendor unique cmd\n");
            AvrcpPassthroughResponse(msg->avrcp, avctp_response_not_implemented);
        }
    }
    else
    {
        u16 peerIndex;

        if (!sinkAvrcpGetIndexFromInstance(msg->avrcp, &avrcpIndex) ||
            !isAvrcpIndexPeer(avrcpIndex, &peerIndex) ||
            (msg->opid < opid_power) ||
            (msg->opid > opid_backward))
        {   /* Only handle AVRCP passthrough commands, in the expected range, if received from Peer */
            LOGD("   received invalid passthrough [%d]\n", msg->opid);
            AvrcpPassthroughResponse(msg->avrcp, avctp_response_not_implemented);
        }
        else if((msg->opid == opid_power) && getA2dpIndexFromAvrcp (avrcpIndex, &a2dpIndex) && theSink.features.TwsSingleDeviceOperation)
        {
             /*We have received a power off command from the peer device so just turn off the device*/
            LOGD(" received power off passthrough command");

             /* The device should accept these commands as it supports AVRCP TG category 1. */
            AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);

            theSink.a2dp_link_data->local_peer_status[a2dpIndex] |= PEER_STATUS_POWER_OFF;
            MessageSend(&theSink.task, EventUsrPowerOff, 0);
        }
        else
        {   /* Passthrough operation is for a Peer and in the range we forward to AV Source */
            u8 opid = msg->opid - opid_power;
            u8 state = !!msg->state;
            u16 avIndex;

            /* The device should accept these commands as it supports AVRCP TG category 1. */
            AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);

            if (getA2dpIndexFromAvrcp (avrcpIndex, &a2dpIndex) &&
                ((theSink.features.TwsSingleDeviceOperation && (theSink.a2dp_link_data->peer_features[a2dpIndex] & remote_features_tws_a2dp_sink)) ||
                 (theSink.features.ShareMePeerControlsSource && (theSink.a2dp_link_data->peer_features[a2dpIndex] & remote_features_shareme_a2dp_sink))))
            {   /* Received AVRCP commands routed to AV Source */
                if (getAvAvrcpIndex(&avIndex))
                {   /* True AV Source exists to receive command */
                    LOGD("   relaying passthrough to AV [%d][%d]\n", msg->opid,state);
                    avrcpSendControlMessage(avrcp_map_opid_to_ctrl[opid][state], avIndex);
                }
            }
            else
            {   /* AVRCP command routing not enabled, so handle ones that affect relay stream state locally */
                if (msg->opid == opid_play)
                {   /* Mark relay as availiable */
                    peerHandleStatusChangeCmd( PEER_STATUS_CHANGE_RELAY_AVAILABLE );
                }
                else if ((msg->opid == opid_pause) || (msg->opid == opid_stop))
                {   /* Mark relay as unavailiable */
                    peerHandleStatusChangeCmd( PEER_STATUS_CHANGE_RELAY_UNAVAILABLE );
                }
            }
        }
    }
#else
    else
    {
        LOGD("   received invalid passthrough [%d]\n", msg->opid);
        /* The device won't accept any other commands. */
        AvrcpPassthroughResponse(msg->avrcp, avctp_response_not_implemented);
    }
#endif
}

/*************************************************************************
NAME
    handleAvrcpUnitInfoInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_UNITINFO_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpUnitInfoInd(AVRCP_UNITINFO_IND_T *msg)
{
    /* Device is a CT and TG, so send the correct response to UnitInfo requests. */
    u32 company_id = 0xffffff; /* IEEE RAC company ID can be used here */
    AvrcpUnitInfoResponse(msg->avrcp, TRUE, subunit_panel, 0, company_id);
}

/*************************************************************************
NAME
    handleAvrcpSubUnitInfoInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_SUBUNITINFO_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpSubUnitInfoInd(AVRCP_SUBUNITINFO_IND_T *msg)
{
    /* Device is a CT and TG, so send the correct response to SubUnitInfo requests. */
    u8 page_data[4];
    page_data[0] = 0x48; /* subunit_type: panel; max_subunit_ID: 0 */
    page_data[1] = 0xff;
    page_data[2] = 0xff;
    page_data[3] = 0xff;
    AvrcpSubUnitInfoResponse(msg->avrcp, TRUE, page_data);
}

/*************************************************************************
NAME
    handleSinkAvrcpVendorUniquePassthroughReq

DESCRIPTION
    Functionality as a result of receiving SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ from the application.

**************************************************************************/
static void handleSinkAvrcpVendorUniquePassthroughReq (SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ_T *req)
{
    sinkAvrcpVendorUniquePassthroughRequest( req->avrcp_index, req->cmd_id, req->size_data, req->data);
}

/*************************************************************************
NAME
    handleAvrcpVendorDependentInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_VENDORDEPENDENT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpVendorDependentInd(AVRCP_VENDORDEPENDENT_IND_T *msg)
{
    /*
        Reject all vendor requests.
    */
    AvrcpVendorDependentResponse(msg->avrcp, avctp_response_not_implemented);
}

/******************/
/* AVRCP Metadata */
/******************/
/*************************************************************************
NAME
    handleGetCapsInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_CAPS_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetCapsInd(AVRCP_GET_CAPS_IND_T *msg)
{
    LOGD("Request for AVRCP capability id %d\n", msg->caps);

    switch (msg->caps)
    {
        case avrcp_capability_company_id:
            /* FALLTHROUGH */

        case avrcp_capability_event_supported:
            /* In all case we just want to respond with only the mandatory
               capabilities, which will be inserted by the AVRCP library. */
            AvrcpGetCapsResponse(msg->avrcp, avctp_response_stable, msg->caps, 0, 0);
            break;

        default:
            /* This should never happen, doing nothing here will cause the library
               to correctly return an error. */
            LOGD("AVRCP capability id not handled\n");
            AvrcpGetCapsResponse(msg->avrcp, avrcp_response_rejected_invalid_param, msg->caps, 0, 0);
            break;
    }
}

/*************************************************************************
NAME
    handleAvrcpGetCapsCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_CAPS_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetCapsCfm(AVRCP_GET_CAPS_CFM_T *msg)
{
    u16 Index;
    u16 caps_returned = 0;
    const u8 *lSource = SourceMap(msg->caps_list);
    u16 size_source = SourceSize(msg->caps_list);

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        caps_returned = (msg->size_caps_list < msg->number_of_caps) ? msg->size_caps_list : msg->number_of_caps;
        caps_returned = (size_source < caps_returned) ? size_source : caps_returned;

        if (lSource)
        {
            LOGD("   success; packet[%d] caps[%d] num_caps[%d] size_caps[%d] size_source[%d]\n", msg->metadata_packet_type, msg->caps, msg->number_of_caps, msg->size_caps_list, size_source);

#ifdef DEBUG_AVRCP
            {
                u16 i;
                for (i = 0; i < caps_returned; i++)
                {
                    LOGD("        capability[%d] = 0x%x\n", i, lSource[i]);
                }
            }
#endif
            if (msg->caps == avrcp_capability_event_supported)
            {
                u16 j;
                /* now know what optional events are supported by the TG so store these for future reference */
                for (j = 0; j < caps_returned; j++)
                {
                    theSink.avrcp_link_data->event_capabilities[Index] |= (1 << lSource[j]);
                }
            }
            /* else if (msg->caps == avrcp_capability_company_id)
               {

               }
            */
        }
        else
        {
            LOGD("   fail; no source\n");
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }

    /* finished processing the source */
    AvrcpSourceProcessed(msg->avrcp);

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        /* Get notified of change in playback status */
        AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_status_changed, 0);

        /* other optional notifications can be registered here as it is known what events are supported by the remote device */

        if (theSink.avrcp_link_data->event_capabilities[Index] & (1 << avrcp_event_playback_pos_changed))
        {
            /* Get notified of change in playback position */
            AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_pos_changed, AVRCP_PLAYBACK_POSITION_TIME_INTERVAL);
        }

       if(theSink.features.TwsQualificationEnable)
       {
           if (theSink.avrcp_link_data->event_capabilities[Index] & (1 << avrcp_event_volume_changed))
           {
               /* Get notified of change in volume indication*/
               AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_volume_changed, 0);
           }
        }
    }
}

/*************************************************************************
NAME
    handleAvrcpRegisterNotificationInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_REGISTER_NOTIFICATION_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpRegisterNotificationInd(AVRCP_REGISTER_NOTIFICATION_IND_T *msg)
{
    u16 Index;

    LOGD("   event_id [%d]\n", msg->event_id);

    switch (msg->event_id)
    {
        case avrcp_event_volume_changed:
            if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
            {
                theSink.avrcp_link_data->registered_events[Index] |= (1 << msg->event_id);
                LOGD("   (avrcp_event_volume_changed)  registered events [%d] index [%d]\n", theSink.avrcp_link_data->registered_events[Index], Index);
                AvrcpEventVolumeChangedResponse(theSink.avrcp_link_data->avrcp[Index],
                                        avctp_response_interim,
                                        theSink.avrcp_link_data->absolute_volume[Index]);
            }
            break;
#ifdef ENABLE_PEER
        case avrcp_event_playback_status_changed:
            if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
            {
                theSink.avrcp_link_data->registered_events[Index] |= (1 << msg->event_id);
                LOGD("   (avrcp_event_playback_status_changed)  registered events [%d] index [%d]\n", theSink.avrcp_link_data->registered_events[Index], Index);
                AvrcpEventPlaybackStatusChangedResponse(theSink.avrcp_link_data->avrcp[Index],
                                                        avctp_response_interim,
                                                        theSink.avrcp_link_data->play_status[Index]);
            }
            break;
#endif
        /* AVRCP TG for Addressed Player */
        case avrcp_event_addressed_player_changed:
        case avrcp_event_track_changed:
            handleAvrcpQualificationRegisterNotificationInd(msg);
            break;
        default:
            /* other registrations should not be received */
            break;
    }
}

/*************************************************************************
NAME
    handleAvrcpSetAbsVolInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_SET_ABSOLUTE_VOLUME_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpSetAbsVolInd(AVRCP_SET_ABSOLUTE_VOLUME_IND_T *msg)
{
    u16 Index;
    u16 a2dp_volume;


    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        AvrcpSetAbsoluteVolumeResponse(theSink.avrcp_link_data->avrcp[Index],
                                   avctp_response_accepted,
                                   msg->volume);

        theSink.avrcp_link_data->absolute_volume[Index] = msg->volume;

        /* convert AVRCP absolute volume to A2DP volume level, the AVRCP volume range is
           0 to 127, convert this to the headset configured range max steps */
        a2dp_volume = ((msg->volume * (sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps-1))/(AVRCP_MAX_ABS_VOL));

        LOGD("  avrcp volume [%d] digital volume [%d]\n", msg->volume, a2dp_volume);

        setA2dpVolume(&theSink.avrcp_link_data->bd_addr[Index], a2dp_volume);
    }
}

/*************************************************************************
NAME
    handleAvrcpGetPlayStatusCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_PLAY_STATUS_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetPlayStatusCfm(AVRCP_GET_PLAY_STATUS_CFM_T *msg)
{
    u16 Index;

    LOGD("   play status cfm [%d] [%d]\n", msg->status, msg->play_status);

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->play_status == avrcp_play_status_fwd_seek) || (msg->play_status == avrcp_play_status_rev_seek))
            theSink.avrcp_link_data->play_status[Index] = 1 << msg->play_status;
        else
            theSink.avrcp_link_data->play_status[Index] = msg->play_status;
    }
}

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    avrcpUpdatePeerPlayStatus

DESCRIPTION
    Used to update Peer device with current AV source play status.

**************************************************************************/
void avrcpUpdatePeerPlayStatus (avrcp_play_status play_status)
{
    u16 peer_id;

    if (avrcpGetPeerIndex(&peer_id))
    {   /* Forward AV Source play status to Peer device */
         if ((play_status == avrcp_play_status_fwd_seek) || (play_status == avrcp_play_status_rev_seek))
         {
                theSink.avrcp_link_data->play_status[peer_id] = 1 << play_status;
         }
         else
         {
            theSink.avrcp_link_data->play_status[peer_id] = play_status;
         }

        LOGD("AVRCP: Update peer play status [%d] index[%d]\n", theSink.avrcp_link_data->play_status[peer_id], peer_id);
        AvrcpEventPlaybackStatusChangedResponse(theSink.avrcp_link_data->avrcp[peer_id],
                                                avctp_response_changed,
                                                play_status);
    }
}
#endif

/*************************************************************************
NAME
    handleAvrcpPlayStatusChangedInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpPlayStatusChangedInd(AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND_T *msg)
{
    u16 Index;

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        LOGD("   play status ind [%d] [%d] index[%d]\n", msg->response, msg->play_status, Index);
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            if ((msg->play_status == avrcp_play_status_fwd_seek) || (msg->play_status == avrcp_play_status_rev_seek))
            {
                theSink.avrcp_link_data->play_status[Index] = 1 << msg->play_status;
            }
            else
            {
                theSink.avrcp_link_data->play_status[Index] = msg->play_status;
            }

            /* store that this command is supported by remote device */
            theSink.avrcp_link_data->registered_events[Index] |= (1 << avrcp_event_playback_status_changed);

            if (msg->response == avctp_response_changed)
            {
                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_status_changed, 0);
            }

#ifdef ENABLE_PEER
            if((msg->response == avctp_response_changed) && (!isAvrcpIndexPeer(Index, NULL)) && (theSink.peer.current_source == RELAY_SOURCE_A2DP))
            {
                avrcpUpdatePeerPlayStatus( msg->play_status);
            }
#endif

            /* Do a now playing request on an interim or changed notification, but don't unless Playing */
            if (theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_playing)
            {
                u16 a2dpIndex;

                if ((msg->response == avctp_response_changed) && getA2dpIndexFromAvrcp(Index, &a2dpIndex))
                {
                    a2dpSetPlayingState(a2dpIndex, TRUE);
#ifdef ENABLE_PEER
                    PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE PLAYING");

                    LOGD("AVRCP: handleAvrcpPlayStatusChangedInd - playing; cancel pause-suspend timeout a2dp_idx %u\n", a2dpIndex);
                    a2dpCancelPauseSuspendTimeout(a2dpIndex);
#endif
                }
#ifdef ENABLE_PARTYMODE
               if((BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_primary],&theSink.avrcp_link_data->bd_addr[Index])))
               {
                   /* Primary AG has started the audio streaming so cancel the timer */
                   MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1));
               }
               else if((BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_secondary],&theSink.avrcp_link_data->bd_addr[Index])))
               {
                   /* Secondary AG has started the audio streaming so cancel the timer */
                   MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice2));
               }
#endif
#ifdef ENABLE_AVRCP_NOW_PLAYING
                if (Index == sinkAvrcpGetActiveConnection())
                    sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(Index, FALSE);
#endif
#ifdef ENABLE_DISPLAY
                displayShowText(DISPLAYSTR_A2DPSTREAMING,  strlen(DISPLAYSTR_A2DPSTREAMING), 2, DISPLAY_TEXT_SCROLL_SCROLL, 500, 1000, FALSE, 15);
#endif
                /* when using the Soundbar manual audio routing and subwoofer support,
                   check to see if the a2dp audio is being routed, if not check whether
                   an esco subwoofer channel is currently in use, if this is the case it
                   will be necessary to pause this a2dp stream to prevent disruption
                   of the subwoofer esco link due to link bandwidth limitations */
#ifdef ENABLE_SUBWOOFER
               /* deteremine which a2dp link this avrcp is part of */
               if ((theSink.avrcp_link_data->connected[Index])&&(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_primary],&theSink.avrcp_link_data->bd_addr[Index])))
                    suspendWhenSubwooferStreamingLowLatency(a2dp_primary);
               else if ((theSink.avrcp_link_data->connected[Index])&&(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_secondary],&theSink.avrcp_link_data->bd_addr[Index])))
                    suspendWhenSubwooferStreamingLowLatency(a2dp_secondary);
#endif
               /* if the AG has previously set the stream state to 'suspended' when reaching the beginning or
                  end of a playlist/album, ensure that the stream state is set to 'not suspended' now that the
                  audio has resumed playing otherwise the audio will not get routed to the speakers, the AG may not
                  automatically do this for us */
               if (theSink.avrcp_link_data->connected[Index])
               {
                   if((BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_primary],&theSink.avrcp_link_data->bd_addr[Index]))&&
                       (a2dpSuspended(a2dp_primary) == a2dp_remote_suspended))
                   {
                        a2dpSetSuspendState (a2dp_primary, a2dp_not_suspended);
                        /* playback changed, check audio routing */
                        audioHandleRouting(audio_source_none);

                   }
                   else if ((BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_secondary],&theSink.avrcp_link_data->bd_addr[Index]))&&
                            (a2dpSuspended(a2dp_secondary) == a2dp_remote_suspended))
                   {
                        a2dpSetSuspendState (a2dp_secondary, a2dp_not_suspended);
                        /* playback changed, check audio routing */
                        audioHandleRouting(audio_source_none);
                   }
               }
            }
            else if (theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_stopped ||
                     theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_paused)
            {
                u16 a2dpIndex;

                if ((msg->response == avctp_response_changed) && getA2dpIndexFromAvrcp(Index, &a2dpIndex))
                {
#ifdef ENABLE_PEER
                    u16 peerIndex;
                    /* When in extended TWS mode we need to suspend the local
                       AG a2dp stream as soon as possible after it is paused.
                       This is to reduce audio dropouts if the AG on the peer
                       device starts streaming before the local AG stream has
                       been suspended.
                       This is a workaround for a performance limitation
                       when handling > 1 a2dp stream as slave. */
                    AVRCP_DEBUG(("AVRCP:    is_peer %u peer_index %u a2dp_index %u is_slave %u playing %u\n",
                        a2dpGetPeerIndex(&peerIndex), peerIndex, a2dpIndex,
                        (theSink.a2dp_link_data->playing[a2dpIndex])));

                    if (a2dpGetPeerIndex(&peerIndex)
                        && !a2dpIsIndexPeer(a2dpIndex)
                        && theSink.a2dp_link_data->playing[a2dpIndex])
                    {
                        a2dpStartPauseSuspendTimeout(a2dpIndex);
                    }

#endif
                    a2dpSetPlayingState(a2dpIndex, FALSE);
#ifdef ENABLE_PEER
                    if (theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_paused)
                    {
                        /* if we are not in single device mode, we also need to make the link available */
                        if(!theSink.features.TwsSingleDeviceOperation && !a2dpIsIndexPeer(a2dpIndex))
                        {
                            peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_RELAY_AVAILABLE);
                        }

                        PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE PAUSED");
                    }
                    else
                    {
                        PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE STOPPED");
                    }
#endif
                }
#ifdef ENABLE_DISPLAY
                /* playback stopped clear the display */
                displayShowSimpleText(DISPLAYSTR_CLEAR,1);
                displayShowSimpleText(DISPLAYSTR_CLEAR,2);
#endif
            }

            if (theSink.features.EnableAvrcpAudioSwitching &&
                (msg->response == avctp_response_changed) &&
                theSink.avrcp_link_data->play_status[Index] != (1 << avrcp_play_status_fwd_seek) &&
                theSink.avrcp_link_data->play_status[Index] != (1 << avrcp_play_status_rev_seek))
            {
                /* playback changed, check audio routing */
                audioHandleRouting(audio_source_none);
            }

#ifdef ENABLE_PARTYMODE
            if (theSink.features.EnableAvrcpAudioSwitching &&
                (msg->response == avctp_response_changed) &&
                ((theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_paused)||
                 (theSink.avrcp_link_data->play_status[Index] == avrcp_play_status_stopped)))
            {
                /* notify party mode that current track being played has ended */
                sinkPartyModeTrackChangeIndication(Index);
            }
#endif
        }
        else
        {
            /* assume not supported by remote device */
            theSink.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_playback_status_changed);
        }
    }
}

#ifdef ENABLE_AVRCP_NOW_PLAYING
/*************************************************************************
NAME
    handleAvrcpGetElementAttributesCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_ELEMENT_ATTRIBUTES_CFM from the AVRCP library.
    This will return the currently now playing track information which can be displayed.

**************************************************************************/
static void handleAvrcpGetElementAttributesCfm(AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_T *msg)
{
    u16 Index;

    if ((msg->status == avrcp_success) && (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index)))
    {
        u16 i = 0;
        u32 attribute_id = 0;
        u16 charset_id = 0;
        u16 attribute_length = 0;
        u16 header_end = 0;
        const u8 *lSource = SourceMap(msg->attributes);
        u16 source_size = SourceSize(msg->attributes);
        u16 size_attributes = (source_size < msg->size_attributes) ? source_size : msg->size_attributes;

        if ((lSource) && (msg->number_of_attributes > 0))
        {
            LOGD("   success; packet[%d] num_attr[%d] size_att[%d] size_source[%d]\n", msg->metadata_packet_type, msg->number_of_attributes, msg->size_attributes, source_size);

            header_end = i + AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_HEADER_SIZE;
            while (size_attributes >= header_end) /* check there is room left for data to exist */
            {
                attribute_id = ((u32)lSource[i] << 24) | ((u32)lSource[i + 1] << 16) | ((u32)lSource[i + 2] << 8) | lSource[i + 3];
                charset_id = (lSource[i + 4] << 8) | lSource[i + 5];
                attribute_length = (lSource[i + 6] << 8) | lSource[i + 7];

                LOGD("        attribute = 0x%lx\n", attribute_id);
                LOGD("        charset_id = 0x%x\n", charset_id);
                LOGD("        attribute_length = 0x%x\n", attribute_length);

                if ((attribute_length + header_end) > size_attributes)
                    attribute_length = size_attributes - header_end;

#ifdef DEBUG_AVRCP
                {
                    u16 j;
                    LOGD("        attribute text : ");
                    for (j = i; j < (i + attribute_length); j++)
                    {
                        LOGD("%c", lSource[j + 8]);
                    }
                        LOGD("\n");
                }
#endif

                LOGD("AVRCP NOW PLAYING:\n");

                if (attribute_length)
                    sinkAvrcpDisplayMediaAttributes(attribute_id, attribute_length, &lSource[i + 8]);

                i = header_end + attribute_length;
                header_end = i + AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_HEADER_SIZE;
            }

            /* finished processing the source */
            AvrcpSourceProcessed(msg->avrcp);

            if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_END) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_CONTINUE))
            {
                /* cancel sending an Abort */
                MessageCancelAll(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CTRL_ABORT_CONTINUING_RESPONSE);
            }
            /* If the response is fragmented then request the next chunk of data */
            if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_START) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_CONTINUE))
            {
                MAKE_AVRCP_MESSAGE(AVRCP_CTRL_ABORT_CONTINUING_RESPONSE);
                message->pdu_id = AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID;
                AvrcpRequestContinuingResponseRequest(msg->avrcp, AVRCP_GET_ELEMENT_ATTRIBUTES_PDU_ID);
                MessageSendLater(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CTRL_ABORT_CONTINUING_RESPONSE, message, AVRCP_ABORT_CONTINUING_TIMEOUT);
            }
        }
        else
        {
            LOGD("   fail; no source; num attributes zero\n");
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }
}


/*************************************************************************
NAME
    handleAvrcpNowPlayingContentChangedInd

DESCRIPTION
    Notification that Now Playing Content has changed.

**************************************************************************/
static void handleAvrcpNowPlayingContentChangedInd(AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED_IND_T *msg)
{
    u16 Index;

    LOGD("   now playing content changed ind [%d]\n", msg->response);

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            /* store that this command is supported by remote device */
            if (msg->response == avctp_response_changed)
            {
                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_now_playing_content_changed, 0);

                if (sinkAvrcpGetActiveConnection() == Index)
                {
                    /* TODO now playing content changed on active connection, may need to update display */
                }
            }
        }
        else
        {
            /* assume not supported by remote device */
            theSink.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_now_playing_content_changed);
        }
    }
}

#endif /* ENABLE_AVRCP_NOW_PLAYING */

/*************************************************************************
NAME
    handleAvrcpTrackChangedInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_EVENT_TRACK_CHANGED_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpTrackChangedInd(AVRCP_EVENT_TRACK_CHANGED_IND_T *msg)
{
    u16 Index;

    LOGD("   track changed ind [%x] [0x%lx][0x%lx]\n", msg->response, msg->track_index_high, msg->track_index_low);

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            /* store that this command is supported by remote device */
            theSink.avrcp_link_data->registered_events[Index] |= (1 << avrcp_event_track_changed);


            if (msg->response == avctp_response_changed)
            {
                LOGD("TrackChanged- Response change\n");
                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_track_changed, 0);

#ifdef ENABLE_AVRCP_NOW_PLAYING
                /* request now playing information */
                if ((msg->track_index_high != 0xffffffff) || (msg->track_index_low != 0xffffffff))
                {
                    if (Index == sinkAvrcpGetActiveConnection())
                        sinkAvrcpRetrieveNowPlayingRequest(msg->track_index_high, msg->track_index_low, FALSE);
                }
#endif /* ENABLE_AVRCP_NOW_PLAYING */

#ifdef ENABLE_PARTYMODE
                /* notify party mode that current track being played has ended */
                sinkPartyModeTrackChangeIndication(Index);
#endif
            }
        }
        else
        {
            /* assume not supported by remote device */
            theSink.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_track_changed);
        }
    }
}


/*************************************************************************
NAME
    handleAvrcpPlaybackPosChangedInd

DESCRIPTION
    Functionality as a result of receiving AVRCP_EVENT_PLAYBACK_POS_CHANGED_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpPlaybackPosChangedInd(AVRCP_EVENT_PLAYBACK_POS_CHANGED_IND_T *msg)
{
    u16 Index;

    LOGD("   playback pos changed ind [%d] playback_pos[0x%lx]\n", msg->response, msg->playback_pos);

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            if (msg->response == avctp_response_changed)
            {
                u16 a2dpIndex;
                if (getA2dpIndexFromAvrcp(Index, &a2dpIndex) )
                {
                    a2dpPauseNonRoutedSource(a2dpIndex);
                }

                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_pos_changed, AVRCP_PLAYBACK_POSITION_TIME_INTERVAL);
            }
        }
        else
        {
            /* assume not supported by remote device */
            theSink.avrcp_link_data->event_capabilities[Index] &= ~(1 << avrcp_event_playback_pos_changed);
        }
    }
}

#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
/*************************************************************************
NAME
    handleAvrcpListAppAttributeCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_LIST_APP_ATTRIBUTE_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpListAppAttributeCfm(AVRCP_LIST_APP_ATTRIBUTE_CFM_T *msg)
{
    u16 Index;
    avrcp_status_code avrcp_status = msg->status;
    u16 attributes_returned = 0;
    u16 source_size = SourceSize(msg->attributes);
    const u8 *lSource = SourceMap(msg->attributes);
    u16 size_attributes = (source_size < msg->size_attributes) ? source_size : msg->size_attributes;

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        attributes_returned = (size_attributes < msg->number_of_attributes) ? size_attributes : msg->number_of_attributes;

        if (lSource)
        {
            LOGD("   success; packet[%d] num_attr[%d] size_att[%d]\n", msg->metadata_packet_type, msg->number_of_attributes, msg->size_attributes);

#ifdef DEBUG_AVRCP
            {
                u16 i;
                for (i = 0; i < attributes_returned; i++)
                {
                    LOGD("        attribute[%d] = 0x%x\n", i, lSource[i]);
                }
            }
#endif

        }
        else
        {
            LOGD("   fail; no source\n");
            avrcp_status = avrcp_fail;
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }

    /* TODO there are attributes_returned attribute IDs contained in lSource - can now retrieve text for these attributes (sinkAvrcpListPlayerAttributesTextRequest) */

    /* finished processing the source */
    AvrcpSourceProcessed(msg->avrcp);
}


/*************************************************************************
NAME
    handleAvrcpGetAppAttributeValueTextCfm

DESCRIPTION
    Functionality as a result of receiving attribute text and attribute values text.

**************************************************************************/
static void handleAvrcpGetAppAttributeValueTextCfm(u16 pdu_id, AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T *msg)
{
    u16 Index;
    avrcp_status_code avrcp_status = msg->status;
    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        u16 i = 0;
        u16 attribute_id = 0;
        u16 charset_id = 0;
        u16 attribute_length = 0;
        u16 header_end = 0;
        const u8 *lSource = SourceMap(msg->attributes);
        u16 source_size = SourceSize(msg->attributes);
        u16 size_attributes = (source_size < msg->size_attributes) ? source_size : msg->size_attributes;

        if (lSource)
        {
            LOGD("   success; packet[%d] num_attr[%d] size_att[%d] source_size[%d]\n", msg->metadata_packet_type, msg->number_of_attributes, msg->size_attributes, source_size);

            if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_SINGLE) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_START))
            {
                /* TODO prepare display for msg->number_of_attributes number of attributes with text to be returned in following responses */
            }

            header_end = i + AVRCP_GET_APP_ATTRIBUTES_TEXT_CFM_HEADER_SIZE;

            while (size_attributes >= header_end) /* check there is room left for header to string data to exist */
            {
                attribute_id = lSource[i];
                charset_id = (lSource[i + 1] << 8) | lSource[i + 2];
                attribute_length = lSource[i + 3];

                LOGD("        attribute = 0x%x\n", attribute_id);
                LOGD("        charset_id = 0x%x\n", charset_id);
                LOGD("        attribute_length = 0x%x\n", attribute_length);

                if ((attribute_length + header_end) > size_attributes)
                {
                    attribute_length = size_attributes - header_end;
                    LOGD("        new attr length = %d\n", attribute_length);
                }

                if (attribute_length)
                {
#ifdef DEBUG_AVRCP
                    {
                        u16 j;
                        LOGD("        attribute text : ");
                        for (j = i; j < (i + attribute_length); j++)
                        {
                            LOGD("%c", lSource[j + 4]);
                        }
                        LOGD("\n");
                    }
#endif

                    /* TODO display attributes that have text associated with them - the text is of length attribute_length and starts at lSource[i + 4] */

                }

                i = header_end + attribute_length;
                header_end = i + AVRCP_GET_APP_ATTRIBUTES_TEXT_CFM_HEADER_SIZE;
            }

            if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_END) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_CONTINUE))
            {
                /* cancel sending an Abort */
                MessageCancelAll(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CTRL_ABORT_CONTINUING_RESPONSE);
            }
            /* If the response is fragmented then request the next chunk of data */
            if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_START) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_CONTINUE))
            {
                MAKE_AVRCP_MESSAGE(AVRCP_CTRL_ABORT_CONTINUING_RESPONSE);
                message->pdu_id = pdu_id;
                AvrcpRequestContinuingResponseRequest(msg->avrcp, pdu_id);
                MessageSendLater(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CTRL_ABORT_CONTINUING_RESPONSE, message, AVRCP_ABORT_CONTINUING_TIMEOUT);
            }
            else if ((msg->metadata_packet_type == AVRCP_PACKET_TYPE_SINGLE) || (msg->metadata_packet_type == AVRCP_PACKET_TYPE_END))
            {
                /* TODO this is the last packet in response so display of text is complete */
            }
        }
        else
        {
            LOGD("   fail; no source\n");
            avrcp_status = avrcp_fail;
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }

    if (!success)
    {
        /* TODO response is not valid so update display based on avrcp_status */
    }

    /* finished processing the source */
    AvrcpSourceProcessed(msg->avrcp);
}


/*************************************************************************
NAME
    handleAvrcpListAppValueCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_LIST_APP_VALUE_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpListAppValueCfm(AVRCP_LIST_APP_VALUE_CFM_T *msg)
{
    u16 Index;
    avrcp_status_code avrcp_status = msg->status;
    const u8 *lSource = SourceMap(msg->values);
    u16 source_size = SourceSize(msg->values);
    u16 attributes_returned = msg->number_of_values;
    u16 size_values = (source_size < msg->size_values) ? source_size : msg->size_values;

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if (lSource)
        {
            LOGD("   success; packet[%d] num_attr[%d] size_att[%d] source_size[%d]\n", msg->metadata_packet_type, msg->number_of_values, msg->size_values, source_size);
            if (size_values < attributes_returned)
                attributes_returned = size_values;

#ifdef DEBUG_AVRCP
            {
                u16 j;
                for (j = 0; j < attributes_returned; j++)
                {
                    LOGD("        value_id[%d] = 0x%x\n", j, lSource[j]);
                }
            }
#endif
        }
        else
        {
            LOGD("   fail; no source\n");
            avrcp_status = avrcp_fail;
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }

    /* TODO there are attributes_returned number of values returned in lSource, can now retrieve text for these values (sinkAvrcpListPlayerValueTextRequest) */

    /* finished processing the source */
    AvrcpSourceProcessed(msg->avrcp);
}


/*************************************************************************
NAME
    handleAvrcpGetAppValueCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_APP_VALUE_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetAppValueCfm(AVRCP_GET_APP_VALUE_CFM_T * msg)
{
    u16 Index;
    avrcp_status_code avrcp_status = msg->status;

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        const u8 *lSource = SourceMap(msg->values);
        u16 source_size = SourceSize(msg->values);
        u16 attributes_returned = msg->number_of_values;
        u16 size_values = (source_size < msg->size_values) ? source_size : msg->size_values;
        u16 i = 0;
        u8 attribute_id = 0;
        u8 value_id = 0;

        if (lSource)
        {
            LOGD("   success; packet[%d] num_attr[%d] size_att[%d] source_size[%d]\n", msg->metadata_packet_type, msg->number_of_values, msg->size_values, source_size);
            if (size_values < (attributes_returned * AVRCP_GET_APP_VALUE_CFM_DATA_SIZE))
                attributes_returned = size_values / AVRCP_GET_APP_VALUE_CFM_DATA_SIZE;

            while ((size_values - i) >= AVRCP_GET_APP_VALUE_CFM_DATA_SIZE)
            {
                attribute_id = lSource[i];
                value_id = lSource[i + 1];
                i += AVRCP_GET_APP_VALUE_CFM_DATA_SIZE;

                LOGD("        attribute = 0x%x\n", attribute_id);
                LOGD("        value_id = 0x%x\n", value_id);

                /* TODO store {attribute_id, value_id} pair for display */
            }
        }
        else
        {
            LOGD("   fail; no source\n");
            avrcp_status = avrcp_fail;
        }
    }
    else
    {
        LOGD("   fail; status %d\n", msg->status);
    }

    /* get value is complete with outcome of avrcp_status */

    /* finished processing the source */
    AvrcpSourceProcessed(msg->avrcp);
}


/*************************************************************************
NAME
    handleAvrcpSetAppValueCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_SET_APP_VALUE_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpSetAppValueCfm(AVRCP_SET_APP_VALUE_CFM_T *msg)
{
    LOGD(" handleAvrcpSetAppValueCfm  status %d\n", msg->status);

    /* setting of value is now complete */
}

/*************************************************************************
NAME
    sinkAvrcpPlayerAppSettingsChangedInd

DESCRIPTION
    Notification of change in Player Application Settings

**************************************************************************/
static void sinkAvrcpPlayerAppSettingsChangedInd(AVRCP_EVENT_PLAYER_APP_SETTING_CHANGED_IND_T *msg)
{
    u16 Index;

    LOGD("   Player App Settings changed ind [%d]\n", msg->response);

    if (sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            if (msg->response == avctp_response_changed)
            {
                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_player_app_setting_changed, 0);

                if (sinkAvrcpGetActiveConnection() == Index) /* update display if this is the active connection */
                {
                    /* TODO active connection has had its Player Application Settings changed, may need to update display */
                }

            }
        }
        else
        {
            /* assume not supported by remote device */
            theSink.avrcp_link_data->event_capabilities[Index] &= ~(1 << avrcp_event_player_app_setting_changed);
        }
    }
    if (msg->attributes)
        AvrcpSourceProcessed(msg->avrcp);
}
#endif


/********************/
/* LIBRARY SPECIFIC */
/********************/

/*************************************************************************
NAME
    handleAvrcpGetExtensionsCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_EXTENSIONS_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetExtensionsCfm(AVRCP_GET_EXTENSIONS_CFM_T *msg)
{
    u16 Index;

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theSink.avrcp_link_data->extensions[Index] = msg->extensions;
        LOGD("   extensions 0x%x\n", msg->extensions);
        /* now retrieve supported features */
        AvrcpGetSupportedFeatures(msg->avrcp);
    }
}

/*************************************************************************
NAME
    handleAvrcpGetSupportedFeaturesCfm

DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_SUPPORTED_FEATURES_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetSupportedFeaturesCfm(AVRCP_GET_SUPPORTED_FEATURES_CFM_T *msg)
{
    u16 Index;

    if ((msg->status == avrcp_success) && sinkAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theSink.avrcp_link_data->features[Index] = msg->features;

        LOGD("   features 0x%x\n", msg->features);

        if (isAvrcpCategory1MetadataEnabled(Index))
        {
            LOGD("   Category 1 Metadata enabled\n");

            /* Get the TG supported event capabilities */
            AvrcpGetCapsRequest(msg->avrcp, avrcp_capability_event_supported);

#ifdef ENABLE_AVRCP_NOW_PLAYING
            /* Get Now Playing information for display, by registering Track Changed event */
            AvrcpRegisterNotificationRequest(theSink.avrcp_link_data->avrcp[Index], avrcp_event_track_changed, 0);
#endif
            if (sinkAvrcpBrowsingMultipleMediaPlayersIsSupported(Index))
            {
                /* Get currently active media player */
                AvrcpRegisterNotificationRequest(theSink.avrcp_link_data->avrcp[Index], avrcp_event_addressed_player_changed, 0);
            }
        }
        else
        {
            LOGD("   No Metadata enabled\n");
        }
    }
}


/*********************** Main Functions ***************************/

/*************************************************************************
NAME
    sinkAvrcpInit

DESCRIPTION
    Initialise the AVRCP library.

**************************************************************************/
void sinkAvrcpInit(void)
{
    avrcp_init_params config;
    u16 *buffer;
    u16 i;

    LOGD("AVRCP: Avrcp Init\n");
    if(theSink.features.avrcp_enabled)
    {
         /* Initialise AVRCP library */
        config.device_type = avrcp_target_and_controller;
        config.supported_controller_features = AVRCP_CATEGORY_1; /* mainly operates as controller */
        config.supported_target_features = AVRCP_CATEGORY_2; /* only target for volume commands */
#ifdef ENABLE_PEER
        config.supported_target_features |= AVRCP_CATEGORY_1; /* Peer devices also a target for category 1 commands (play, pause etc) */
#endif
        config.profile_extensions = AVRCP_VERSION_1_6; /* operate as AVRCP 1.6 device */


#ifdef ENABLE_AVRCP_BROWSING
        config.profile_extensions |= AVRCP_BROWSING_SUPPORTED; /* indicate Browsing support */
#endif


        AvrcpInit(&theSink.task, &config);
    }
    else
    {
        config.device_type = 0;
        config.supported_controller_features = 0;
        config.supported_target_features = 0;
        config.profile_extensions = 0;
    }

    /* initialise avrcp data memory (A2DP and AVRCP data share same memory allocation) */
    buffer = (u16 *)theSink.a2dp_link_data;
    theSink.avrcp_link_data = (avrcp_data *)&buffer[sizeof(a2dp_data)];
    /* initialise structure to 0 */
    memset(theSink.avrcp_link_data, 0, (sizeof(avrcp_data)));
    /* Initialise local message handler */
    for_all_avrcp(i)
        theSink.avrcp_link_data->avrcp_ctrl_handler[i].handler = avrcpControlHandler;
    /* Initialise free up message handler */
    for_all_avrcp(i)
    {
        (theSink.avrcp_link_data->dataCleanUpTask[i].task).handler = sinkAvrcpDataCleanUp;
        theSink.avrcp_link_data->dataCleanUpTask[i].data = NULL;
    }

    /* Even for PTS qualification, use the same dataclean up task */
    sinkAvrcpQualificationInit();

    /* initialise Browsing variables */
    sinkAvrcpBrowsingChannelInit(TRUE, 0);
}

/****************************************************************************
*NAME
*    sinkAvrcpDataCleanUp
*
*DESCRIPTION
*    handler function to free the allocated data.
******************************************************************************/
void sinkAvrcpDataCleanUp(Task task, MessageId id, Message message)
{
    SinkAvrcpCleanUpTask *cleanTask = (SinkAvrcpCleanUpTask *) task;

    switch (id)
    {
    case MESSAGE_SOURCE_EMPTY:
        {
            /* Free the previously stored data ptr. */
            if (cleanTask->data)
            {
                free(cleanTask->data);
                LOGD("avrcpDataCleanUp: Cleanup Stored Data \n");
            }
            cleanTask->data = NULL;
        }
        break;

    default:
        AVRCP_DEBUG((" AVRCP handleUnexpected in "
                   " SinkAvrcpCleanUpTask- MsgId 0x%x\n",id));
        break;
    }
}


/*************************************************************************
NAME
    sinkAvrcpDisconnect

DESCRIPTION
    Disconnect the AVRCP connection associated with the specified device.

**************************************************************************/
void sinkAvrcpDisconnect(const bdaddr *bd_addr)
{
    u16 Index;

    LOGD("AVRCP: Avrcp Disconnect\n");
    if (sinkAvrcpGetIndexFromBdaddr(bd_addr, &Index, TRUE))
    {
        LOGD("    Disconnect [%x:%x:%lx] index[%d]\n", bd_addr->uap, bd_addr->nap, bd_addr->lap, Index);
        AvrcpDisconnectRequest(theSink.avrcp_link_data->avrcp[Index]);
    }
}


/*************************************************************************
NAME
    sinkAvrcpDisconnectAll

DESCRIPTION
    Disconnect all AVRCP connections.

**************************************************************************/
void sinkAvrcpDisconnectAll(bool disconnect_peer)
{
    u8 i;

    LOGD("AVRCP: Avrcp Disconnect All\n");
    if(theSink.avrcp_link_data)
    {
        /* loop for all AVRCP connections */
        for_all_avrcp(i)
        {
#ifdef ENABLE_PEER
            if (theSink.avrcp_link_data->connected[i] && ( !isAvrcpIndexPeer(i, NULL) || disconnect_peer))
#else
            if (theSink.avrcp_link_data->connected[i])
#endif
            {
                AvrcpDisconnectRequest(theSink.avrcp_link_data->avrcp[i]);
                BdaddrSetZero(&theSink.avrcp_link_data->bd_addr[i]);
            }
            theSink.avrcp_link_data->avrcp_manual_connect = FALSE;
            BdaddrSetZero(&theSink.avrcp_link_data->avrcp_play_addr);
        }
    }
}


/*************************************************************************
NAME
    sinkAvrcpAclClosed

DESCRIPTION
    The ACL to the device specified has closed, so perform any clean up that is required

**************************************************************************/
void sinkAvrcpAclClosed(bdaddr bd_addr)
{
    u16 Index;

    /* It is possible that a disconnect crossover can fail to inform the application of the AVRCP disconnection.
       Clean up any active connections if the ACL has disappeared */
    if (sinkAvrcpGetIndexFromBdaddr(&bd_addr, &Index, FALSE))
    {
        LOGD("AVRCP: Reset AVRCP connection as ACL closed %d\n", Index);
        resetAvrcpConnection(Index);
        sinkAvrcpBrowsingChannelInit(FALSE, Index);
    }
}

/*************************************************************************
NAME
    sinkAvrcpConnect

DESCRIPTION
    Create an AVRCP connection with with the specified device.

**************************************************************************/
void sinkAvrcpConnect(const bdaddr *bd_addr, u16 delay_time)
{
    u16 connections = 0;
    u16 i;
    bool bdaddr_set = FALSE;
    u16 Index = 0;


    if ( !theSink.avrcp_link_data || !bd_addr || BdaddrIsZero(bd_addr) )
    {   /* Avrcp link data structure not initialised or invalid connection address */
        LOGD("AVRCP: Avrcp Connect ***unable to action*** - bd_addr=0x%X avrcp_link_data=0x%X\n", (u16) bd_addr, (u16) theSink.avrcp_link_data);
        return;
    }

    LOGD("AVRCP: Avrcp Connect [%x:%x:%lx] delay[%d]\n", bd_addr->nap, bd_addr->uap, bd_addr->lap, delay_time);

    for_all_avrcp(i)
    {
        if (!BdaddrIsZero(&theSink.avrcp_link_data->bd_addr[i]))
        {
            /* count the number of currently active connections, and don't allow a futher connection to the same device */
            connections++;
            if (BdaddrIsSame(&theSink.avrcp_link_data->bd_addr[i], bd_addr))
            {
                return;
            }
        }
        else if (!bdaddr_set)
        {
            /* store the address of the attempted connection */
            theSink.avrcp_link_data->bd_addr[i] = *bd_addr;
            Index = i;
            bdaddr_set = TRUE;
        }
    }

    if ((connections < MAX_AVRCP_CONNECTIONS) && bdaddr_set)
    {
        if (!delay_time)
        {
            LOGD("   Avrcp connect request\n");
            AvrcpConnectRequest(&theSink.task, bd_addr);
        }
        else
        {
            MAKE_AVRCP_MESSAGE(AVRCP_CREATE_CONNECTION);
            message->bd_addr = *bd_addr;
            LOGD("   Avrcp delayed connect [%d]\n", delay_time);
            MessageSendLater(&theSink.avrcp_link_data->avrcp_ctrl_handler[Index], AVRCP_CREATE_CONNECTION, message, delay_time);
        }
    }
}

/*************************************************************************
NAME
    sinkAvrcpManualConnect

DESCRIPTION
    Create an manual AVRCP connection with  the specified device

**************************************************************************/
void sinkAvrcpManualConnect(void)
{
    u16 i;
    for_all_a2dp(i)
    {
        /* see if an a2dp link is connected without AVRCP */
        if (theSink.a2dp_link_data->connected[i])
        {
            if (theSink.a2dp_link_data->avrcp_support[i] != avrcp_support_unsupported)
            {
                theSink.a2dp_link_data->avrcp_support[i] = avrcp_support_unknown;
                theSink.avrcp_link_data->avrcp_manual_connect = TRUE;
                theSink.avrcp_link_data->avrcp_play_addr = theSink.a2dp_link_data->bd_addr[i];
                sinkAvrcpConnect(&theSink.a2dp_link_data->bd_addr[i], DEFAULT_AVRCP_NO_CONNECTION_DELAY);
                LOGD("AVRCP: Retry AVRCP connection\n");
            }
            else
            {
                /* TODO if AVRCP can't be connected, control stopping and starting of A2DP audio here */
                LOGD("AVRCP: AVRCP not supported\n");
            }
            return;
        }
    }
    LOGD("AVRCP: Establish SLC connections\n");
    theSink.avrcp_link_data->avrcp_manual_connect = TRUE;
    MessageSend(&theSink.task, EventUsrEstablishSLC, 0 );
}

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    sinkAvrcpVolumeUp

DESCRIPTION
    Send an AVRCP_VOLUME_UP to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpVolumeUp (void)
{
    u16 Index;

    LOGD("AVRCP: Avrcp Volume Up\n");

    /* Check if the provided index is of a true AV device or a remote device with a true Av conncted , otherwise find the next valid index*/
    if (avrcpGetPeerIndex(&Index))
    {
        if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
        {
            avrcpSendControlMessage(AVRCP_CTRL_VOLUME_UP_PRESS, Index);
            avrcpSendControlMessage(AVRCP_CTRL_VOLUME_UP_RELEASE, Index);
        }
    }
}
#endif

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    sinkAvrcpVolumeDown

DESCRIPTION
    Send an AVRCP_VOLUME_DOWN to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpVolumeDown (void)
{
    u16 Index;

    LOGD("AVRCP: Avrcp Volume Up\n");

    /* Check if the provided index is of a true AV device or a remote device with a true Av conncted , otherwise find the next valid index*/
    if (avrcpGetPeerIndex(&Index))
    {
        if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
        {
            avrcpSendControlMessage(AVRCP_CTRL_VOLUME_DOWN_PRESS, Index);
            avrcpSendControlMessage(AVRCP_CTRL_VOLUME_DOWN_RELEASE, Index);
        }
    }
}
#endif

#ifdef ENABLE_PEER
/*************************************************************************
NAME
    sinkAvrcpPowerOff

DESCRIPTION
    Send an AVRCP_POWER to the peer device.

**************************************************************************/
void sinkAvrcpPowerOff (void)
{
    u16 Index;

    LOGD("AVRCP: Avrcp Power off\n");

    if (avrcpGetPeerIndex(&Index))
    {
        if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
        {
            avrcpSendControlMessage(AVRCP_CTRL_POWER_OFF, Index);
        }
    }
}
#endif


/*************************************************************************
NAME
    sinkAvrcpPlay

DESCRIPTION
    Send an AVRCP_PLAY to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpPlay(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();
    u16 IndexToUse = 0;

    LOGD("AVRCP: Avrcp Play\n");

#ifdef ENABLE_PEER
    /* Check if the provided index is of a true AV device or a remote device with a true Av conncted , otherwise find the next valid index*/
    if(getValidAvrcpIndex(&IndexToUse , Index))
    {
        if(IndexToUse != Index)
        {
            /* We have a new index so update the active connection*/
            sinkAvrcpSetActiveConnectionFromBdaddr( &(theSink.avrcp_link_data->bd_addr[IndexToUse]));
        }
#else
     if (theSink.avrcp_link_data->connected[Index])
     {
        IndexToUse = Index;
#endif
        if (getAvrcpQueueSpace(IndexToUse) >= 2)
        {
            avrcpSendControlMessage(AVRCP_CTRL_PLAY_PRESS, IndexToUse);
            avrcpSendControlMessage(AVRCP_CTRL_PLAY_RELEASE, IndexToUse);
            if (!isAvrcpPlaybackStatusSupported(IndexToUse))
            {
                theSink.avrcp_link_data->play_status[IndexToUse] = avrcp_play_status_playing;
            }
#ifdef ENABLE_PEER
            /* If single device operation is disabled and the IndexToUse is a peer device then mark the peer relay as available */
            if( !theSink.features.TwsSingleDeviceOperation && isAvrcpIndexPeer(IndexToUse, NULL) )
            {
                peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_RELAY_AVAILABLE);
            }
#endif
        }
     }
    else
    {
        /* Couldn't find a valid index to route this comand so connect manually*/
        sinkAvrcpManualConnect();
    }
}


/*************************************************************************
NAME
    sinkAvrcpPause

DESCRIPTION
    Send an AVRCP_PAUSE to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpPause(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();
    u16 IndexToUse = 0;

    LOGD("AVRCP: Avrcp Pause\n");

#ifdef ENABLE_PEER
    /* Check if the provided index is of a true AV device or a remote device with a true Av conncted , otherwise find the next valid index*/
    if(getValidAvrcpIndex(&IndexToUse , Index))
    {
        if(IndexToUse != Index)
        {
            /* We have a new index so update the active connection*/
            sinkAvrcpSetActiveConnectionFromBdaddr( &(theSink.avrcp_link_data->bd_addr[IndexToUse]));
        }
#else
     if (theSink.avrcp_link_data->connected[Index])
     {
        IndexToUse = Index;
#endif
        if ( (getAvrcpQueueSpace(IndexToUse) >= 2) &&
              (theSink.avrcp_link_data->play_status[IndexToUse] == avrcp_play_status_playing))
        {
            avrcpSendControlMessage(AVRCP_CTRL_PAUSE_PRESS, IndexToUse);
            avrcpSendControlMessage(AVRCP_CTRL_PAUSE_RELEASE, IndexToUse);
            if (!isAvrcpPlaybackStatusSupported(IndexToUse))
            {
                theSink.avrcp_link_data->play_status[IndexToUse] = avrcp_play_status_paused;
            }
#ifdef ENABLE_PEER
            /* If single device operation is disabled and the IndexToUse is a peer device then mark the peer relay as unavailable */
            if( !theSink.features.TwsSingleDeviceOperation && isAvrcpIndexPeer(IndexToUse, NULL) )
            {
                peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_RELAY_UNAVAILABLE);
            }
#endif
        }
    }
    else
    {
        sinkAvrcpManualConnect();
    }
}

/*************************************************************************
NAME
    sinkAvrcpPlayPause

DESCRIPTION
    Send a AVRCP_PLAY / AVRCP_PAUSE based on play status, to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpPlayPause(void)
{
    u16 a2dpIndex = 0;
    a2dp_suspend_state suspend_state = a2dp_not_suspended;
    u16 Index = sinkAvrcpGetActiveConnection();
    u16 IndexToUse = 0;

    LOGD("AVRCP: Avrcp PlayPause\n");

#ifdef ENABLE_PEER
    /* Check if the provided index is of a true AV device or a remote device with a true Av conncted , otherwise find the next valid index*/
    if(getValidAvrcpIndex(&IndexToUse , Index))
    {
        if(IndexToUse != Index)
        {
            /* We have a new index so update the active connection*/
            sinkAvrcpSetActiveConnectionFromBdaddr( &(theSink.avrcp_link_data->bd_addr[IndexToUse]));
        }
#else
     if (theSink.avrcp_link_data->connected[Index])
     {

        IndexToUse = Index;
#endif
        getA2dpIndexFromAvrcp(Index, &a2dpIndex);
        suspend_state = theSink.a2dp_link_data->SuspendState[a2dpIndex];

        if ((theSink.avrcp_link_data->play_status[IndexToUse] != avrcp_play_status_playing) ||
            (suspend_state != a2dp_not_suspended))
        {
            sinkAvrcpPlay();
        }
        else
        {
            sinkAvrcpPause();
        }
    }
    else
    {
       sinkAvrcpManualConnect();
    }
}


/*************************************************************************
NAME
    sinkAvrcpStop

DESCRIPTION
    Send a AVRCP_STOP to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpStop(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Stop\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_STOP_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_STOP_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_stopped;
    }
}


/*************************************************************************
NAME
    sinkAvrcpSkipForward

DESCRIPTION
    Send a AVRCP_FORWARD to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpSkipForward(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Skip Fwd\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FORWARD_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_FORWARD_RELEASE, Index);
    }
}


/*************************************************************************
NAME
    sinkAvrcpSkipBackward

DESCRIPTION
    Send a AVRCP_BACKWARD to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpSkipBackward(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Skip Bwd\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_BACKWARD_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_BACKWARD_RELEASE, Index);
    }
}


/*************************************************************************
NAME
    sinkAvrcpFastForwardPress

DESCRIPTION
    Send a AVRCP_FAST_FORWARD pressed to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpFastForwardPress(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp FFWD Press\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FF_PRESS, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            /* use bitmask for fwd and rev so with AVRCP 1.0 devices can store previous play status before this operation */
            theSink.avrcp_link_data->play_status[Index] |= (1 << avrcp_play_status_fwd_seek);
            theSink.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_rev_seek);
        }
    }
}


/*************************************************************************
NAME
    sinkAvrcpFastForwardRelease

DESCRIPTION
    Send a AVRCP_FAST_FORWARD released to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpFastForwardRelease(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp FFWD Release\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FF_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theSink.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_fwd_seek);
    }
}


/*************************************************************************
NAME
    sinkAvrcpRewindPress

DESCRIPTION
    Send a AVRCP_REWIND pressed to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpRewindPress(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp REW Press\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_REW_PRESS, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            theSink.avrcp_link_data->play_status[Index] |= (1 << avrcp_play_status_rev_seek);
            theSink.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_fwd_seek);
        }
    }
}


/*************************************************************************
NAME
    sinkAvrcpRewindRelease

DESCRIPTION
    Send a AVRCP_REWIND released to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpRewindRelease(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp REW Release\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_REW_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theSink.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_rev_seek);
    }
}


/*************************************************************************
NAME
    sinkAvrcpNextGroupPress

DESCRIPTION
    Send a Next Group Press command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpNextGroupPress(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Next Group Press\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) && isAvrcpGroupNavigationEnabled(Index))
    {
        avrcpSendControlMessage(AVRCP_CTRL_NEXT_GROUP_PRESS, Index);
    }
}

/*************************************************************************
NAME
    sinkAvrcpNextGroupRelease

DESCRIPTION
    Send a Next Group Release command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpNextGroupRelease(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Next Group Release\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) && isAvrcpGroupNavigationEnabled(Index))
    {
        avrcpSendControlMessage(AVRCP_CTRL_NEXT_GROUP_RELEASE, Index);
    }
}


/*************************************************************************
NAME
    sinkAvrcpPreviousGroupPress

DESCRIPTION
    Send a Previous Group Press command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpPreviousGroupPress(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Previous Group Press\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) && isAvrcpGroupNavigationEnabled(Index))
    {
        avrcpSendControlMessage(AVRCP_CTRL_PREVIOUS_GROUP_PRESS, Index);
    }
}

/*************************************************************************
NAME
    sinkAvrcpPreviousGroupRelease

DESCRIPTION
    Send a Previous Group Release command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpPreviousGroupRelease(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Previous Group Release\n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) && isAvrcpGroupNavigationEnabled(Index))
    {
        avrcpSendControlMessage(AVRCP_CTRL_PREVIOUS_GROUP_RELEASE, Index);
    }
}

/****************************************************************************
* NAME
* sinkAvrcpSourceFromData
*
* DESCRIPTION
*  Create a Source from the passed memory location.
*
* RETURNS
*    void
*******************************************************************************/
Source sinkAvrcpSourceFromData(SinkAvrcpCleanUpTask *avrcp_cleanup, u8 *data, u16 length)
{
    /* Create a source from the data */
    Source src = StreamRegionSource(data, length);

    /* Register a task for freeing the data and store a ptr to it */
    avrcp_cleanup->data = data;
    (void)MessageSinkTask(StreamSinkFromSource(src), &avrcp_cleanup->task);

    return src;
}


/*************************************************************************
NAME
    sinkAvrcpGetShuffleType

DESCRIPTION
    Map the user requested shuffle type to spec value

**************************************************************************/
u16 sinkAvrcpGetShuffleType(avrcp_shuffle_t type)
{
    u16 retval = 0;

    switch(type)
    {
        case AVRCP_SHUFFLE_OFF:
            LOGD("AVRCP_PLAYER_VALUE_SHUFFLE_OFF\n");
            retval = AVRCP_PLAYER_VALUE_SHUFFLE_OFF;
            break;

        case AVRCP_SHUFFLE_ALL_TRACK:
            LOGD("AVRCP_PLAYER_VALUE_SHUFFLE_ALL\n");
            retval = AVRCP_PLAYER_VALUE_SHUFFLE_ALL;
            break;

        case AVRCP_SHUFFLE_GROUP:
            LOGD("AVRCP_PLAYER_VALUE_SHUFFLE_GROUP\n");
            retval = AVRCP_PLAYER_VALUE_SHUFFLE_GROUP;
            break;

        default:
            /* Not supported. */
            break;
    }

    return retval;
}

/*************************************************************************
NAME
    sinkAvrcpShuffle

DESCRIPTION
    Send a Shuffle command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpShuffle(avrcp_shuffle_t type)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Shuffle \n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) )
    {
#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS

        if ((isAvrcpPlayerApplicationSettingsSupported(Index)) &&
            (theSink.avrcp_link_data->event_capabilities[Index] & (1 << avrcp_event_player_app_setting_changed))
            && (type != 0))
        {
            Source src;

            u8 *data = (u8 *) PanicUnlessMalloc(AVRCP_PLAYER_APP_SETTINGS_DATA_LEN);/*Freed by cleanup task*/

            data[0] = AVRCP_PLAYER_ATTRIBUTE_SHUFFLE;
            data[1] = sinkAvrcpGetShuffleType(type);

            LOGD("AVRCP: Avrcp Shuffle Command Sent \n");
            src = sinkAvrcpSourceFromData(&theSink.avrcp_link_data->dataCleanUpTask[Index], data, AVRCP_PLAYER_APP_SETTINGS_DATA_LEN);
            AvrcpSetAppValueRequest(theSink.avrcp_link_data->avrcp[Index], AVRCP_PLAYER_APP_SETTINGS_DATA_LEN, src);
        }
#endif
    }
}


/*************************************************************************
NAME
    sinkAvrcpGetRepeatType

DESCRIPTION
    Map the user requested repeat type to spec value

**************************************************************************/
u16 sinkAvrcpGetRepeatType(avrcp_repeat_t type)
{
    u16 retval = 0;

    switch(type)
    {
        case AVRCP_REPEAT_OFF:
            LOGD("AVRCP_PLAYER_VALUE_REPEAT_MODE_OFF\n");
            retval = AVRCP_PLAYER_VALUE_REPEAT_MODE_OFF;
            break;

        case AVRCP_REPEAT_SINGLE_TRACK:
            LOGD("AVRCP_PLAYER_VALUE_REPEAT_MODE_SINGLE\n");
            retval = AVRCP_PLAYER_VALUE_REPEAT_MODE_SINGLE;
            break;

        case AVRCP_REPEAT_ALL_TRACK:
            LOGD("AVRCP_PLAYER_VALUE_REPEAT_MODE_ALL\n");
            retval = AVRCP_PLAYER_VALUE_REPEAT_MODE_ALL;
            break;

        case AVRCP_REPEAT_GROUP:
            LOGD("AVRCP_PLAYER_VALUE_REPEAT_MODE_GROUP\n");
            retval = AVRCP_PLAYER_VALUE_REPEAT_MODE_GROUP;
            break;

        default:
            /* Not supported. */
            break;
    }

    return retval;
}

/*************************************************************************
NAME
    sinkAvrcpRepeat

DESCRIPTION
    Send a Repeat command to the device with the currently active AVRCP connection.

**************************************************************************/
void sinkAvrcpRepeat(avrcp_repeat_t type)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Repeat \n");

    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1) )
    {
#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
        if ((isAvrcpPlayerApplicationSettingsSupported(Index)) &&
            (theSink.avrcp_link_data->event_capabilities[Index] & (1 << avrcp_event_player_app_setting_changed))
            && (type != 0))
        {
            Source src;

            u8 *data = (u8 *) PanicUnlessMalloc(AVRCP_PLAYER_APP_SETTINGS_DATA_LEN);/*Freed by cleanup task*/

            data[0] = AVRCP_PLAYER_ATTRIBUTE_REPEAT_MODE;
            data[1] = sinkAvrcpGetRepeatType(type);

            LOGD("AVRCP: Avrcp Repeat Command Sent \n");
            src = sinkAvrcpSourceFromData(&theSink.avrcp_link_data->dataCleanUpTask[Index], data, AVRCP_PLAYER_APP_SETTINGS_DATA_LEN);
            AvrcpSetAppValueRequest(theSink.avrcp_link_data->avrcp[Index], AVRCP_PLAYER_APP_SETTINGS_DATA_LEN, src);
        }
#endif
    }
}

/*************************************************************************
NAME
    sinkAvrcpToggleActiveConnection

DESCRIPTION
    Toggles between active AVRCP connections.

**************************************************************************/
void sinkAvrcpToggleActiveConnection(void)
{
    u16 start_index = sinkAvrcpGetActiveConnection();
    u16 search_index = start_index + 1;

    while (search_index != start_index)
    {
        if (search_index >= MAX_AVRCP_CONNECTIONS)
            search_index = 0;
        if (theSink.avrcp_link_data->connected[search_index])
            break;
        search_index++;
    }

    AVRCP_DEBUG(("AVRCP: Toggle Active connection[%d -> %d]\n", start_index,
                search_index))

    /* switch to new connection if required */
    sinkAvrcpSetActiveConnectionFromIndex(search_index);
}


/*************************************************************************
NAME
    sinkAvrcpVolumeStepChange

DESCRIPTION
    The volume has increased or decreased, so update the absolute volume (AVRCP 1.5) and notify
    the remote device if this was a local change of volume, and it requested to be notified.

**************************************************************************/
void sinkAvrcpVolumeStepChange(const volume_direction operation, const u16 step_change, const u16 no_of_steps)
{
    u16 Index = sinkAvrcpGetActiveConnection();
    u16 avrcp_volume = theSink.avrcp_link_data->absolute_volume[Index];
    u16 avrcp_step_change = ((step_change * AVRCP_MAX_ABS_VOL) / (no_of_steps - 1));

#define isGreaterThanMax()                      (avrcp_volume > AVRCP_MAX_ABS_VOL)
#define lessThanSingleStepRemainingBeforeMax()  ((AVRCP_MAX_ABS_VOL / (no_of_steps-1)) > (AVRCP_MAX_ABS_VOL - avrcp_volume))
#define isLessThanMin()                         (((i16)avrcp_volume) < AVRCP_MIN_ABS_VOL)
#define lessThanSingleStepRemainingBeforeMin()  ((AVRCP_MAX_ABS_VOL / (no_of_steps-1)) > (avrcp_volume - AVRCP_MIN_ABS_VOL))
#define localVolumeLimitIndicated()             (avrcp_step_change == 0)

    LOGD("AVRCP: sinkAvrcpVolumeStepChange dir:[%d] step[%d]\n", operation, avrcp_step_change);

    if (theSink.avrcp_link_data->connected[Index])
    {
        if(operation == increase_volume)
        {
            avrcp_volume += avrcp_step_change;

            if(isGreaterThanMax() || localVolumeLimitIndicated() ||
               lessThanSingleStepRemainingBeforeMax())
            {
                avrcp_volume = AVRCP_MAX_ABS_VOL;
            }
        }
        else if(operation == decrease_volume)
        {
            avrcp_volume -= avrcp_step_change;

            if(isLessThanMin() || localVolumeLimitIndicated() ||
               lessThanSingleStepRemainingBeforeMin())
            {
                avrcp_volume = AVRCP_MIN_ABS_VOL;
            }
        }

        AVRCP_DEBUG(("AVRCP: Avrcp Local Vol change from [%d] to [%d]\n",
                     theSink.avrcp_link_data->absolute_volume[Index], avrcp_volume));

        theSink.avrcp_link_data->absolute_volume[Index] = avrcp_volume;

        if (theSink.avrcp_link_data->registered_events[Index] & (1 << avrcp_event_volume_changed))
        {
            LOGD("  Notify remote device\n");
            AvrcpEventVolumeChangedResponse(theSink.avrcp_link_data->avrcp[Index],
                                    avctp_response_changed,
                                    avrcp_volume);
            /* reset registered volume notification */
            theSink.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_volume_changed);
        }
    }
#undef isGreaterThanMax
#undef lessThanSingleStepRemainingBeforeMax
#undef isLessThanMin
#undef lessThanSingleStepRemainingBeforeMin
#undef localVolumeLimitIndicated
}

/*************************************************************************
NAME
    sinkAvrcpSetLocalVolume

DESCRIPTION
    The volume has been locally set so update the absolute volume (AVRCP 1.5) and notify
    the remote device if this was a local change of volume, and it requested to be notified.

**************************************************************************/
void sinkAvrcpSetLocalVolume(u16 Index, u16 a2dp_volume)
{
    /* convert digital volume into avrcp scaled volume 0 to 127 */
    theSink.avrcp_link_data->absolute_volume[Index] = ((a2dp_volume * AVRCP_MAX_ABS_VOL)/sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps);

    if (theSink.avrcp_link_data->registered_events[Index] & (1 << avrcp_event_volume_changed))
    {
        LOGD("  Notify remote device [local:%d][abs:%d]\n", a2dp_volume, theSink.avrcp_link_data->absolute_volume[Index]);
        AvrcpEventVolumeChangedResponse(theSink.avrcp_link_data->avrcp[Index],
                                        avctp_response_changed,
                                        theSink.avrcp_link_data->absolute_volume[Index]);
        /* reset registered volume notification */
        theSink.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_volume_changed);
    }
}


/*************************************************************************
NAME
    sinkAvrcpGetIndexFromInstance

DESCRIPTION
    Retrieve the correct AVRCP connection index based on the AVRCP library instance pointer.

RETURNS
    Returns TRUE if the AVRCP connection was found, FASLE otherwise.
    The actual connection index is returned in the Index variable.

**************************************************************************/
bool sinkAvrcpGetIndexFromInstance(AVRCP *avrcp, u16 *Index)
{
    u8 i;

    /* go through Avrcp connections looking for device_id match */
    for_all_avrcp(i)
    {
        /* if the avrcp link is connected check for its AVRCP pointer */
        if (theSink.avrcp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successful match found */
            if (avrcp && (theSink.avrcp_link_data->avrcp[i] == avrcp))
            {
                *Index = i;
                LOGD("AVRCP: getIndex = %d\n", i);
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpGetIndexFromBdaddr

DESCRIPTION
    Retrieve the correct AVRCP connection index based on the Bluetooth address.

RETURNS
    Returns TRUE if the AVRCP connection was found, FALSE otherwise.
    The actual connection index is returned in the Index variable.

**************************************************************************/
bool sinkAvrcpGetIndexFromBdaddr(const bdaddr *bd_addr, u16 *Index, bool require_connection)
{
    u8 i;

    /* go through Avrcp connections looking for device_id match */
    for_all_avrcp(i)
    {
        if ((require_connection && theSink.avrcp_link_data->connected[i]) || !require_connection)
        {
            if (BdaddrIsSame(&theSink.avrcp_link_data->bd_addr[i], bd_addr))
            {
                *Index = i;
                LOGD("AVRCP: getIndex = %d\n", i);
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpSetActiveConnectionFromBdaddr

DESCRIPTION
    Sets the active AVRCP connection based on Bluetooth address.

**************************************************************************/
void sinkAvrcpSetActiveConnectionFromBdaddr(const bdaddr *bd_addr)
{
    u16 Index;

    if (sinkAvrcpGetIndexFromBdaddr(bd_addr, &Index, TRUE))
    {
        sinkAvrcpSetActiveConnectionFromIndex(Index);
    }
}

/*************************************************************************
NAME
    sinkAvrcpSetActiveConnectionFromIndex

DESCRIPTION
    Sets the active AVRCP connection based on Bluetooth address.

**************************************************************************/
static void sinkAvrcpSetActiveConnectionFromIndex(u16 new_index)
{
    u16 current_index = theSink.avrcp_link_data->active_avrcp;

    if (current_index != new_index)
    {
        sinkAvrcpFlushAndResetQueue(current_index);
        LOGD("AVRCP: Set Active connection[%d]\n", new_index);
        /* set new active connection */
        theSink.avrcp_link_data->active_avrcp = new_index;
#ifdef ENABLE_AVRCP_NOW_PLAYING
        /* sources have been switched, update the current playing track */
        sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(new_index, FALSE);
#endif
    }
}

/*************************************************************************
NAME
    sinkAvrcpFlushAndResetQueue

DESCRIPTION
    Flushes any pending messages, frees memory tied up in them
    and resets the message queue.

**************************************************************************/
static void sinkAvrcpFlushAndResetQueue(u16 index)
{
    /* remove pending messages */
    MessageFlushTask(&theSink.avrcp_link_data->avrcp_ctrl_handler[index]);
    sinkAvrcpBrowsingFlushHandlerTask(index);
    sinkAvrcpBrowsingChannelDisconnectRequest(theSink.avrcp_link_data->avrcp[index]);

    if (theSink.avrcp_link_data->vendor_data[index])
    {
        /*
         * There is a memory buffer still pointed to that
         * now needs freeing
         */
        free(theSink.avrcp_link_data->vendor_data[index]);
        theSink.avrcp_link_data->vendor_data[index] = NULL;
    }

    /* reset the message queueing mechanism */
    theSink.avrcp_link_data->pending_cmd[index] = FALSE;
    theSink.avrcp_link_data->cmd_queue_size[index] = 0;
}

/*************************************************************************
NAME
    sinkAvrcpGetActiveConnection

DESCRIPTION
    Gets the active AVRCP connection.

**************************************************************************/
u16 sinkAvrcpGetActiveConnection(void)
{
    return theSink.avrcp_link_data->active_avrcp;
}


/*************************************************************************
NAME
    sinkAvrcpUpdateActiveConnection

DESCRIPTION
    Updates the active AVRCP connection based on what is currently connected.

**************************************************************************/
void sinkAvrcpUpdateActiveConnection(void)
{
    u8 i = 0;
    u16 activeIndex = sinkAvrcpGetActiveConnection();
#ifdef ENABLE_PEER
    u16 peerIndex = 0;
#endif

    if (!theSink.avrcp_link_data->connected[activeIndex])
    {
        for_all_avrcp(i)
        {

#ifdef ENABLE_PEER
            /* If there is a peer device connected but the single device operation is not set then don't set the index as active avrcp.*/
            if ( (theSink.avrcp_link_data->connected[i]) && (!isAvrcpIndexPeer(i, &peerIndex) || theSink.features.TwsSingleDeviceOperation))
#else
            if (theSink.avrcp_link_data->connected[i] )
#endif
            {
                theSink.avrcp_link_data->active_avrcp = i;
                break;
            }
        }
    }

#ifdef ENABLE_PEER
    else
    {
        u16 avIndex;
        /* If the active connction is a peer device and if there is a local AG connected and if this sink is not streaming then update the
        active connection to that local AG.*/
        if(isAvrcpIndexPeer(activeIndex, NULL) && getAvAvrcpIndex (&avIndex) &&
           ((theSink.avrcp_link_data->play_status[activeIndex] == avrcp_play_status_stopped) ||
           (theSink.avrcp_link_data->play_status[activeIndex] == avrcp_play_status_paused) ))
        {
            sinkAvrcpSetActiveConnectionFromBdaddr(&theSink.avrcp_link_data->bd_addr[avIndex]);
        }
    }
#endif

}


/*************************************************************************
NAME
    sinkAvrcpGetNumberConnections

DESCRIPTION
    Retrieves the number of active AVRCP connections

**************************************************************************/
u16 sinkAvrcpGetNumberConnections(void)
{
    u8 i = 0;
    u16 connected = 0;

    for_all_avrcp(i)
    {
        if (theSink.avrcp_link_data->connected[i])
            connected++;
    }

    return connected;
}


/*************************************************************************
NAME
    sinkAvrcpSetPlayStatus

DESCRIPTION
    Sets the play status but only for AVRCP 1.0 devices. For AVRCP 1.3 onwards, the remote
    device will notify of changes in play status.

**************************************************************************/
void sinkAvrcpSetPlayStatus(const bdaddr *bd_addr, u16 play_status)
{
    u16 Index;

    if (sinkAvrcpGetIndexFromBdaddr(bd_addr, &Index, TRUE))
    {
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            LOGD("AVRCP: update play status %d\n", play_status);
            theSink.avrcp_link_data->play_status[Index] = play_status;
        }
    }
}


/*************************************************************************
NAME
    sinkAvrcpCheckManualConnectReset

DESCRIPTION
    Checks if the manual AVRCP connect state needs resetting.

**************************************************************************/
bool sinkAvrcpCheckManualConnectReset(bdaddr *bd_addr)
{
    if (theSink.features.avrcp_enabled &&
        theSink.avrcp_link_data->avrcp_manual_connect)
    {
        if (BdaddrIsZero(&theSink.avrcp_link_data->avrcp_play_addr) ||
                ((bd_addr != NULL) &&
                 !BdaddrIsZero(&theSink.avrcp_link_data->avrcp_play_addr) &&
                 BdaddrIsSame(&theSink.avrcp_link_data->avrcp_play_addr, bd_addr))
        )
        {
            theSink.avrcp_link_data->avrcp_manual_connect = FALSE;
            BdaddrSetZero(&theSink.avrcp_link_data->avrcp_play_addr);
            return TRUE;
        }
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpDisplayMediaAttributes

DESCRIPTION
    Displays the Media Attribute text (title, artist etc.) for the supplied attribute ID.
    The text is contained in data with length attribute_length.

**************************************************************************/
void sinkAvrcpDisplayMediaAttributes(u32 attribute_id, u16 attribute_length, const u8 *data)
{
    switch (attribute_id)
    {
        case AVRCP_MEDIA_ATTRIBUTE_TITLE:
        {
            LOGD("  Title: ");
#ifdef ENABLE_DISPLAY
            displayShowText((char*)data,  attribute_length, 1, DISPLAY_TEXT_SCROLL_SCROLL, 500, 2000, FALSE, 0);
#endif
        }
            break;
        case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
            LOGD("  Artist: ");
            break;
        case AVRCP_MEDIA_ATTRIBUTE_ALBUM:
            LOGD("  Album: ");
            break;
        case AVRCP_MEDIA_ATTRIBUTE_NUMBER:
            LOGD("  Number: ");
            break;
        case AVRCP_MEDIA_ATTRIBUTE_TOTAL_NUMBER:
            LOGD("  Total Number: ");
            break;
        case AVRCP_MEDIA_ATTRIBUTE_GENRE:
            LOGD("  Genre: ");
            break;
        case AVRCP_MEDIA_ATTRIBUTE_PLAYING_TIME:
            LOGD("  Playing time (ms): ");
            break;
        default:
            break;
    }

#ifdef DEBUG_AVRCP
    {
        u16 i;
        for (i = 0; i < attribute_length; i++)
        {
            LOGD("%c", data[i]);
        }
        LOGD("\n");
    }
#endif
}


#ifdef ENABLE_AVRCP_NOW_PLAYING
/*************************************************************************
NAME
    sinkAvrcpRetrieveNowPlayingRequest

DESCRIPTION
    Retrieves Now Playing Track from remote device.

        track_index_high - upper 32 bits of unique track UID
        track_index_high - lower 32 bits of unique track UID
        full_attributes - TRUE if all media attributes should be returned, FALSE if only the basic media attributes should be returned

**************************************************************************/
void sinkAvrcpRetrieveNowPlayingRequest(u32 track_index_high, u32 track_index_low, bool full_attributes)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    LOGD("AVRCP: Avrcp Retrieve Now Playing\n");

    if (theSink.avrcp_link_data->connected[Index])
    {
        if (sinkAvrcpBrowsingIsSupported(Index) && ((track_index_high != 0x0) || (track_index_low != 0x0)))
        {
            /* open browsing channel and use Browsing:GetItemAttributes */
            sinkAvrcpBrowsingRetrieveNowPlayingTrackRequest(Index, track_index_high, track_index_low, full_attributes);
        }
        else
        {
            /* use GetElementAttributes command */
            sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(Index, full_attributes);
        }
    }
}


/*************************************************************************
NAME
    sinkAvrcpRetrieveNowPlayingNoBrowsingRequest

DESCRIPTION
    Retrieves Now Playing track from remote device without using browsing channel.

        Index - AVRCP connection instance, usually the active one
        full_attributes - TRUE if all media attributes should be returned, FALSE if only the basic media attributes should be returned

**************************************************************************/
void sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(u16 Index, bool full_attributes)
{
    u16 size_media_attributes;
    Source src_media_attributes;

    if (full_attributes)
    {
        size_media_attributes = sizeof(avrcp_retrieve_media_attributes_full);
        src_media_attributes = StreamRegionSource(avrcp_retrieve_media_attributes_full, size_media_attributes);
    }
    else
    {
        size_media_attributes = sizeof(avrcp_retrieve_media_attributes_basic);
        src_media_attributes = StreamRegionSource(avrcp_retrieve_media_attributes_basic, size_media_attributes);
    }

    AvrcpGetElementAttributesRequest(theSink.avrcp_link_data->avrcp[Index],
                                     0,
                                     0,
                                     size_media_attributes,
                                     src_media_attributes);

    LOGD("AVRCP: Avrcp Retrieve Now Playing no Browsing, size:%d\n", size_media_attributes);
}

#endif /* ENABLE_AVRCP_NOW_PLAYING */


#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
/*************************************************************************
NAME
    sinkAvrcpListPlayerAttributesRequest

DESCRIPTION
    Retrieves the list of Player Attributes from the remote device.

**************************************************************************/
bool sinkAvrcpListPlayerAttributesRequest(void)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp List Player Attributes\n");

        /* register to get notifications of changes in application settings */
        AvrcpRegisterNotificationRequest(theSink.avrcp_link_data->avrcp[Index], avrcp_event_player_app_setting_changed, 0);

        /* retrieve the attributes */
        AvrcpListAppAttributeRequest(theSink.avrcp_link_data->avrcp[Index]);

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpListPlayerAttributesTextRequest

DESCRIPTION
    Retrieves the list of Player Attributes Text from the remote device.

        size_attributes - number of IDs contained in attributes
        attributes - the IDs to return the associated text for

**************************************************************************/
bool sinkAvrcpListPlayerAttributesTextRequest(u16 size_attributes, Source attributes)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp List Player Attributes Text\n");

        AvrcpGetAppAttributeTextRequest(theSink.avrcp_link_data->avrcp[Index], size_attributes, attributes);

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpListPlayerValuesRequest

DESCRIPTION
    Retrieves the list of Player Values for the specified Attribute from the remote device.

        attribute_id - return all the possible values for this attribute ID

**************************************************************************/
bool sinkAvrcpListPlayerValuesRequest(u8 attribute_id)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp List Player Values\n");

        AvrcpListAppValueRequest(theSink.avrcp_link_data->avrcp[Index], attribute_id);

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpGetPlayerValueRequest

DESCRIPTION
    Retrieves the current Player Value for the specified Attributes from the remote device.

        size_attributes - number of attribute IDs contained in attributes
        attributes - the IDs to get the associated value for

**************************************************************************/
bool sinkAvrcpGetPlayerValueRequest(u16 size_attributes, Source attributes)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp Get Player Value\n");

        AvrcpGetAppValueRequest(theSink.avrcp_link_data->avrcp[Index], size_attributes, attributes);

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpListPlayerValueTextRequest

DESCRIPTION
    Retrieves the current Player Value Text for the specified Attributes from the remote device.

        attribute_id - the attribute ID for which to retrieve the value text for
        size_values - number of value IDs contained in values
        values - the value IDs to get the associated text for

**************************************************************************/
bool sinkAvrcpListPlayerValueTextRequest(u16 attribute_id, u16 size_values, Source values)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp Get Player Value Text\n");

        AvrcpGetAppValueTextRequest(theSink.avrcp_link_data->avrcp[Index], attribute_id, size_values, values);

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
NAME
    sinkAvrcpSetPlayerValueRequest

DESCRIPTION
    Sets the current Player Value for the specified Attributes on the remote device.

        size_attributes - number of {attribute, value} pairs contained in attributes
        attributes - the {attribute, value} pairs to set the value for

**************************************************************************/
bool sinkAvrcpSetPlayerValueRequest(u16 size_attributes, Source attributes)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp Set Player Value\n");

        AvrcpSetAppValueRequest(theSink.avrcp_link_data->avrcp[Index], size_attributes, attributes);

        return TRUE;
    }
    return FALSE;
}

/*************************************************************************
NAME
    sinkAvrcpInformBatteryStatusRequest

DESCRIPTION
    Informs the TG of the battery status

        status - current status of battery

**************************************************************************/
bool sinkAvrcpInformBatteryStatusRequest(avrcp_battery_status status)
{
    u16 Index = sinkAvrcpGetActiveConnection();

    if (isAvrcpPlayerApplicationSettingsSupported(Index))
    {
        LOGD("AVRCP: Avrcp Inform Battery Status\n");

        AvrcpInformBatteryStatusRequest(theSink.avrcp_link_data->avrcp[Index], status);

        return TRUE;
    }
    return FALSE;
}
#endif /* ENABLE_AVRCP_PLAYER_APP_SETTINGS */





/*************************************************************************
NAME
    sinkAvrcpPauseRequest

DESCRIPTION
    attemtps to pause the media stream of the passed in avrcp connection index

RETURNS
    success or failure status

**************************************************************************/
bool sinkAvrcpPlayPauseRequest(u16 Index, avrcp_remote_actions action)
{
    bool status = FALSE;

    LOGD("AVRCP: Avrcp Play or Pause to %x\n",Index);

    /* ensure avrcp is connected */
    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        /* pause requested ? */
        if(action == AVRCP_PAUSE)
        {
            LOGD("AVRCP: Avrcp Pause to %x\n",Index);
            avrcpSendControlMessage(AVRCP_CTRL_PAUSE_PRESS, Index);
            avrcpSendControlMessage(AVRCP_CTRL_PAUSE_RELEASE, Index);
            if (!isAvrcpPlaybackStatusSupported(Index))
                theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_paused;
            /* paused successfully */
            status = TRUE;
        }
        /* play requested ? */
        else if(action == AVRCP_PLAY)
        {
            LOGD("AVRCP: Avrcp Play to %x\n",Index);
            avrcpSendControlMessage(AVRCP_CTRL_PLAY_PRESS, Index);
            avrcpSendControlMessage(AVRCP_CTRL_PLAY_RELEASE, Index);
            if (!isAvrcpPlaybackStatusSupported(Index))
                theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_playing;
            /* resume play successfully */
            status = TRUE;
        }
    }
    return status;
}


/*************************************************************************
NAME
    sinkAvrcpStopRequest

DESCRIPTION
    attemtps to stop the media stream of the passed in avrcp connection index

RETURNS
    success or failure status

**************************************************************************/
bool sinkAvrcpStopRequest(u16 Index)
{
    bool status = FALSE;

    /* ensure avrcp is connected */
    if (theSink.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        LOGD("AVRCP: Avrcp Stop to %x\n",Index);
        avrcpSendControlMessage(AVRCP_CTRL_STOP_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_STOP_RELEASE, Index);

        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            theSink.avrcp_link_data->play_status[Index] = avrcp_play_status_stopped;
        }

        /* stopped successfully */
        status = TRUE;
    }
    return status;
}


/*************************************************************************
NAME
    sinkAvrcpVendorUniquePassthroughRequest

DESCRIPTION
    attemtps to pause the media stream of the passed in avrcp connection index

RETURNS
    FALSE if command could not be sent, TRUE otherwise

**************************************************************************/
bool sinkAvrcpVendorUniquePassthroughRequest (u16 avrcp_index, u16 cmd_id, u16 size_data, const u8 *data)
{
#ifdef ENABLE_PEER
    u16 a2dp_index;

    if ( !a2dpGetPeerIndex(&a2dp_index) || !(theSink.a2dp_link_data->peer_features[a2dp_index] & remote_features_peer_avrcp_target) )
    {   /* The Peer does not accept custom commands */
        LOGD("AVRCP: No peer target\n");
        return FALSE;
    }
#endif

    if ( !theSink.features.avrcp_enabled || !(theSink.avrcp_link_data && theSink.avrcp_link_data->connected[avrcp_index]) )
    {   /* We don't support AVRCP */
        LOGD("AVRCP: No AVRCP\n");
        return FALSE;
    }

    LOGD("AVRCP: sinkAvrcpVendorUniquePassthroughRequest avrcp_index=%u  cmd_id=%u  size_data=%u  data=0x%X\n",avrcp_index,cmd_id,size_data,(u16)data);

    if (!theSink.avrcp_link_data->pending_cmd[avrcp_index] && !theSink.avrcp_link_data->vendor_data[avrcp_index])
    {
        Source vendor_source;

        /* Need to retain this data until AVRCP library informs us that it has been sent */
        theSink.avrcp_link_data->vendor_data[avrcp_index] = (u8 *)malloc(VENDOR_CMD_TOTAL_SIZE(size_data));

        if (!theSink.avrcp_link_data->vendor_data[avrcp_index])
        {
            return FALSE;
        }

        theSink.avrcp_link_data->vendor_data[avrcp_index][0] = (u8)(cmd_id >> 8);
        theSink.avrcp_link_data->vendor_data[avrcp_index][1] = (u8)(cmd_id & 0xFF);

        if(size_data != 0)
        {
            memcpy(&theSink.avrcp_link_data->vendor_data[avrcp_index][2], data, size_data);
        }

        vendor_source = StreamRegionSource( theSink.avrcp_link_data->vendor_data[avrcp_index], VENDOR_CMD_TOTAL_SIZE(size_data) );

        LOGD("AVRCP:    Issuing...\n");

        theSink.avrcp_link_data->pending_cmd[avrcp_index] = TRUE;
        AvrcpPassthroughRequest(theSink.avrcp_link_data->avrcp[avrcp_index],
                                subunit_panel,
                                0,
                                0,
                                opid_vendor_unique,
                                VENDOR_CMD_TOTAL_SIZE(size_data),
                                vendor_source);
    }
    else
    {
        MAKE_AVRCP_MESSAGE_WITH_LEN( SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ, size_data);

        LOGD("AVRCP:    Requeueing...\n");
        message->avrcp_index = avrcp_index;
        message->cmd_id = cmd_id;
        message->size_data = size_data;
        memcpy(message->data, data, size_data);

        MessageSendConditionally( &theSink.task, SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ, message, (u16 *)&theSink.avrcp_link_data->pending_cmd[avrcp_index]);
    }

    return TRUE;
}


/*************************************************************************
NAME
    sinkAvrcpHandleMessage

DESCRIPTION
    Handles AVRCP library messages.

**************************************************************************/
void sinkAvrcpHandleMessage(Task task, MessageId id, Message message)
{
    LOGD("AVRCP_MSG id=%x : \n", id);
    LOGD("Allocations=%d : \n", VmGetAvailableAllocations());

    switch (id)
    {

/******************/
/* INITIALISATION */
/******************/

        case AVRCP_INIT_CFM:
            LOGD("AVRCP_INIT_CFM :\n");
            sinkAvrcpInitComplete((AVRCP_INIT_CFM_T *) message);
            break;

/******************************/
/* CONNECTION / DISCONNECTION */
/******************************/

        case AVRCP_CONNECT_CFM:
            LOGD("AVRCP_CONNECT_CFM :\n");
            handleAvrcpConnectCfm((AVRCP_CONNECT_CFM_T *) message);
            break;
        case AVRCP_CONNECT_IND:
            LOGD("AVRCP_CONNECT_IND :\n");
            handleAvrcpConnectInd((AVRCP_CONNECT_IND_T *) message);
            break;
        case AVRCP_DISCONNECT_IND:
            LOGD("AVRCP_DISCONNECT_IND :\n");
            handleAvrcpDisconnectInd((AVRCP_DISCONNECT_IND_T *) message);
            break;

/*****************/
/* AV/C COMMANDS */
/*****************/

        case AVRCP_PASSTHROUGH_CFM:
            LOGD("AVRCP_PASSTHROUGH_CFM :\n");
            handleAvrcpPassthroughCfm(((AVRCP_PASSTHROUGH_CFM_T *) message)->avrcp);
            break;
        case AVRCP_PASSTHROUGH_IND:
            LOGD("AVRCP_PASSTHROUGH_IND :\n");
            handleAvrcpPassthroughInd((AVRCP_PASSTHROUGH_IND_T *) message);
            break;
        case AVRCP_UNITINFO_IND:
            LOGD("AVRCP_UNITINFO_IND :\n");
            handleAvrcpUnitInfoInd((AVRCP_UNITINFO_IND_T *) message);
            break;
        case AVRCP_SUBUNITINFO_IND:
            LOGD("AVRCP_SUBUNITINFO_IND :\n");
            handleAvrcpSubUnitInfoInd((AVRCP_SUBUNITINFO_IND_T *) message);
            break;
        case AVRCP_VENDORDEPENDENT_IND:
            LOGD("AVRCP_VENDORDEPENDENT_IND :\n");
            handleAvrcpVendorDependentInd((AVRCP_VENDORDEPENDENT_IND_T *) message);
            break;
        case AVRCP_NEXT_GROUP_CFM:
            LOGD("AVRCP_NEXT_GROUP_CFM :\n");
            handleAvrcpPassthroughCfm(((AVRCP_NEXT_GROUP_CFM_T *) message)->avrcp); /* treat as general Passthrough cmd */
            break;
        case AVRCP_PREVIOUS_GROUP_CFM:
            LOGD("AVRCP_PREVIOUS_GROUP_CFM :\n");
            handleAvrcpPassthroughCfm(((AVRCP_PREVIOUS_GROUP_CFM_T *) message)->avrcp); /* treat as general Passthrough cmd */
            break;

/******************/
/* AVRCP Metadata */
/******************/

        case AVRCP_GET_CAPS_IND:
            LOGD("AVRCP_GET_CAPS_IND :\n");
            handleAvrcpGetCapsInd((AVRCP_GET_CAPS_IND_T *) message);
            break;
        case AVRCP_GET_CAPS_CFM:
            LOGD("AVRCP_GET_CAPS_CFM :\n");
            handleAvrcpGetCapsCfm((AVRCP_GET_CAPS_CFM_T *) message);
            break;
        case AVRCP_REGISTER_NOTIFICATION_IND:
            LOGD("AVRCP_REGISTER_NOTIFICATION_IND :\n");
            handleAvrcpRegisterNotificationInd((AVRCP_REGISTER_NOTIFICATION_IND_T *) message);
            break;
        case AVRCP_REGISTER_NOTIFICATION_CFM:
            LOGD("AVRCP_REGISTER_NOTIFICATION_CFM :\n");
            break;
        case AVRCP_SET_ABSOLUTE_VOLUME_IND:
            LOGD("AVRCP_SET_ABSOLUTE_VOLUME_IND :\n");
            handleAvrcpSetAbsVolInd((AVRCP_SET_ABSOLUTE_VOLUME_IND_T *) message);
            break;
        case AVRCP_GET_PLAY_STATUS_CFM:
            LOGD("AVRCP_GET_PLAY_STATUS_CFM :\n");
            handleAvrcpGetPlayStatusCfm((AVRCP_GET_PLAY_STATUS_CFM_T *) message);
            break;
        case AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND:
            LOGD("AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND :\n");
            handleAvrcpPlayStatusChangedInd((AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND_T *) message);
            break;
        case AVRCP_EVENT_TRACK_CHANGED_IND:
            LOGD("AVRCP_EVENT_TRACK_CHANGED_IND :\n");
            handleAvrcpTrackChangedInd((AVRCP_EVENT_TRACK_CHANGED_IND_T *) message);
            break;
        case AVRCP_EVENT_PLAYBACK_POS_CHANGED_IND:
            LOGD("AVRCP_EVENT_PLAYBACK_POS_CHANGED_IND :\n");
            handleAvrcpPlaybackPosChangedInd((AVRCP_EVENT_PLAYBACK_POS_CHANGED_IND_T *) message);
            break;
        case AVRCP_EVENT_VOLUME_CHANGED_IND:
            LOGD("AVRCP_EVENT_VOLUME_CHANGED_IND :\n");
            handleAvrcpVolumeChangedInd((AVRCP_EVENT_VOLUME_CHANGED_IND_T *) message);
            break;
        case AVRCP_SET_ABSOLUTE_VOLUME_CFM:
            LOGD("AVRCP_SET_ABSOLUTE_VOLUME_CFM :\n");
            handleAvrcpSetAbsoluteVolumeCfm((AVRCP_SET_ABSOLUTE_VOLUME_CFM_T *) message);
            break;
        case AVRCP_EVENT_TRACK_REACHED_END_IND:
            LOGD("AVRCP_EVENT_TRACK_REACHED_END_IND :\n"); /* notification not currently used */
            break;
        case AVRCP_EVENT_TRACK_REACHED_START_IND:
            LOGD("AVRCP_EVENT_TRACK_REACHED_START_IND :\n"); /* notification not currently used */
            break;
        case AVRCP_EVENT_BATT_STATUS_CHANGED_IND:
            LOGD("AVRCP_EVENT_BATT_STATUS_CHANGED_IND :\n"); /* notification not currently used */
            break;
        case AVRCP_EVENT_SYSTEM_STATUS_CHANGED_IND:
            LOGD("AVRCP_EVENT_SYSTEM_STATUS_CHANGED_IND :\n"); /* notification not currently used */
            break;
        case AVRCP_REQUEST_CONTINUING_RESPONSE_CFM:
            LOGD("AVRCP_REQUEST_CONTINUING_RESPONSE_CFM :\n");
            break;
        case AVRCP_ABORT_CONTINUING_RESPONSE_CFM:
            LOGD("AVRCP_ABORT_CONTINUING_RESPONSE_CFM :\n");
            break;
#ifdef ENABLE_AVRCP_NOW_PLAYING
        case AVRCP_GET_ELEMENT_ATTRIBUTES_CFM:
            LOGD("AVRCP_GET_ELEMENT_ATTRIBUTES_CFM :\n");
            handleAvrcpGetElementAttributesCfm((AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_T *) message);
            break;
        case AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED_IND:
            LOGD("AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED_IND :\n");
            handleAvrcpNowPlayingContentChangedInd((AVRCP_EVENT_NOW_PLAYING_CONTENT_CHANGED_IND_T *) message);
            break;
        case AVRCP_ADD_TO_NOW_PLAYING_CFM:
            LOGD("AVRCP_ADD_TO_NOW_PLAYING_CFM %d :\n", ((AVRCP_ADD_TO_NOW_PLAYING_CFM_T *)message)->status);
            break;
#endif /* ENABLE_AVRCP_NOW_PLAYING */
#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
        case AVRCP_LIST_APP_ATTRIBUTE_CFM:
            LOGD("AVRCP_LIST_APP_ATTRIBUTE_CFM :\n");
            handleAvrcpListAppAttributeCfm((AVRCP_LIST_APP_ATTRIBUTE_CFM_T *) message);
            break;
        case AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM:
            LOGD("AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM :\n");
            handleAvrcpGetAppAttributeValueTextCfm(AVRCP_GET_APP_ATTRIBUTE_TEXT_PDU_ID, (AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T *) message);
            break;
        case AVRCP_LIST_APP_VALUE_CFM:
            LOGD("AVRCP_LIST_APP_VALUE_CFM :\n");
            handleAvrcpListAppValueCfm((AVRCP_LIST_APP_VALUE_CFM_T *) message);
            break;
        case AVRCP_GET_APP_VALUE_CFM:
            LOGD("AVRCP_GET_APP_VALUE_CFM :\n");
            handleAvrcpGetAppValueCfm((AVRCP_GET_APP_VALUE_CFM_T *) message);
            break;
        case AVRCP_GET_APP_VALUE_TEXT_CFM:
            LOGD("AVRCP_GET_APP_VALUE_TEXT_CFM :\n");
            handleAvrcpGetAppAttributeValueTextCfm(AVRCP_GET_APP_VALUE_TEXT_PDU_ID, (AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T *) message); /* AVRCP_GET_APP_VALUE_TEXT_CFM_T is same structure as AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T */
            break;
        case AVRCP_SET_APP_VALUE_CFM:
            LOGD("AVRCP_SET_APP_VALUE_CFM :\n");
            handleAvrcpSetAppValueCfm((AVRCP_SET_APP_VALUE_CFM_T *) message);
            break;
        case AVRCP_EVENT_PLAYER_APP_SETTING_CHANGED_IND:
            LOGD("AVRCP_EVENT_PLAYER_APP_SETTING_CHANGED_IND :\n");
            sinkAvrcpPlayerAppSettingsChangedInd((AVRCP_EVENT_PLAYER_APP_SETTING_CHANGED_IND_T *)message);
            break;
#endif /* ENABLE_AVRCP_PLAYER_APP_SETTINGS */

/************/
/* BROWSING */
/************/
        case AVRCP_BROWSE_CONNECT_IND:
            LOGD("AVRCP_BROWSE_CONNECT_IND :\n");
            sinkAvrcpBrowsingChannelConnectInd((AVRCP_BROWSE_CONNECT_IND_T *) message);
            break;
        case AVRCP_BROWSE_DISCONNECT_IND:
            LOGD("AVRCP_BROWSE_DISCONNECT_IND :\n");
            sinkAvrcpBrowsingChannelDisconnectInd((AVRCP_BROWSE_DISCONNECT_IND_T *) message);
            break;
        case AVRCP_BROWSE_CONNECT_CFM:
            LOGD("AVRCP_BROWSE_CONNECT_CFM :\n");
            sinkAvrcpBrowsingChannelConnectCfm((AVRCP_BROWSE_CONNECT_CFM_T *) message);
            break;
        case AVRCP_BROWSE_GET_ITEM_ATTRIBUTES_CFM:
            LOGD("AVRCP_BROWSE_GET_ITEM_ATTRIBUTES_CFM :\n");
            sinkAvrcpBrowsingGetItemAttributesCfm((AVRCP_BROWSE_GET_ITEM_ATTRIBUTES_CFM_T *) message);
            break;
        case AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED_IND:
            LOGD("AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED_IND :\n");
            sinkAvrcpBrowsingAddressedPlayerChangedInd((AVRCP_EVENT_ADDRESSED_PLAYER_CHANGED_IND_T *) message);
            break;
        case AVRCP_BROWSE_GET_FOLDER_ITEMS_CFM:
            LOGD("AVRCP_BROWSE_GET_FOLDER_ITEMS_CFM :\n");
            sinkAvrcpBrowsingGetFolderItemsCfm((AVRCP_BROWSE_GET_FOLDER_ITEMS_CFM_T *) message);
            break;

        case AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_CFM:
            LOGD("AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_CFM :\n");
            sinkAvrcpBrowsingGetNumberOfItemsCfm((AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_CFM_T *) message);
            break;
        case AVRCP_SET_ADDRESSED_PLAYER_CFM:
            LOGD("AVRCP_SET_ADDRESSED_PLAYER_CFM %d : Opening of Player started \n", ((AVRCP_SET_ADDRESSED_PLAYER_CFM_T *)message)->status);
            break;
        case AVRCP_BROWSE_SET_PLAYER_CFM:
            LOGD("AVRCP_BROWSE_SET_PLAYER_CFM :\n");
            sinkAvrcpBrowsingSetBrowsedPlayerCfm((AVRCP_BROWSE_SET_PLAYER_CFM_T *) message);
            break;
        case AVRCP_PLAY_ITEM_CFM:
            LOGD("AVRCP_PLAY_ITEM_CFM %d :\n", ((AVRCP_PLAY_ITEM_CFM_T *)message)->status);
            break;
        case AVRCP_BROWSE_CHANGE_PATH_CFM:
            LOGD("AVRCP_BROWSE_CHANGE_PATH_CFM %d :\n", ((AVRCP_BROWSE_CHANGE_PATH_CFM_T *)message)->status);
            sinkAvrcpBrowsingChangePathCfm((AVRCP_BROWSE_CHANGE_PATH_CFM_T *) message);
            break;
        case AVRCP_EVENT_UIDS_CHANGED_IND:
            LOGD("AVRCP_EVENT_UIDS_CHANGED_IND :\n");
            sinkAvrcpBrowsingUIDsChangedInd((AVRCP_EVENT_UIDS_CHANGED_IND_T *)message);
            break;
        case AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED_IND:
            LOGD("AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED_IND :\n");
            sinkAvrcpBrowsingAvailablePlayersChangedInd((AVRCP_EVENT_AVAILABLE_PLAYERS_CHANGED_IND_T *)message);
            break;
        case AVRCP_BROWSE_SEARCH_CFM:
            LOGD("AVRCP_BROWSE_SEARCH_CFM :\n");
            sinkAvrcpBrowsingSearchCfm((AVRCP_BROWSE_SEARCH_CFM_T *)message);
            break;

/********************/
/* LIBRARY SPECIFIC */
/********************/

        case AVRCP_GET_EXTENSIONS_CFM:
            LOGD("AVRCP_GET_EXTENSIONS_CFM :\n");
            handleAvrcpGetExtensionsCfm((AVRCP_GET_EXTENSIONS_CFM_T *) message);
            break;
        case AVRCP_GET_SUPPORTED_FEATURES_CFM:
            LOGD("AVRCP_GET_SUPPORTED_FEATURES_CFM :\n");
            handleAvrcpGetSupportedFeaturesCfm((AVRCP_GET_SUPPORTED_FEATURES_CFM_T *) message);
            break;

/************************/
/* APPLICATION SPECIFIC */
/************************/

        case SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ:
            LOGD("SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ :\n");
            handleSinkAvrcpVendorUniquePassthroughReq((SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ_T *) message);
            break;


/************************/
/* AVRCP TG COMMANDS TO BE HANDLED IN PTS*/
/************************/
        /* Have all the fall through cases for PTS here */
        case AVRCP_BROWSE_GET_FOLDER_ITEMS_IND:
        case AVRCP_SET_ADDRESSED_PLAYER_IND:
        case AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_IND:
        case AVRCP_GET_PLAY_STATUS_IND:
        case AVRCP_GET_ELEMENT_ATTRIBUTES_IND:
            handleAvrcpQualificationTestCaseInd(task, id, message);
            break;

/*************/
/* UNHANDLED */
/*************/

        default:
            break;
    }
}

bool avrcpAvrcpIsEnabled(void)
{
    if(theSink.features.avrcp_enabled)
    {
        return TRUE;
    }
    return FALSE;
}
#else /* ENABLE_AVRCP*/
static const int avrcp_disabled;
#endif /* ENABLE_AVRCP*/
