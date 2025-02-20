/****************************************************************************
Copyright (c) 2011 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_peer.c

DESCRIPTION


NOTES

*/


/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_a2dp.h"
#include "sink_configmanager.h"
#include "sink_devicemanager.h"
#include "sink_config.h"
#include "sink_wired.h"
#include "sink_usb.h"
#include "sink_slc.h"
#include "sink_auth.h"
#include "sink_peer.h"
#include "sink_avrcp.h"
#include "sink_device_id.h"
#include "sink_link_policy.h"
#include "sink_powermanager.h"
#include "sink_peer_qualification.h"

#include <bdaddr.h>
#include <ps.h>
#include <panic.h>
#include <sdp_parse.h>
#include <stdlib.h>
#include <string.h>


#ifdef ENABLE_PEER

#ifdef DEBUG_PEER_SM

static const char * const av_source_str[] =
{
    "SOURCE_NULL",
    "SOURCE_ANALOGUE",
    "SOURCE_USB",
    "SOURCE_A2DP"
};

static const char * const relay_state_str[] =
{
    "STATE_IDLE",
    "STATE_CLOSING",
    "STATE_OPENING",
    "STATE_OPEN",
    "STATE_SUSPENDING",
    "STATE_STARTING",
    "STATE_STREAMING"
};

static const char * const relay_event_str[] =
{
    "EVENT_DISCONNECTED",
    "EVENT_CONNECTED",
    "EVENT_CLOSED",
    "EVENT_OPEN",
    "EVENT_OPENING",
    "EVENT_NOT_OPENED",
    "EVENT_OPENED",
    "EVENT_CLOSE",
    "EVENT_SUSPENDED",
    "EVENT_START",
    "EVENT_STARTING",
    "EVENT_NOT_STARTED",
    "EVENT_STARTED",
    "EVENT_SUSPEND"
};

#else
#endif


/* Unsigned Integer with Size Index 1 (implicit, 2 bytes) */
#define DATA_EL_UINT_16(value)  (0x09),(((value)>>8)&0xFF),((value)&0xFF)

/* Data Element Sequence with (count) uint16s, Size Index 5 (explicit) */
#define DATA_EL_UINT_16_LIST(count) (0x35),(3 * count)

/* UUID with Size Index 4 (implicit, 16 bytes) */
#define DATA_EL_UUID_128(value)  (0x1C),value

/* Data Element Sequence with (count) UUID128s, Size Index 5 (explicit) */
#define DATA_EL_UUID_128_LIST(count) (0x35),(17 * count)




/* Supported features are those Peer features offered by the local device */
/* Compatible feature are those Peer features that need to be supported by the remote device in order to be available for use */
#ifdef PEER_AS
#define SUPPORTED_SHAREME_A2DP_FEATURES  (remote_features_shareme_a2dp_source | remote_features_shareme_a2dp_sink  )
#define COMPATIBLE_SHAREME_A2DP_FEATURES (remote_features_shareme_a2dp_sink   | remote_features_shareme_a2dp_source)
#else
#define SUPPORTED_SHAREME_A2DP_FEATURES  (0)
#define COMPATIBLE_SHAREME_A2DP_FEATURES (0)
#endif

#ifdef PEER_TWS
#define SUPPORTED_TWS_A2DP_FEATURES  (remote_features_tws_a2dp_source | remote_features_tws_a2dp_sink  )
#define COMPATIBLE_TWS_A2DP_FEATURES (remote_features_tws_a2dp_sink   | remote_features_tws_a2dp_source)
#else
#define SUPPORTED_TWS_A2DP_FEATURES  (0)
#define COMPATIBLE_TWS_A2DP_FEATURES (0)
#endif

#ifdef PEER_AVRCP
#define SUPPORTED_PEER_AVRCP_FEATURES  (remote_features_peer_avrcp_target     | remote_features_peer_avrcp_controller)
#define COMPATIBLE_PEER_AVRCP_FEATURES (remote_features_peer_avrcp_controller | remote_features_peer_avrcp_target    )
#else
#define SUPPORTED_PEER_AVRCP_FEATURES  (0)
#define COMPATIBLE_PEER_AVRCP_FEATURES (0)
#endif

#define PEER_DEVICE_UUID128 0x00, 0x00, 0x10, 0x02, 0xD1, 0x02, 0x11, 0xE1, 0x9B, 0x23, 0x00, 0x02, 0x5B, 0x00, 0xA5, 0xA5     /* CSR Peer Device service UUID */
#define PEER_DEVICE_SUPPORTED_FEATURES  (SUPPORTED_SHAREME_A2DP_FEATURES | SUPPORTED_TWS_A2DP_FEATURES | SUPPORTED_PEER_AVRCP_FEATURES)
#define PEER_DEVICE_COMPATIBLE_FEATURES (COMPATIBLE_SHAREME_A2DP_FEATURES | COMPATIBLE_TWS_A2DP_FEATURES | COMPATIBLE_PEER_AVRCP_FEATURES)


#define BCD_MAJOR_VERSION 0xFF00
#define BCD_MINOR_VERSION 0x00F0
#define BCD_PATCH_VERSION 0x000F

#define PEER_DEVICE_MIN_SUPPORTED_VERSION 0x0300    /* Version 3.0.0 - should always remain at v3.0.0 for IOP with ADK3.x based devices */
#define PEER_DEVICE_VERSION 0x0320                  /* Major.Minor.Patch */
#define PEER_DEVICE_MIN_SUPPORTED_BUFFER_COMPATIBILITY_VERSION 0x0320 /* versions greater than this support 350ms internal buffering delay */
#define PEER_DEVICE_MIN_SUPPORTED_SLAVE_DELAY_COMPATIBILITY_VERSION     0x0310

#define MATCH_MAJOR_VERSION(X,Y) (((X) & BCD_MAJOR_VERSION) == ((Y) & BCD_MAJOR_VERSION))
#define MATCH_MINOR_VERSION(X,Y) (((X) & BCD_MINOR_VERSION) == ((Y) & BCD_MINOR_VERSION))
#define MATCH_PATCH_VERSION(X,Y) (((X) & BCD_PATCH_VERSION) == ((Y) & BCD_PATCH_VERSION))

/* First Battery Level Sending Delay 1 second */
#define FIRST_BATTERY_LEVEL_SENDING_DELAY    1000u

/* Subsequent Battery Level Sending Delay 60 seconds */
#define BATTERY_LEVEL_SENDING_DELAY          60000u

/* Internal message base */
#define PEER_INTERNAL_MSG_BASE               0x0000u

/* Internal messages */
typedef enum __peer_internal_msg
{
    PEER_INTERNAL_MSG_SEND_BATTERY_LEVEL = PEER_INTERNAL_MSG_BASE,
    PEER_INTERNAL_MSG_STATE_MACHINE_UNLOCK,
    PEER_INTERNAL_MSG_TOP

} peer_internal_msg_t;

/* Internal message handler */
static void peerInternalMessageHandler  ( Task task, MessageId id, Message message );

/* Internal message handler TaskData */
static const TaskData peer_internal_message_task = {peerInternalMessageHandler};

static const u8 peer_device_service_record_template[] =
{
    /* DataElUint16, ServiceClassIDList */
    DATA_EL_UINT_16(UUID_SERVICE_CLASS_ID_LIST),
        /* DataElSeq, 1 UUID128 */
        DATA_EL_UUID_128_LIST(1),
            /* DataElUuid128, 128-bit (16 byte) UUID */
            DATA_EL_UUID_128(PEER_DEVICE_UUID128),

#define PEER_DEVICE_OFFSET_UUID128_ATTR_VAL (6)


    /* DataElUint16, Supported Features */
    DATA_EL_UINT_16(UUID_SUPPORTED_FEATURES),
        /* DataElUint16, Supported Features Bitmask */
        DATA_EL_UINT_16(PEER_DEVICE_SUPPORTED_FEATURES),

#ifdef PEER_AS
    /* DataElUint16, Supported ShareMe Codecs */
    DATA_EL_UINT_16(UUID_SUPPORTED_CODECS_SHAREME),
        /* DataElUint16, ShareMe codecs bitmask */
        DATA_EL_UINT_16(0),

#define PEER_DEVICE_OFFSET_CODEC_BITS_SHAREME (32)
#endif /* PEER_AS */

#ifdef PEER_TWS
    /* DataElUint16, Supported TWS Codecs */
    DATA_EL_UINT_16(UUID_SUPPORTED_CODECS_TWS),
        /* DataElUint16, TWS codecs bitmask */
        DATA_EL_UINT_16(0),

#ifdef PEER_AS
/* TWS element is offset by size of ShareMe element */
#define PEER_DEVICE_OFFSET_CODEC_BITS_TWS (38)
#else
#define PEER_DEVICE_OFFSET_CODEC_BITS_TWS (32)
#endif

#endif /* PEER_TWS */
    /* DataElUint16, Legacy Peer Version */
    DATA_EL_UINT_16(UUID_LEGACY_VERSION),
        /* DataElUint16, Peer Device support version number */
        DATA_EL_UINT_16(PEER_DEVICE_MIN_SUPPORTED_VERSION),

    /* DataElUint16, Current Peer Version */
    DATA_EL_UINT_16(UUID_CURRENT_VERSION),
        /* DataElUint16, Peer Device support version number */
        DATA_EL_UINT_16(PEER_DEVICE_VERSION)
};

#define PEER_DEVICE_SERVICE_RECORD_SIZE (sizeof peer_device_service_record_template)


/* DataElSeq(0x35), Length(0x11), 128-bit UUID(0x1C) */
static const u8 peer_device_search_pattern[] = {0x35, 0x11, 0x1C, PEER_DEVICE_UUID128};

static const u8 peer_device_attr_list[] =
{
    DATA_EL_UINT_16_LIST(5),
        DATA_EL_UINT_16(UUID_SUPPORTED_FEATURES),       /* Supported Features */
        DATA_EL_UINT_16(UUID_SUPPORTED_CODECS_SHAREME), /* Supported ShareMe Codecs */
        DATA_EL_UINT_16(UUID_SUPPORTED_CODECS_TWS),     /* Supported TWS Codecs */
        DATA_EL_UINT_16(UUID_LEGACY_VERSION),           /* Legacy Peer Version */
        DATA_EL_UINT_16(UUID_CURRENT_VERSION)           /* Current Peer Version */
};
/* Prototype and task structure for Peer Device credentials specific SDP searches */
static void handleCredentialsMessage (Task task, MessageId id, Message message);
static const struct TaskData credentials_task = { handleCredentialsMessage };

#define isPeerStateStable()             (theSink.peer.current_state == theSink.peer.required_state)
#define isPeerStateStableAndStreaming() (isPeerStateStable() && theSink.peer.required_state == RELAY_STATE_STREAMING)

static bool isRemotePeerPaused(void)
{
    u16 peer_index;
    if(a2dpGetPeerIndex(&peer_index))
    {
        if((theSink.a2dp_link_data->remote_peer_status[peer_index] & (PEER_STATUS_IN_CALL | PEER_STATUS_DONT_RELAY))
            && (!theSink.features.TwsSingleDeviceOperation))
        {
            return TRUE;
        }
    }
    return FALSE;
}


/*************************************************************************
NAME
    expandUuid128

DESCRIPTION
    Expands a 128-bit UUID stored as eight 16-bit words to sixteen 8-bit words

RETURNS
    TRUE if non zero UUID read from PS, FALSE otherwise

**************************************************************************/
static bool expandUuid128 (u16 *uuid_base)
{
    bool non_zero = FALSE;
    u16 *read_ptr = uuid_base + 8;
    u16 *write_ptr = (u16 *)(uuid_base + 16);

    while (--read_ptr >= uuid_base)
    {
        u16 read_data = *read_ptr;

        LOGD(" %X",read_data);

        *(--write_ptr) = (read_data & 0xFF);
        *(--write_ptr) = ((read_data >> 8) & 0xFF);

        if (read_data)
        {
            non_zero = TRUE;
        }

    }

    LOGD("   non_zero=%u\n",non_zero);
    return non_zero;
}

/*************************************************************************
NAME
    handleCredentialsSdpServiceSearchAttributeCfm

DESCRIPTION
    Handles search results returned from requesting Peer credentials (Device Id and/or Peer SDP records)

RETURNS
    None

**************************************************************************/
static void handleCredentialsSdpServiceSearchAttributeCfm (CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    /* Not inquiring thus we are the acceptor of an incoming A2DP signalling channel connect request */
    u16 index;

    if (getA2dpIndexFromBdaddr(&cfm->bd_addr, &index))
    {
        bool peer_sdp_record_obtained = FALSE;

        if (theSink.features.PeerUseDeviceId && (theSink.a2dp_link_data->peer_device[index] == remote_device_unknown))
        {   /* Response is due to a request for the Device Id service record */
            if ( (cfm->status==sdp_response_success) && !cfm->error_code )
            {   /* Response obtained, examine it */
                theSink.a2dp_link_data->peer_device[index] = CheckRemoteDeviceId(cfm->attributes, cfm->size_attributes);
            }
            else
            {
                theSink.a2dp_link_data->peer_device[index] = remote_device_nonpeer;
            }

            theSink.a2dp_link_data->peer_features[index] = remote_features_none;
        }
        else
        {   /* Response is due to a request for the Peer Device service record */

            peer_sdp_record_obtained = TRUE;

            if ( (cfm->status==sdp_response_success) && !cfm->error_code )
            {   /* Response obtained, examine it */
                u16 remote_peer_version = peerGetRemoteVersionFromServiceRecord(cfm->attributes, cfm->size_attributes);
                remote_features remote_peer_features = peerGetRemoteSupportedFeaturesFromServiceRecord(cfm->attributes, cfm->size_attributes);

                peerSetRemoteSupportedCodecsFromServiceRecord(cfm->attributes, cfm->size_attributes);
                theSink.a2dp_link_data->peer_features[index] = peerDetermineCompatibleRemoteFeatures(remote_peer_features, remote_peer_version);
                theSink.a2dp_link_data->peer_device[index] = remote_device_peer;
                theSink.a2dp_link_data->peer_version[index] = remote_peer_version;
                theSink.a2dp_link_data->peer_dsp_required_buffering_level[index] = getPeerDSPBufferingRequired(remote_peer_version);
            }
            else
            {
                theSink.a2dp_link_data->peer_features[index] = remote_features_none;
                theSink.a2dp_link_data->peer_device[index] = remote_device_nonpeer;
                theSink.a2dp_link_data->peer_version[index] = 0;
                theSink.a2dp_link_data->peer_dsp_required_buffering_level[index] = PEER_BUFFER_250MS;
            }
        }

        if ((theSink.a2dp_link_data->peer_device[index] == remote_device_peer) && !peer_sdp_record_obtained)
        {   /* Remote device is a Peer, check to see what features it supports */
            RequestRemotePeerServiceRecord((Task)&credentials_task, &cfm->bd_addr);
        }
        else
        {   /* A non Peer device or a Peer with known features */
            if ((theSink.a2dp_link_data->peer_device[index] != remote_device_peer) ||        /* A non-Peer device */
                ((theSink.a2dp_link_data->peer_features[index] != remote_features_none) &&   /* A Peer with matching features? */
                 peerIsCompatibleDevice(theSink.a2dp_link_data->peer_version[index]) ) )     /* A Peer with compatible version number? */
            {   /* Accept the connection */
                LOGD("PEER: Device compatible - accepting A2DP Signalling\n");
                issueA2dpSignallingConnectResponse(&cfm->bd_addr, TRUE);
            }
            else
            {   /* Reject the connection */
                LOGD("PEER: Device NOT compatible - REJECTING A2DP Signalling\n");
                issueA2dpSignallingConnectResponse(&cfm->bd_addr, FALSE);
            }
        }
    }
}


/****************************************************************************
NAME
	handleCredentialsMessage

DESCRIPTION
    Task function to handle responses to Peer device specific credentials requests

RETURNS
	void
*/
static void handleCredentialsMessage (Task task, MessageId id, Message message)
{
    switch (id)
    {
    case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
        LOGD("PEER [Cred]: CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM\n");
        handleCredentialsSdpServiceSearchAttributeCfm ((CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*) message);
        break;

    default:
        LOGD("PEER [Cred]: Unexpected message, id=0x%X\n", id);
        break;
    }
}


/****************************************************************************
NAME
    peerSetLowPowerLinkPolicyInConnectedState

DESCRIPTION
    Function to set link policy in connected state to enter low power mode

RETURNS
    void
*/
static void peerSetLowPowerLinkPolicyInConnectedState(void)
{
    u16 peer_id;

    if (a2dpGetPeerIndex(&peer_id))
    {
        linkPolicyUseA2dpSettings( theSink.a2dp_link_data->device_id[peer_id],
                                               theSink.a2dp_link_data->stream_id[peer_id],
                                               A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[peer_id]) );
    }
}


/****************************************************************************
NAME
    peerCredentialsRequest

DESCRIPTION
    Request Peer device credentials (device id and/or peer device SDP record)
    from the specified device

RETURNS
    TRUE if credentials were requested, FALSE otherwise
*/
bool peerCredentialsRequest (bdaddr *device_addr)
{
    if (RequestRemoteDeviceId((Task)&credentials_task, device_addr))
    {
        LOGD("PEER: Requesting device id record\n");
        return TRUE;
    }

    if (RequestRemotePeerServiceRecord((Task)&credentials_task, device_addr))
    {
        LOGD("PEER: Requesting Peer Device service record\n");
        return TRUE;
    }

    return FALSE;
}


/*************************************************************************
NAME
    peerRequestServiceRecord

DESCRIPTION
    Issues a request to obtain the attributes of a Peer Device SDP record from
    the specified device

RETURNS
    TRUE if a search requested, FALSE otherwise

**************************************************************************/
bool RequestRemotePeerServiceRecord (Task task, const bdaddr *bd_addr)
{
    if (theSink.features.PeerUseCsrUuid)
    {
        LOGD("Requesting PEER SDP record using CSR UUID\n");
        ConnectionSdpServiceSearchAttributeRequest(task, bd_addr, 32, sizeof(peer_device_search_pattern), peer_device_search_pattern, sizeof(peer_device_attr_list), peer_device_attr_list);
        return TRUE;
    }
    else
    {
        u16 *search_pattern = (u16 *)mallocPanic( sizeof(u16) * sizeof(peer_device_search_pattern) );

        PsRetrieve(CONFIG_PEER_DEVICE_CUSTOM_UUID, &search_pattern[3], 8);    /* Get 128 bit UUID from PS */
        if ( expandUuid128((u16 *)&search_pattern[3]) )
        {
            LOGD("Requesting PEER SDP record using custom UUID\n");
            memcpy(search_pattern, peer_device_search_pattern, 3);          /* Get Data El Seq info from peer_device_search_pattern */

            ConnectionSdpServiceSearchAttributeRequest(task, bd_addr, 32, sizeof(peer_device_search_pattern), (u8 *)search_pattern, sizeof(peer_device_attr_list), peer_device_attr_list);
            return TRUE;
        }
        else
        {
            LOGD("Not requesting PEER SDP record\n");
            free(search_pattern);
        }
    }

    return FALSE;
}


/*************************************************************************
NAME
    peerSetRemoteSupportedCodecsFromServiceRecord

DESCRIPTION
    Extracts and stores the list of optional codecs supported by the remote peer device
    from the supplied Peer Device SDP record

RETURNS
    None
**************************************************************************/
void peerSetRemoteSupportedCodecsFromServiceRecord (const u8 *attr_data, u16 attr_data_size)
{
/*  Store peer's supported codec bitmask
 *  TODO: handle the (currently unsupported) case where we have both AS and TWS
 */
    u32 codecs = 0;

#ifdef PEER_AS
    if (SdpParseGetArbitrary(attr_data_size, attr_data, UUID_SUPPORTED_CODECS_SHAREME, &codecs))
    {
        LOGD("Remote ShareMe codecs 0x%04lX\n", codecs);
    }
#endif

#ifdef PEER_TWS
    if (SdpParseGetArbitrary(attr_data_size, attr_data, UUID_SUPPORTED_CODECS_TWS, &codecs))
    {
        LOGD("Remote TWS codecs 0x%04lX\n", codecs);
    }
#endif

    peerSetRemoteSupportedCodecs((u16)codecs);
}


/*************************************************************************
NAME
    peerGetRemoteVersionFromServiceRecord

DESCRIPTION
    Extracts the Peer Device version number from the supplied SDP record.

RETURNS
    The Peer Device support version number of a connected Peer, or 0 if not
    present in SDP record

**************************************************************************/
u16 peerGetRemoteVersionFromServiceRecord( const u8 *attr_data, u16 attr_data_size )
{
    u16 version;
    u32 long_version = 0;

    LOGD( "peerGetRemoteVersion" );

    /* Extract the legacy Peer version number, which is the only Peer version number advertised by ADK3.x based devices */
    SdpParseGetArbitrary( attr_data_size, attr_data, UUID_LEGACY_VERSION, &long_version );
    LOGD( "   legacy version = %lX\n", long_version );
    version = ( u16 )long_version;

    /* Extract current Peer version of remote device.  This will not exist for ADK3.x based devices */
    if( SdpParseGetArbitrary( attr_data_size, attr_data, UUID_CURRENT_VERSION, &long_version ) )
    {
        LOGD( "   current version = %lX\n", long_version );
        version = ( version > ( u16 )long_version ) ? version : ( u16 )long_version;
    }

    LOGD( "   version = %X\n", version );
    return version;
}


/*************************************************************************
NAME
    peerGetRemoteSupportedFeaturesFromServiceRecord

DESCRIPTION
    Extracts the Peer Device supported features from the supplied SDP record.

RETURNS
    The Peer Device supported features of a connected Peer, or 0 if not
    present in SDP record

**************************************************************************/
remote_features peerGetRemoteSupportedFeaturesFromServiceRecord (const u8 *attr_data, u16 attr_data_size)
{
    u32 supported_features;

    LOGD("peerGetRemoteSupportedFeatures");

    if ( SdpParseGetArbitrary(attr_data_size, attr_data, UUID_SUPPORTED_FEATURES, &supported_features) )
    {
        LOGD("   features = %lX\n",supported_features);
        return (remote_features)supported_features;
    }

    LOGD("   features not found\n");
    return remote_features_none;
}


/*************************************************************************
NAME
    peerDetermineCompatibleRemoteFeatures

DESCRIPTION
    Identifies the set of compatible features based on locally supported peer features
    and the remote features obtained from a Peer during a Peer Device SDP record search
    request.
    The compatible features are a set of mutually matching features i.e. if the local device
    supports a tws source role then the compatible feature would be the Peer supporting a tws
    sink role.

RETURNS
    The compatible set of features of the connected Peer

**************************************************************************/
remote_features peerDetermineCompatibleRemoteFeatures (remote_features supported_features, u16 version)
{
    remote_features compatible_features = remote_features_shareme_a2dp_sink & PEER_DEVICE_COMPATIBLE_FEATURES;   /* Minimally, we can expect a device to be a standard A2DP sink */

    LOGD("GetCompatibleRemotePeerFeatures\n");

    if ( supported_features && version )
    {
        LOGD("   features = %X   version = %X\n",supported_features,version);

        if (peerIsCompatibleDevice(version))
        {   /* Same peer version, so we can expect to support all A2DP and AVRCP features advertised */
            LOGD("   version matched\n");
            compatible_features = supported_features & PEER_DEVICE_COMPATIBLE_FEATURES;
        }
        else
        {   /* ShareMe uses standard A2DP operation, thus ShareMe can be supported in one or more roles */
            compatible_features = supported_features & (remote_features_shareme_a2dp_source | remote_features_shareme_a2dp_sink) & PEER_DEVICE_COMPATIBLE_FEATURES;
        }
    }
    LOGD("   compatible features = %X\n",compatible_features);

    return compatible_features;
}


/*************************************************************************
NAME
    peerIsCompatibleDevice

DESCRIPTION
    Uses the Peer version number to determine if another Peer device is compatible

RETURNS
    TRUE if device is deemed compatible, FALSE otherwise

**************************************************************************/
bool peerIsCompatibleDevice( u16 version )
{
    LOGD( "peerIsCompatibleDevice  remote ver=0x%X local_ver=0x%X min_ver=0x%X\n", version, PEER_DEVICE_VERSION, PEER_DEVICE_MIN_SUPPORTED_VERSION );

    if( version < ( u16 )PEER_DEVICE_MIN_SUPPORTED_VERSION )
    {   /* Don't support devices less than the minimum supported version */
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
NAME
    peerRegisterServiceRecord

DESCRIPTION
    Registers a Peer Device SDP record

RETURNS
    None

**************************************************************************/
void RegisterPeerDeviceServiceRecord ( void )
{
    u16 *service_record = mallocPanic(PEER_DEVICE_SERVICE_RECORD_SIZE);
    u16 codecs;
    memmove(service_record, peer_device_service_record_template, PEER_DEVICE_SERVICE_RECORD_SIZE);
    if (!theSink.features.PeerUseCsrUuid)
    {
        /* Update 128-bit UUID from PS */
        if (PsRetrieve(CONFIG_PEER_DEVICE_CUSTOM_UUID, service_record + PEER_DEVICE_OFFSET_UUID128_ATTR_VAL, 8))
        {
            expandUuid128((u16 *)&service_record[PEER_DEVICE_OFFSET_UUID128_ATTR_VAL]);

        }
    }
    /* Update codec bitmaps */
    if (peerGetLocalSupportedCodecs(&codecs))
    {
#ifdef PEER_TWS
        LOGD("Local TWS codecs 0x%04X\n", codecs);
        service_record[PEER_DEVICE_OFFSET_CODEC_BITS_TWS] = codecs >> 8;
        service_record[PEER_DEVICE_OFFSET_CODEC_BITS_TWS + 1] = codecs & 0x00FF;
#endif
#ifdef PEER_AS
        codecs &= ~(1 << AAC_CODEC_BIT);
        LOGD("Local ShareMe codecs 0x%04X\n", codecs);
        service_record[PEER_DEVICE_OFFSET_CODEC_BITS_SHAREME] = codecs >> 8;
        service_record[PEER_DEVICE_OFFSET_CODEC_BITS_SHAREME + 1] = codecs & 0x00FF;
#endif
    }
    /* Malloc'd block is passed to f/w and unmapped from VM space */
    ConnectionRegisterServiceRecord(&theSink.task, PEER_DEVICE_SERVICE_RECORD_SIZE, (u8 *)service_record);
}

/*************************************************************************
NAME
    peerUpdateMuteState

DESCRIPTION
    Responsible for updating the mute state according to the state
    of the peer state machine. Should mute when device is a TWS master
    during relay setup or tear down, or when devices are in idle state
    with no audio source.

RETURNS
    None

**************************************************************************/
void peerUpdateMuteState(void)
{
    /* Link-loss, relay or single TWS device, no mute */
    if((!peerIsRelayAvailable()) &&
       (isPeerStateStable() || isRemotePeerPaused()))
    {
        AudioSetInputAudioMute(FALSE);
    }
    /* TWS streaming, unmute if muted */
    else if(isPeerStateStableAndStreaming())
    {
        AudioSetInputAudioMute(FALSE);
    }
    /* TWS not currently streaming, mute if not muted.
       This only applies for a Master device.
    */
#ifdef PEER_AS
    else if (peerIsTwsMaster())
#else
    else
#endif /* PEER_AS */
    {
        AudioSetInputAudioMute(TRUE);
    }
}

/*************************************************************************
NAME
    updateAudioGating

DESCRIPTION
    Updates gating used for "multimedia" (non-sco) types of audio based on
    call status of any connected Peer and audio source being relayed

RETURNS
    None

**************************************************************************/
static void updateAudioGating (void)
{
    u16 peer_id;

    /* Initially assume that no audio needs gating */
    audioUngateAudio(audio_gate_all);

    if (theSink.peer.current_state != RELAY_STATE_STREAMING)
    {   /* Ensure we only relay audio data when in the Streaming state */
        audioGateAudio(audio_gate_relay);
    }

    if ( a2dpGetPeerIndex(&peer_id) )
    {   /* Have a connected Peer */
        if (theSink.features.TwsSingleDeviceOperation &&
            (theSink.a2dp_link_data->peer_features[peer_id] & remote_features_tws_a2dp_sink) &&
            (theSink.a2dp_link_data->remote_peer_status[peer_id] & PEER_STATUS_IN_CALL))
        {   /* Peer has an active call and we are in single device mode */
            audioGateAudio(audio_gate_multimedia);
            return;
        }
    }

    /* Not in a state to required multimedia audio to be gated due to a call on the other Peer, update for currently selected source */
    switch (theSink.features.PeerSource)
    {
    case RELAY_SOURCE_NULL:
        /* Auto source selection mode - don't gate any audio */
        break;
    case RELAY_SOURCE_ANALOGUE:
        /* Gate all non-sco audio excluding wired */
        audioGateAudio(audio_gate_multimedia & ~audio_gate_wired);
        break;
    case RELAY_SOURCE_USB:
        /* Gate all non-sco audio excluding USB */
        audioGateAudio(audio_gate_multimedia & ~audio_gate_usb);
        break;
    case RELAY_SOURCE_A2DP:
        /* Gate all non-sco audio excluding A2DP */
        audioGateAudio(audio_gate_multimedia & ~audio_gate_a2dp);
        break;
    }
}

/*************************************************************************
NAME
    issuePeerStatusChange

DESCRIPTION
    Issues a relay availability status update to the specified Peer

RETURNS
    TRUE if successful, FALSE otherwise

**************************************************************************/
static void issuePeerStatusChange (u16 peer_id, PeerStatusChange peer_status_change)
{
#ifdef ENABLE_AVRCP
    u16 avrcp_id;

    /* does the device support AVRCP and is AVRCP currently connected to this device? */
    for_all_avrcp(avrcp_id)
    {
        /* ensure media is streaming and the avrcp channel is that requested to be paused */
        if (theSink.avrcp_link_data && theSink.a2dp_link_data && theSink.avrcp_link_data->connected[avrcp_id] &&
            (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peer_id], &theSink.avrcp_link_data->bd_addr[avrcp_id])))
        {
            /* Inform remote peer of change to local devices status */
            sinkAvrcpVendorUniquePassthroughRequest( avrcp_id, AVRCP_PEER_CMD_PEER_STATUS_CHANGE, sizeof(peer_status_change), (const u8 *)&peer_status_change );
        }
    }
#endif
}

/*************************************************************************
NAME
    checkPeerFeatures

DESCRIPTION
    Checks the remote features bitmask of the current Peer device for support
    of one or more of the supplied features.

RETURNS
    TRUE if one or more features supported, FALSE otherwise

**************************************************************************/
static bool checkPeerFeatures (remote_features features)
{
    u16 peer_id;

    if (a2dpGetPeerIndex(&peer_id))
    {
        if (theSink.a2dp_link_data->peer_features[peer_id] & features)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    setRelayAvailability

DESCRIPTION
    Updates the relay availability for the specified peer status

RETURNS
    None

**************************************************************************/
static void setRelayAvailability (PeerStatus *peer_status, PeerStatusChange peer_status_change)
{
    LOGD("setRelayAvailability\n");

    if ((peer_status_change & PEER_CALL_STATUS_CHANGE) == PEER_STATUS_CHANGE_CALL_INACTIVE)
    {
        LOGD("   call inactive\n");
        *peer_status &= ~PEER_STATUS_IN_CALL;
    }

    if ((peer_status_change & PEER_CALL_STATUS_CHANGE) == PEER_STATUS_CHANGE_CALL_ACTIVE)
    {
        LOGD("   call active\n");
        *peer_status |= PEER_STATUS_IN_CALL;
    }

    if ((peer_status_change & PEER_RELAY_STATUS_CHANGE) == PEER_STATUS_CHANGE_RELAY_AVAILABLE)
    {
        LOGD("   relay available\n");
        *peer_status &= ~PEER_STATUS_DONT_RELAY;
    }

    if ((peer_status_change & PEER_RELAY_STATUS_CHANGE) == PEER_STATUS_CHANGE_RELAY_UNAVAILABLE)
    {
        LOGD("   relay unavailable\n");
        *peer_status |= PEER_STATUS_DONT_RELAY;
    }

    if ((peer_status_change & PEER_RELAY_OWNERSHIP_CHANGE) == PEER_STATUS_CHANGE_RELAY_FREED)
    {
        LOGD("   relay free'd\n");
        *peer_status &= ~PEER_STATUS_OWNS_RELAY;
    }

    if ((peer_status_change & PEER_RELAY_OWNERSHIP_CHANGE) == PEER_STATUS_CHANGE_RELAY_CLAIMED)
    {
        LOGD("   relay claimed\n");
        *peer_status |= PEER_STATUS_OWNS_RELAY;
    }
}

/*************************************************************************
NAME
    setRemoteRelayAvailability

DESCRIPTION
    Helper function used to update remote relay availability

RETURNS
    None

**************************************************************************/
static void setRemoteRelayAvailability (u16 peer_id, PeerStatusChange peer_status_change)
{
    LOGD("setRemoteRelayAvailability\n");
    setRelayAvailability( &theSink.a2dp_link_data->remote_peer_status[peer_id], peer_status_change );

    updateAudioGating();
    audioHandleRouting(audio_source_none);
}

/*************************************************************************
NAME
    setLocalRelayAvailability

DESCRIPTION
    Helper function to update local relay availability and issue status change to Peer

RETURNS
    None

**************************************************************************/
static void setLocalRelayAvailability (u16 peer_id, PeerStatusChange peer_status_change)
{
    LOGD("setLocalRelayAvailability\n");
    setRelayAvailability( &theSink.a2dp_link_data->local_peer_status[peer_id], peer_status_change );

    issuePeerStatusChange( peer_id, peer_status_change );
}


/*************************************************************************
NAME
    peerClaimRelay

DESCRIPTION
    Helper function to update local relay availability(claim or free) and issue status change to Peer

RETURNS
    None

**************************************************************************/
void peerClaimRelay(bool claim)
{
    u16 peer_id;

    if (a2dpGetPeerIndex(&peer_id))
    {
        if(claim)
        {
            if(peerIsRelayAvailable() && !(theSink.a2dp_link_data->local_peer_status[peer_id] & PEER_STATUS_OWNS_RELAY))
            {
                LOGD("Relay claimed\n");
                setLocalRelayAvailability( peer_id, PEER_STATUS_CHANGE_RELAY_CLAIMED);
            }
        }
        else
        {
            if(theSink.a2dp_link_data->local_peer_status[peer_id] & PEER_STATUS_OWNS_RELAY)
            {
                LOGD("Relay freed\n");
                setLocalRelayAvailability( peer_id, PEER_STATUS_CHANGE_RELAY_FREED);
            }
        }
    }
}

/*************************************************************************
NAME
    requestPeerRoleSwitch

DESCRIPTION
    Issues a request to role switch the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise

**************************************************************************/
static bool requestPeerRoleSwitch (u16 peer_id)
{
    if (theSink.a2dp_link_data->link_role[peer_id] == LR_CURRENT_ROLE_MASTER)
    {
        return FALSE;
    }
    else
    {
        Sink signalling_sink = A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[peer_id]);

        LOGD("   requesting role check before issuing start...\n");

        theSink.a2dp_link_data->link_role[peer_id] = LR_CHECK_ROLE_PENDING_START_REQ;
        linkPolicyGetRole(&signalling_sink);

        return TRUE;
    }
}

/*************************************************************************
NAME
    setPeerStreaming

DESCRIPTION
    Issues a request to start the relay channel to the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise

**************************************************************************/
static bool setPeerStreaming (void)
{
    u16 peer_id;

    if (a2dpGetPeerIndex(&peer_id))
    {
        LOGD("issuePeerStartRequest peer=%u  local_peer_status=0x%X  remote_peer_status=0x%X\n",peer_id,theSink.a2dp_link_data->local_peer_status[peer_id],theSink.a2dp_link_data->remote_peer_status[peer_id]);

        if ( peerIsRelayAvailable() )
        {
            if (requestPeerRoleSwitch(peer_id))
            {
                return TRUE;
            }

            if (a2dpIssuePeerStartRequest())
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}


/*************************************************************************
NAME
    issueSourceStartResponse

DESCRIPTION
    Peer state machine helper function

    Issues a start response to the current source

RETURNS
    TRUE if successful, FALSE otherwise

**************************************************************************/
static bool issueSourceStartResponse (RelaySource current_source)
{
    u16 av_id;

    LOGD("issueSourceStartResponse av=%u\n",current_source);

    switch (current_source)
    {
    case RELAY_SOURCE_NULL:
        break;
    case RELAY_SOURCE_ANALOGUE:
        /* Don't actually need to send a start response to an analogue source */
        return TRUE;
        break;
    case RELAY_SOURCE_USB:
        /* Don't actually need to send a start response to a USB source */
        return TRUE;
        break;
    case RELAY_SOURCE_A2DP:
        if (a2dpGetSourceIndex(&av_id))
        {
            a2dp_stream_state a2dp_state = A2dpMediaGetState(theSink.a2dp_link_data->device_id[av_id], theSink.a2dp_link_data->stream_id[av_id]);

            if (a2dp_state == a2dp_stream_streaming)
            {   /* Already streaming */
                return TRUE;
            }
            else if (a2dp_state == a2dp_stream_starting)
            {   /* Starting, so attempt to send a response */
                return A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[av_id], theSink.a2dp_link_data->stream_id[av_id], TRUE);
            }
            /* else in wrong state, so fall through and return FALSE */
        }
        break;
    }

    return FALSE;
}

static a2dp_link_role peerGetLinkRole(void)
{
    u16 source_id;
    a2dp_link_role role = LR_UNKNOWN_ROLE;
    if(a2dpGetPeerIndex(&source_id))
    {
        role = theSink.a2dp_link_data->link_role[source_id];
    }
    return role;
}

static bool peerIsLinkMaster(void)
{
    return (peerGetLinkRole() == LR_CURRENT_ROLE_MASTER);
}

static bool peerIsLinkRoleKnown(void)
{
    return (!(peerGetLinkRole() == LR_UNKNOWN_ROLE));
}

static bool peerIsSlaveDelaySupportedByConnectedPeer(void)
{
    u16 source_id;
    return (a2dpGetPeerIndex(&source_id)
            && (theSink.a2dp_link_data->peer_version[source_id] >= PEER_DEVICE_MIN_SUPPORTED_SLAVE_DELAY_COMPATIBILITY_VERSION));
}

static bool peerIsAllowedToRelaySource(void)
{
    return ((peerIsLinkMaster() && peerIsSlaveDelaySupportedByConnectedPeer())
            || theSink.features.UnlockPeerStateMachine || (peerIsLinkMaster() && theSink.features.TwsQualificationEnable));
}

/*************************************************************************
NAME
    determineRequiredState

DESCRIPTION
    Peer state machine helper function.

    Examines any currently connected sources to determine which source needs to be relayed.

RETURNS
    TRUE if a change to the required state occurs, FALSE otherwise

**************************************************************************/
static bool determineRequiredState (void)
{
    RelayState last_state = theSink.peer.required_state;
    RelaySource last_source = theSink.peer.required_source;

    /* Initially assume no source is connected */
    theSink.peer.required_state = RELAY_STATE_IDLE;
    theSink.peer.required_source = RELAY_SOURCE_NULL;

    if(peerIsAllowedToRelaySource())
    {
        u16 source_id;
        if (((theSink.features.PeerSource == RELAY_SOURCE_ANALOGUE) || (theSink.features.PeerSource == RELAY_SOURCE_NULL)) && analogAudioConnected())
        {   /* If an analogue source is connected, it is considered to be streaming */
            theSink.peer.required_state = RELAY_STATE_STREAMING;
            theSink.peer.required_source = RELAY_SOURCE_ANALOGUE;
        }

        if (((theSink.features.PeerSource == RELAY_SOURCE_USB) || (theSink.features.PeerSource == RELAY_SOURCE_NULL)) && (usbAudioIsAttached() && sinkUsbIsSpeakerActive() && (!sinkUsbAudioIsSuspended())))
        {   /* Streaming USB source has higher priority than streaming analogue source */
            theSink.peer.required_state = RELAY_STATE_STREAMING;
            theSink.peer.required_source = RELAY_SOURCE_USB;
        }

        /* Check for A2DP Source */
        if (((theSink.features.PeerSource == RELAY_SOURCE_A2DP) || (theSink.features.PeerSource == RELAY_SOURCE_NULL)) && a2dpGetSourceIndex(&source_id))
        {
            a2dp_stream_state a2dp_state = A2dpMediaGetState(theSink.a2dp_link_data->device_id[source_id], theSink.a2dp_link_data->stream_id[source_id]);
            a2dp_suspend_state suspend_state = theSink.a2dp_link_data->SuspendState[source_id]; /* AVRCP play status can lead AVDTP stream state changes */

            if ((suspend_state == a2dp_not_suspended) && ((a2dp_state == a2dp_stream_starting) || (a2dp_state == a2dp_stream_streaming)))
            {   /* A streaming (or heading towards it) A2DP source has the highest priority */
                theSink.peer.required_state = RELAY_STATE_STREAMING;
                theSink.peer.required_source = RELAY_SOURCE_A2DP;
            }
            else if ((a2dp_state == a2dp_stream_open) || (a2dp_state == a2dp_stream_suspending) || (a2dp_state == a2dp_stream_streaming))
            {   /* A2DP media stream is open (or heading back to it).  Check for streaming state as well as AVRCP play status can lead AVDTP stream state changes */
                u16 peer_id;

                if (a2dpGetPeerIndex(&peer_id) && !(theSink.a2dp_link_data->peer_features[peer_id] & remote_features_peer_source))
                {   /* We have a Peer that can't be a source itself, thus it is ok to leave a media channel open when suspended as */
                    /* it will never want to use the relay channel. (Only when idle will the relay channel be marked as available) */
                    if (theSink.peer.required_state != RELAY_STATE_STREAMING)
                    {   /* Don't allow an open A2DP stream to have higher precedence that an streaming Analogue/USB stream */
                        theSink.peer.required_state = RELAY_STATE_OPEN;
                        theSink.peer.required_source = RELAY_SOURCE_A2DP;
                    }
                }
            }
        }
    }

    PEER_LOGD("PEER: Required[%s:%s]\n",av_source_str[theSink.peer.required_source],relay_state_str[theSink.peer.required_state]);

    if ((last_state != theSink.peer.required_state) || (last_source != theSink.peer.required_source))
    {   /* Required state has changed */
        return TRUE;
    }
    else
    {   /* No change to required state */
        return FALSE;
    }
}

/*************************************************************************
NAME
    updateTargetRelayState

DESCRIPTION
    Peer state machine helper function to manage updating of the target state.

    The Peer state machine is designed to manage the media (relay) channel to another Peer device.  Generally, the Peer driving the
    state machine is the one attempting to relay audio.
    However, for adherence to the GAVDP specification, an A2DP source must allow a sink to open and start a media channel.  We cope
    with this by allowing a Peer to change the current_state but keep the current_source and target_source as RELAY_SOURCE_NULL.  The
    target_state is allowed to track the current_state in this scenario, so the state machine does not attempt to drive the relay
    channel from this side.

    The required_state/source indicates what is currently streaming and what the relay channel ultimately needs to be forwarding.

    The required_state/source can change rapidly as a user pauses/resumes a single source or swaps between different sources.
    This is tempered by the target_state/source which attempts to track the required_state/source as closely as possible but is
    only updated when the current state reaches that last set target state.

    However, changing the required_source will cause the state machine to return the relay state to idle before progressing again.
    This ensures the relay media channel is closed and then re-opened with a SEP suitable for forwarding audio data from the
    required_source.

    The final stage of the state machine is to only action changes to the current state once one of the non-transitional (stable)
    states has been reached (idle, open, streaming and routed).  This, again, prevents rapid changes to the required_state from
    unduly upsetting operation.

RETURNS
    None

**************************************************************************/
static void updateTargetRelayState (void)
{
    if ( peerIsRelayAvailable() )
    {
        if (theSink.peer.current_state == theSink.peer.target_state)
        {   /* Reached the target state, so update as appropriate */
            if ((theSink.peer.target_source != theSink.peer.required_source) && (theSink.peer.target_state != RELAY_STATE_IDLE))
            {   /* Source has changed, go through idle state to get to it */
                theSink.peer.target_state = RELAY_STATE_IDLE;
            }
            else
            {   /* Update to the required state */
                theSink.peer.target_source = theSink.peer.required_source;
                theSink.peer.target_state = theSink.peer.required_state;
            }
        }
        else
        {   /* Current state differs to target state */
            if ((theSink.peer.current_source == RELAY_SOURCE_NULL) && (theSink.peer.target_source == RELAY_SOURCE_NULL))
            {   /* No source being managed.  Thus the other Peer is driving the state machine */
                /* Update target state to keep track of what other Peer is doing and also prevent this device from driving the state machine too */
                if (checkPeerFeatures(remote_features_peer_source) || (theSink.peer.required_source == RELAY_SOURCE_NULL))
                {   /* Only prevent local Peer state machine from driving state if other Peer can act as a source or we don't require use of relay channel */
                    /* Prevents sink only Peers (such as PTS) from holding a media channel open if we need to use it */
                    theSink.peer.target_state = theSink.peer.current_state;
                }

                if (theSink.peer.current_state == RELAY_STATE_IDLE)
                {   /* Other peer has now finished using the relay channel.  Update to the required state */
                    theSink.peer.target_source = theSink.peer.required_source;
                    theSink.peer.target_state = theSink.peer.required_state;
                }
            }
            /* else allow relay state to reach target state before updating target state again */
        }
    }
    else
    {
        if ((theSink.peer.current_source == RELAY_SOURCE_NULL) && (theSink.peer.target_source == RELAY_SOURCE_NULL))
        {   /* No source being managed.  Thus the other Peer is driving the state machine */
            /* Update target state to keep track of what other Peer is doing and also prevent this device from driving the state machine too */
            theSink.peer.target_state = theSink.peer.current_state;
        }
        else
        {   /* Either no Peer connected or not in a state to accept a media stream.  Revert target state to idle */
            theSink.peer.target_source = RELAY_SOURCE_NULL;
            theSink.peer.target_state = RELAY_STATE_IDLE;
        }
    }

    PEER_LOGD("PEER: Target[%s:%s]\n",av_source_str[theSink.peer.target_source],relay_state_str[theSink.peer.target_state]);
}


/*************************************************************************
NAME
    updateCurrentState

DESCRIPTION
    Peer state machine helper function to help manage changes to the current state

RETURNS
    TRUE if successful, FALSE otherwise

**************************************************************************/
void updateCurrentState (RelayState relay_state)
{
    theSink.peer.current_state = relay_state;

    if (theSink.peer.current_state == RELAY_STATE_IDLE)
    {   /* Reset current source once idle state reached */
        theSink.peer.current_source = RELAY_SOURCE_NULL;
    }

    /* Update target state, if necessary */
    updateTargetRelayState();

    if (theSink.peer.current_source == RELAY_SOURCE_NULL)
    {   /* Update current source if we have reached idle and target has changed */
        theSink.peer.current_source = theSink.peer.target_source;
    }

    PEER_LOGD("PEER: Current[%s:%s]\n",av_source_str[theSink.peer.current_source],relay_state_str[theSink.peer.current_state]);
}

static void peerScheduleUnlockStateMachine(void)
{
    LOGD("PEER: schedule unlock\n");
    MessageSendLater((TaskData*)&peer_internal_message_task, PEER_INTERNAL_MSG_STATE_MACHINE_UNLOCK, NULL, PEER_UNLOCK_DELAY );
}

#define peerUnlockStateMachine()   {LOGD("PEER: unlock\n"); theSink.features.UnlockPeerStateMachine = 1;}
#define peerLockStateMachine()     {LOGD("PEER: lock\n"); theSink.features.UnlockPeerStateMachine = 0;}

/*************************************************************************
NAME
    peerSendBatteryLevel

DESCRIPTION
    Sends Battery level to currently connected Peer

RETURNS
    None

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
static void peerSendBatteryLevel (void)
{
    u16 peerA2dpId;
    u16 peerAvrcpIndex;

    if(!a2dpGetPeerIndex(&peerA2dpId))
    {
        return;
    }

    for_all_avrcp(peerAvrcpIndex)
    {
        if(theSink.avrcp_link_data && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peerA2dpId], &theSink.avrcp_link_data->bd_addr[peerAvrcpIndex]))
        {
            u8 battery_levels[2];
            u16 batterylevel = powerManagerBatteryLevelAsPercentage();
            battery_levels[0] = (u8)(batterylevel >> 8);
            battery_levels[1] = (u8)batterylevel & 0xff;
            sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex , AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL,
                                                    sizeof(battery_levels), (const u8 *)battery_levels);
            break;
        }
    }
}
#else
#define peerSendBatteryLevel() ((void)0)
#endif

/*************************************************************************
NAME
    peerScheduleSendBatteryLevel

DESCRIPTION
    Schedule sending battery level message

RETURNS
    None

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
static void peerScheduleSendBatteryLevel(u16 delay)
{
    MessageSendLater((TaskData*)&peer_internal_message_task, PEER_INTERNAL_MSG_SEND_BATTERY_LEVEL , 0 , delay);
}
#else
#define peerScheduleSendBatteryLevel(delay)
#endif

/*************************************************************************
NAME
    peerStopSendingBatteryLevel

DESCRIPTION
    Stop sending internal send battery level messages

RETURNS
    None

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
static void peerStopSendingBatteryLevel(void)
{
    theSink.peer_battery_level = BATTERY_LEVEL_INVALID;
    MessageCancelAll((TaskData*)&peer_internal_message_task, PEER_INTERNAL_MSG_SEND_BATTERY_LEVEL ) ;
}
#else
#define peerStopSendingBatteryLevel()
#endif

/*************************************************************************
NAME
    peerInternalMessageHandler

DESCRIPTION
    Handle peer internal messages

RETURNS
    None

**************************************************************************/
static void peerInternalMessageHandler  ( Task task, MessageId id, Message message )
{
    switch(id)
    {
        case PEER_INTERNAL_MSG_SEND_BATTERY_LEVEL:
        {
            peerSendBatteryLevel();
            peerScheduleSendBatteryLevel(BATTERY_LEVEL_SENDING_DELAY);
        }
        break;

        case PEER_INTERNAL_MSG_STATE_MACHINE_UNLOCK:
            LOGD("PEER: unlock msg recieved\n");
            if(peerIsLinkRoleKnown())
            {
                peerUnlockStateMachine();
                PEER_UPDATE_REQUIRED_RELAY_STATE("UNLOCK PEER");
            }
            else
            {
                peerScheduleUnlockStateMachine();
            }
            break;

        default:
            LOGD("PEER: Unhandled msg[%x]\n", id);
            break;
    }
}

/*************************************************************************
NAME
    peerAdvanceRelayState

DESCRIPTION
    Updates the Peer state machine current state based on the supplied event.
    This function should be called when an event occurs that would cause a change to
    the actual Peer state machine status.

    This forms one of the two functions that should be used to drive the Peer state machine.

RETURNS
    None

**************************************************************************/
void peerAdvanceRelayState (RelayEvent relay_event)
{
    PEER_LOGD("----\nPEER: Relay[%s]\n",relay_event_str[relay_event]);

    /* For PTS there needs to be State machine change to trigger
     * reconfiguring of Caps (audio SRC's Caps) on getting stream suspend indication.
     * And for some PTS TC's, PTS expects SRC to open the discover/set configuration.
     * So, this wrapper function is called to handle such situation */
    if(!peerQualificationAdvanceRelayState(relay_event))
    {
        switch(relay_event)
        {
            case RELAY_EVENT_DISCONNECTED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_DISCONNECTED\n");
                updateCurrentState( RELAY_STATE_IDLE );
                peerStopSendingBatteryLevel();
                peerLockStateMachine();
                determineRequiredState();
                break;

            case RELAY_EVENT_CONNECTED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_CONNECTED\n");
                updateCurrentState( RELAY_STATE_IDLE );
                peerScheduleSendBatteryLevel(FIRST_BATTERY_LEVEL_SENDING_DELAY);

                if ( (theSink.peer.target_state > theSink.peer.current_state) && a2dpIssuePeerOpenRequest() )
                {   /* Open relay stream */
                    /* Opened relay stream */
                    LOGD("PEER: Issue PEER OPEN REQ\n");
                    updateCurrentState( RELAY_STATE_OPENING );
                }
                else
                {
                    peerSetLowPowerLinkPolicyInConnectedState();
                }
                peerScheduleUnlockStateMachine();
                break;

            case RELAY_EVENT_CLOSED:
                updateCurrentState( RELAY_STATE_IDLE );
                peerStopSendingBatteryLevel();
                if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Open relay stream */
                    if (a2dpIssuePeerOpenRequest())
                    {
                        PEER_LOGD("PEER: Issue[%s]\n","PEER OPEN REQ");
                        updateCurrentState( RELAY_STATE_OPENING );
                    }
                }
                break;

            case RELAY_EVENT_OPEN:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_OPEN\n");
                updateCurrentState( RELAY_STATE_IDLE );
                if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Open relay stream */
                    if (a2dpIssuePeerOpenRequest())
                    {
                            LOGD("PEER: Issue PEER OPEN REQ\n");
                        updateCurrentState( RELAY_STATE_OPENING );
                    }
                }
                break;

            case RELAY_EVENT_OPENING:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_OPENING\n");
                updateCurrentState( RELAY_STATE_OPENING );
                /* Peer will wait for open to complete, so do nothing further */
                break;

            case RELAY_EVENT_NOT_OPENED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_NOT_OPENED\n");
                updateCurrentState( RELAY_STATE_IDLE );
                if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Open relay stream */
                    if (a2dpIssuePeerOpenRequest())
                    {
                            LOGD("PEER: Issue PEER OPEN REQ\n");
                        updateCurrentState( RELAY_STATE_OPENING );
                    }
                }
                break;

            case RELAY_EVENT_OPENED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_OPENED\n");
                updateCurrentState( RELAY_STATE_OPEN );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Close relay stream */
                    if (a2dpIssuePeerCloseRequest())
                    {
                            LOGD("PEER: Issue PEER CLOSE REQ\n");
                        updateCurrentState( RELAY_STATE_CLOSING );
                    }
                }
                else if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Start relay stream */
                    if (setPeerStreaming())
                    {
                            LOGD("PEER: Issue PEER START REQ\n");
                        updateCurrentState( RELAY_STATE_STARTING );
                    }
                }
                break;

            case RELAY_EVENT_CLOSE:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_CLOSE\n");
                updateCurrentState( RELAY_STATE_OPEN );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Close relay stream */
                    if (a2dpIssuePeerCloseRequest())
                    {
                            LOGD("PEER: Issue PEER CLOSE REQ\n");
                        updateCurrentState( RELAY_STATE_CLOSING );
                    }
                }
                break;

            case RELAY_EVENT_SUSPENDED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_SUSPENDED\n");
                updateCurrentState( RELAY_STATE_OPEN );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Close relay stream */
                        if(a2dpIssuePeerCloseRequest())
                    {
                            LOGD("PEER: Issue PEER CLOSE REQ\n");
                        updateCurrentState( RELAY_STATE_CLOSING );
                    }
                }
                else if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Start relay stream */
                    if (setPeerStreaming())
                    {
                            LOGD("PEER: Issue PEER START REQ\n");
                        updateCurrentState( RELAY_STATE_STARTING );
                    }
                }
                break;

            case RELAY_EVENT_START:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_START\n");
                updateCurrentState( RELAY_STATE_OPEN );
                if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Start relay stream */
                    if (setPeerStreaming())
                    {
                            LOGD("PEER: Issue PEER START REQ\n");
                        updateCurrentState( RELAY_STATE_STARTING );
                    }
                }
                break;

            case RELAY_EVENT_STARTING:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_STARTING\n");
                updateCurrentState( RELAY_STATE_STARTING );
                /* Peer will wait for start to complete, so do nothing further */
                break;

            case RELAY_EVENT_NOT_STARTED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_NOT_STARTED\n");
                updateCurrentState( RELAY_STATE_OPEN );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Close relay stream */
                    if (a2dpIssuePeerCloseRequest())
                    {
                            LOGD("PEER: Issue PEER CLOSE REQ\n");
                        updateCurrentState( RELAY_STATE_CLOSING );
                    }
                        PEER_LOGD("PEER: Issue[%s]\n","PEER CLOSE REQ");
                }
                else if (theSink.peer.target_state > theSink.peer.current_state)
                {   /* Start relay stream */
                    if (setPeerStreaming())
                    {
                            LOGD("PEER: Issue PEER START REQ\n");
                        updateCurrentState( RELAY_STATE_STARTING );
                    }
                }
                break;

            case RELAY_EVENT_STARTED:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_STARTED\n");
                updateCurrentState( RELAY_STATE_STREAMING );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Suspend relay stream */
                    if (a2dpIssuePeerSuspendRequest())
                    {
                            LOGD("PEER: Issue PEER SUSPEND REQ\n");
                        updateCurrentState( RELAY_STATE_SUSPENDING );
                    }
                        PEER_LOGD("PEER: Issue[%s]\n","PEER SUSPEND REQ");
                }
                else
                {   /* Route relay stream */
                        issueSourceStartResponse(theSink.peer.current_source);
                        VolumeUpdateRoutedAudioMainAndAuxVolume();
                        LOGD("PEER: Issue SOURCE START RESP\n");
                }
                break;

            case RELAY_EVENT_SUSPEND:
                LOGD("peerAdvanceRelayState : RELAY_EVENT_SUSPEND\n");
                updateCurrentState( RELAY_STATE_STREAMING );
                if (theSink.peer.target_state < theSink.peer.current_state)
                {   /* Suspend relay stream */
                    if (a2dpIssuePeerSuspendRequest())
                    {
                            LOGD("PEER: Issue PEER SUSPEND REQ\n");
                        updateCurrentState( RELAY_STATE_SUSPENDING );
                    }
                }
                break;
        }
    }

    /* Update mute state if necessary */
    peerUpdateMuteState();
    updateAudioGating();
    audioHandleRouting(audio_source_none);
}


/*************************************************************************
NAME
    kickPeerStateMachine

DESCRIPTION
    Will automatically attempt to advance the Relay channel if it is in a 'steady' state (i.e. not expecting an external event)

RETURNS
    None

**************************************************************************/
static void kickPeerStateMachine (void)
{
    switch (theSink.peer.current_state)
    {
    case RELAY_STATE_IDLE:
        if (theSink.peer.target_state > theSink.peer.current_state)
        {   /* Open relay stream */
            peerAdvanceRelayState(RELAY_EVENT_OPEN);
        }
        break;
    case RELAY_STATE_OPEN:
        if (theSink.peer.target_state < theSink.peer.current_state)
        {   /* Close relay stream */
            peerAdvanceRelayState(RELAY_EVENT_CLOSE);
        }
        if (theSink.peer.target_state > theSink.peer.current_state)
        {   /* Start relay stream */
            peerAdvanceRelayState(RELAY_EVENT_START);
        }
        break;
    case RELAY_STATE_STREAMING:
        if (theSink.peer.target_state < theSink.peer.current_state)
        {   /* Suspend relay stream */
            peerAdvanceRelayState(RELAY_EVENT_SUSPEND);
        }
        break;
    default:
        /* Do nothing as we are waiting for an action to complete */
        break;
    }
}


/*************************************************************************
NAME
    peerUpdateRequiredRelayState

DESCRIPTION
    Used to update the Peer state machine required state.  This should be called when any event occurs
    that would cause a change to the required streaming state or source.

    This forms one of the two functions that should be used to drive the Peer state machine.

RETURNS
    None

**************************************************************************/
void peerUpdateRequiredRelayState (void)
{
    determineRequiredState();
    updateTargetRelayState();
    kickPeerStateMachine();
}

#ifdef PEER_TWS
/*************************************************************************
NAME
    getPeerSource

DESCRIPTION
    Obtains the currently configured source

RETURNS
    Current source

**************************************************************************/
static RelaySource getPeerSource (void)
{
    if (theSink.features.PeerSource == RELAY_SOURCE_NULL)
    {   /* Automatic source selection configured, so return the current relayed source */
        return theSink.peer.required_source;
    }
    else
    {   /* Return the current configured source */
        return theSink.features.PeerSource;
    }
}

/*************************************************************************
NAME
    setSource

DESCRIPTION
    Sets the current source to use for relaying audio

RETURNS
    TRUE if successful, FALSE otherwise

**************************************************************************/
static bool setPeerSource(const RelaySource new_source)
{
    if (theSink.features.PeerSource != new_source)
    {
        theSink.features.PeerSource = new_source;

        updateAudioGating();
        audioHandleRouting(audio_source_none);

        PEER_UPDATE_REQUIRED_RELAY_STATE("SELECT NEW SOURCE");
    }

    return TRUE;
}
#endif

/*************************************************************************
NAME
    peerGetPeerSink

DESCRIPTION
    Obtains sink to relay channel

RETURNS
    Handle to relay channel, NULL otherwise

**************************************************************************/
Sink peerGetPeerSink (void)
{
    if (theSink.peer.current_state >= RELAY_STATE_OPEN)
    {
        u16 peer_id;

        if (a2dpGetPeerIndex(&peer_id))
        {
            return A2dpMediaGetSink(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id]);
        }
    }

    return NULL;
}


/*************************************************************************
NAME
    peerGetSourceSink

DESCRIPTION
    Obtains sink of the current source

RETURNS
    Handle to sink if there is a current streaming source, NULL otherwise

**************************************************************************/
Sink peerGetSourceSink (void)
{
    Sink sink = NULL;

    switch (theSink.peer.current_source)
    {
    case RELAY_SOURCE_NULL:
        break;

    case RELAY_SOURCE_ANALOGUE:
        sink = analogGetAudioSink();
        break;

    case RELAY_SOURCE_USB:
        sink = usbGetAudioSink();
        break;

    case RELAY_SOURCE_A2DP:
        sink = a2dpGetSourceSink();
        break;
    }

    return sink;
}


/*************************************************************************
NAME
    peerIsRelayAvailable

DESCRIPTION
    Determines if relay channel is available for use

RETURNS
    TRUE if relay available, FALSE otherwise

**************************************************************************/
bool peerIsRelayAvailable (void)
{
    u16 peer_id;

    if (a2dpGetPeerIndex(&peer_id))
    {
        if ( !(theSink.a2dp_link_data->remote_peer_status[peer_id] & (PEER_STATUS_IN_CALL | PEER_STATUS_DONT_RELAY | PEER_STATUS_OWNS_RELAY)) &&   /* Has remote peer requested that relay not be used? */
             !(theSink.a2dp_link_data->local_peer_status[peer_id] & (PEER_STATUS_IN_CALL | PEER_STATUS_DONT_RELAY)) &&    /* Does local device not want relay to be used? */
              ((theSink.a2dp_link_data->seid[peer_id] == 0) || (theSink.a2dp_link_data->seid[peer_id] & SOURCE_SEID_MASK)) &&   /* Either no SEP or a source SEP has been configured */
              (theSink.a2dp_link_data->peer_features[peer_id] & remote_features_peer_sink) )                              /* Peer supports a sink role */
        {
            PEER_LOGD("PEER: RelayAvailable[TRUE]\n");
            return TRUE;
        }
    }

    PEER_LOGD("PEER: RelayAvailable[FALSE]\n");
    return FALSE;
}

/*************************************************************************
NAME
    peerCheckSource

DESCRIPTION
    Determines if the current peer source is the same as the new_source.

RETURNS
    TRUE if the current peer source is the same as new_source or if it is RELAY_SOURCE_NULL, FALSE otherwise

**************************************************************************/
bool peerCheckSource(RelaySource new_source)
{
    if((theSink.features.PeerSource != RELAY_SOURCE_NULL) && (theSink.features.PeerSource != new_source))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/*************************************************************************
NAME
    peerUpdateLocalStatusChange

DESCRIPTION
    Updates the local relay availability and issues a status change to a connected Peer

RETURNS
    None

**************************************************************************/
void peerUpdateLocalStatusChange (PeerStatusChange peer_status_change)
{
    u16 peer_id;

    LOGD("peerUpdateLocalStatusChange status=0x%X\n",peer_status_change);

    if (a2dpGetPeerIndex(&peer_id))
    {
        setLocalRelayAvailability( peer_id, peer_status_change);
        PEER_UPDATE_REQUIRED_RELAY_STATE("LOCAL STATUS CHANGED");
        peerUpdateMuteState(); /* update mute state, if required */
    }
}


/*************************************************************************
NAME
    peerHandleStatusChangeCmd

DESCRIPTION
    Handles a relay availability status change from a Peer

RETURNS
    None

**************************************************************************/
void peerHandleStatusChangeCmd (PeerStatusChange peer_status_change)
{
    u16 peer_id;

    LOGD("peerHandleStatusChangeCmd status=0x%X\n",peer_status_change);

    if (a2dpGetPeerIndex(&peer_id))
    {
        a2dp_link_priority priority;

        setRemoteRelayAvailability( peer_id, peer_status_change);
        /* If not in single device mode, we need to flag the link as available locally,
         * but not propagate to the remote device */
        if(!theSink.features.TwsSingleDeviceOperation && findCurrentA2dpSource(&priority)
            && (priority!=peer_id) && (peer_status_change==PEER_STATUS_CHANGE_RELAY_AVAILABLE))
        {
            setRelayAvailability( &theSink.a2dp_link_data->local_peer_status[peer_id], peer_status_change );
        }
        PEER_UPDATE_REQUIRED_RELAY_STATE("REMOTE STATUS CHANGED");
        peerUpdateMuteState(); /* update mute state, if required */
    }
}


/*************************************************************************
NAME
    peerHandleAudioRoutingCmd

DESCRIPTION
    Hamdles a audio routing notification from a Peer

RETURNS
    None

**************************************************************************/
void peerHandleAudioRoutingCmd (PeerTwsAudioRouting master_routing_mode, PeerTwsAudioRouting slave_routing_mode)
{
    u16 peer_id;

    LOGD("peerHandleAudioRoutingCmd  master=%u  slave=%u\n",master_routing_mode,slave_routing_mode);

    if (a2dpGetPeerIndex(&peer_id))
    {
            theSink.a2dp_link_data->a2dp_audio_mode_params.master_routing_mode = master_routing_mode;
            theSink.a2dp_link_data->a2dp_audio_mode_params.slave_routing_mode = slave_routing_mode;
        deviceManagerUpdateAttributes(&theSink.a2dp_link_data->bd_addr[peer_id], sink_tws, 0, 0);

        if (theSink.routed_audio)
        {
            AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
        }
    }
}


/*************************************************************************
NAME
    peerHandleVolumeCmd

DESCRIPTION
    Handle a volume change notification from a Peer

RETURNS
    None

**************************************************************************/
void peerHandleVolumeCmd (u8 volume)
{
    u16 peer_id;

    LOGD("peerHandleVolumeCmd  volume=%u\n",volume);

    if (a2dpGetPeerIndex(&peer_id))
    {
        volume = volume;
    }
}

/*************************************************************************
NAME
    peerUpdateMasterTrimVolume

DESCRIPTION
    Handle a trim volume change notification from a Peer

RETURNS
    None

**************************************************************************/

void peerUpdateTrimVolume(PeerTrimVolChangeCmd cmd)
{

    switch(cmd)
    {
        case PEER_TRIM_VOL_MASTER_UP:
            MessageSend(&theSink.task , EventUsrMasterDeviceTrimVolumeUp , 0);
            break;

        case PEER_TRIM_VOL_MASTER_DOWN:
            MessageSend(&theSink.task , EventUsrMasterDeviceTrimVolumeDown , 0);
            break;

        case PEER_TRIM_VOL_SLAVE_UP:
            MessageSend(&theSink.task , EventUsrSlaveDeviceTrimVolumeUp , 0);
            break;

        case PEER_TRIM_VOL_SLAVE_DOWN:
            MessageSend(&theSink.task , EventUsrSlaveDeviceTrimVolumeDown , 0);
            break;

        default:
            break;
        }
}


/*************************************************************************
NAME
    peerUpdateMasterTrimVolume

DESCRIPTION
   Sends the appropriate trim volume change command to the master peer device.

RETURNS
    None

**************************************************************************/
void peerSendDeviceTrimVolume(volume_direction dir, tws_device_type tws_device)
{
    /* Audio routed from a Peer device which supports TWS as a Source.  This takes priority over ShareMe and thus means we are in a TWS session */
    PeerTrimVolChangeCmd cmd = PEER_TRIM_VOL_NO_CHANGE;
    if (tws_device == tws_master)
    {
        switch (dir)
        {
        case increase_volume:
            cmd = PEER_TRIM_VOL_MASTER_UP;
            break;

        case decrease_volume:
            cmd = PEER_TRIM_VOL_MASTER_DOWN;
            break;

        case same_volume:
        default:
            cmd = PEER_TRIM_VOL_NO_CHANGE;
            break;
        }
    }
    else if (tws_device == tws_slave)
    {
        switch (dir)
        {
        case increase_volume:
            cmd = PEER_TRIM_VOL_SLAVE_UP;
            break;

        case decrease_volume:
            cmd = PEER_TRIM_VOL_SLAVE_DOWN;
            break;

        case same_volume:
        default:
            cmd = PEER_TRIM_VOL_NO_CHANGE;
            break;
        }
    }
    if(cmd != PEER_TRIM_VOL_NO_CHANGE)
    {
        u16 avrcp_index;

        if ( avrcpGetPeerIndex(&avrcp_index) )
        {
            sinkAvrcpVendorUniquePassthroughRequest(avrcp_index , AVRCP_PEER_CMD_UPDATE_TRIM_VOLUME, sizeof(u16), (const u8 *)&cmd);
        }
    }
}


/*************************************************************************
NAME
    peerHandleRemoteAGConnected

DESCRIPTION
    Checks if the sink device is already connected to an AG with the same bd address as the one connected to the peer,
    if yes, then disconnect the local AG connected if the bd addr of the sink device is lower than that of peer.

RETURNS
    None

**************************************************************************/
void peerHandleRemoteAgConnected(void)
{
    if(theSink.remote_peer_audio_conn_status & A2DP_AUDIO)
    {
        u16 avA2dpId;
        u16 peerA2dpId;
        /* check if the bd address of the local AG is the same as the AG connected to the peer */
        if(a2dpGetPeerIndex (&peerA2dpId) && a2dpGetSourceIndex(&avA2dpId) &&
            BdaddrIsSame(theSink.remote_peer_ag_bd_addr, &(theSink.a2dp_link_data->bd_addr[avA2dpId])))
        {
            sinkAvrcpDisconnect(&(theSink.a2dp_link_data->bd_addr[avA2dpId]));
            A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[avA2dpId]);
            sinkDisconnectAllSlc();
        }
    }
}

/*************************************************************************
NAME
    compareBdAddr

DESCRIPTION
    Compares the first and the second bdaddr.
RETURNS
    TRUE if the first is greater than second, otherwise return FALSE.

**************************************************************************/
bool peerCompareBdAddr(const bdaddr* first , const bdaddr* second)
{
    if( first->nap == second->nap)
    {
        if(first->uap == second->uap)
        {
            if(first->lap == second->lap)
            {
                return FALSE;
            }
            else
            {
                if(first->lap > second->lap)
                {
                    return TRUE;
                }
            }
        }
        else
        {
            if(first->uap > second->uap)
            {
                return TRUE;
            }
        }
    }
    else
    {
        if(first->nap > second->nap)
        {
            return TRUE;
        }
    }
    return FALSE;
}


/*************************************************************************
NAME
    peerSendAudioEnhancements

DESCRIPTION
    Sends audio enhancement setting to currently connected Peer

RETURNS
    None

**************************************************************************/
void peerSendAudioEnhancements()
{
    u16 peerA2dpId;

    if(a2dpGetPeerIndex(&peerA2dpId))
    {
        /* Send the audio enhancement settings to the peer if this device is a TWS master*/
        if((theSink.a2dp_link_data->seid[peerA2dpId] & (SOURCE_SEID_MASK | TWS_SEID_MASK)) == (SOURCE_SEID_MASK | TWS_SEID_MASK))
        {
            u16 peerAvrcpIndex;
            for_all_avrcp(peerAvrcpIndex)
            {
                if(theSink.avrcp_link_data && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peerA2dpId], &theSink.avrcp_link_data->bd_addr[peerAvrcpIndex]))
                {
                    u8 audio_enhancements_array[2];
                    u16 audio_enhancements = ((theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing -
                            A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0) & A2DP_MUSIC_CONFIG_USER_EQ_SELECT)|
                            (theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements & ~A2DP_MUSIC_CONFIG_USER_EQ_SELECT);

                    audio_enhancements_array[0] = (u8)audio_enhancements;
                    audio_enhancements_array[1] = (u8)(audio_enhancements >> 8);

                    sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex , AVRCP_PEER_CMD_UPDATE_AUDIO_ENHANCEMENT_SETTINGS,
                       sizeof(audio_enhancements_array), (const u8 *)audio_enhancements_array);

                    break;
                }
            }
        }
    }
}

/*************************************************************************
NAME
    peerSendEqSettings

DESCRIPTION
    Sends DSP EQ setting to currently connected Peer

RETURNS
    None

**************************************************************************/
void peerSendUserEqSettings(void)
{
    u16 peerA2dpId;
    u16 i;

    if(a2dpGetPeerIndex(&peerA2dpId))
    {
        /* Send the audio enhancement settings to the peer if this device is a TWS master*/
        if((theSink.a2dp_link_data->seid[peerA2dpId] & (SOURCE_SEID_MASK | TWS_SEID_MASK)) == (SOURCE_SEID_MASK | TWS_SEID_MASK))
        {
            u16 peerAvrcpIndex;
            for_all_avrcp(peerAvrcpIndex)
            {
                if(theSink.avrcp_link_data && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peerA2dpId], &theSink.avrcp_link_data->bd_addr[peerAvrcpIndex]))
                {
                    u16 size_data = (NUM_USER_EQ_BANDS * USER_EQ_BAND_PARAMS_SIZE) + USER_EQ_PARAM_PRE_GAIN_SIZE;
                    u8 *user_eq_params_array = (u8 *)mallocPanic( sizeof(u8) * size_data) ;
                    u8 *local_data_array = user_eq_params_array;

                    if((local_data_array != NULL) && (theSink.PEQ != NULL))
                    {
                        /*Store the data into the u8 array */
                        local_data_array[PRE_GAIN_LO_OFFSET] = LOBYTE(theSink.PEQ->preGain);
                        local_data_array[PRE_GAIN_HI_OFFSET] = HIBYTE(theSink.PEQ->preGain);

                        /*Increment the data pointer with the no. of bytes required to store the Pre-gain value */
                        local_data_array += USER_EQ_PARAM_PRE_GAIN_SIZE ;

                        for(i=0; i<NUM_USER_EQ_BANDS ; i++)
                        {
                            local_data_array[BAND_FILTER_OFFSET]= (u8)theSink.PEQ->bands[i].filter;
                            local_data_array[BAND_FREQ_LO_OFFSET]=  LOBYTE(theSink.PEQ->bands[i].freq);
                            local_data_array[BAND_FREQ_HI_OFFSET]=  HIBYTE(theSink.PEQ->bands[i].freq);
                            local_data_array[BAND_GAIN_LO_OFFSET]=  LOBYTE(theSink.PEQ->bands[i].gain);
                            local_data_array[BAND_GAIN_HI_OFFSET]=  HIBYTE(theSink.PEQ->bands[i].gain);
                            local_data_array[BAND_Q_LO_OFFSET]=  LOBYTE(theSink.PEQ->bands[i].Q);
                            local_data_array[BAND_Q_HI_OFFSET]=  HIBYTE(theSink.PEQ->bands[i].Q);

                            /*Increment the data pointer with the no. of bytes required to store all the band parameters */
                            local_data_array += USER_EQ_BAND_PARAMS_SIZE ;
                        }

                        local_data_array = NULL;

                        sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex , AVRCP_PEER_CMD_UPDATE_USER_EQ_SETTINGS,
                                                                                                  size_data, (const u8 *)user_eq_params_array);
                        free(user_eq_params_array);

                        break;
                    }
                }
            }
        }
    }
}


/*************************************************************************
NAME
    peerRequestUserEqSetings

DESCRIPTION
    Request current DSP EQ setting from the connected Peer (Master)

RETURNS
    None

**************************************************************************/
void peerRequestUserEqSetings(void)
{
    u16 peerA2dpId;
    a2dp_link_priority priority;

    if(a2dpGetPeerIndex(&peerA2dpId))
    {
        /* Request the current user eq settings from the peer if this device is a TWS Slave*/
        if(findCurrentA2dpSource(&priority) && (theSink.a2dp_link_data->peer_device[priority] == remote_device_peer))
        {
            u16 peerAvrcpIndex;
            for_all_avrcp(peerAvrcpIndex)
            {
                if(theSink.avrcp_link_data && BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peerA2dpId], &theSink.avrcp_link_data->bd_addr[peerAvrcpIndex]))
                {
                    sinkAvrcpVendorUniquePassthroughRequest(peerAvrcpIndex , AVRCP_PEER_CMD_REQUEST_USER_EQ_SETTINGS, 0 , NULL);
                }
            }
        }
    }
}


/*************************************************************************
NAME
    peerConnectPeer

DESCRIPTION
    Attempts to connect a TWS Peer, if not already in a Peer session

RETURNS
    TRUE if a connection is attempted, FALSE otherwise

**************************************************************************/
bool peerConnectPeer (void)
{
    if (!a2dpGetPeerIndex(NULL))
    {
        typed_bdaddr  peer_addr;
        sink_attributes peer_attributes;
        u16 list_idx;
        u16 list_size = ConnectionTrustedDeviceListSize();

        for (list_idx = 0; list_idx < list_size; list_idx++)
        {
            /* attempt to obtain the device attributes for the current ListID required */
            if (deviceManagerGetIndexedAttributes(list_idx, &peer_attributes, &peer_addr))
    {
                if ((peer_attributes.peer_device == remote_device_peer) &&
                    (peer_attributes.peer_features & (remote_features_tws_a2dp_sink | remote_features_tws_a2dp_source)))
                {   /* Only attempt to connect a TWS Peer */
                    slcConnectDevice(&peer_addr.addr, peer_attributes.profiles);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    peerObtainPairingMode

DESCRIPTION
    Obtains the pairing mode used for the currently connected Peer device

RETURNS
    None

**************************************************************************/
PeerPairingMode peerObtainPairingMode (u16 peer_id)
{
    PeerPairingMode pairing_mode = PEER_PAIRING_MODE_TEMPORARY;   /* By default, devices use temporary pairing */

    if (theSink.a2dp_link_data->peer_features[peer_id] & (remote_features_tws_a2dp_sink | remote_features_tws_a2dp_source))
    {   /* If remote Peer supports TWS then use the configured TWS pairing mode */
        pairing_mode = theSink.features.TwsPairingMode;
        LOGD("PEER: TWS Pairing Mode = %u\n",pairing_mode);
    }
    else
    {
        if (theSink.a2dp_link_data->peer_features[peer_id] & (remote_features_shareme_a2dp_sink | remote_features_shareme_a2dp_source))
        {   /* If remote Peer supports ShareMe and not TWS then use the configured ShareMe pairing mode */
            pairing_mode = theSink.features.ShareMePairingMode;
            LOGD("PEER: ShareMe Pairing Mode = %u\n",pairing_mode);
        }
    }

    return pairing_mode;
}

/*************************************************************************
NAME
    peerUpdatePairing

DESCRIPTION
    Ensures permanent pairing data is updated

RETURNS
    None

**************************************************************************/
void peerUpdatePairing (u16 peer_id, void *peer_attributes)
{
    switch (peerObtainPairingMode(peer_id))
    {
    case PEER_PAIRING_MODE_TEMPORARY:
    case PEER_PAIRING_MODE_NORMAL:
        break;
    case PEER_PAIRING_MODE_PERMANENT:
        /* Permanent pairing is enabled for Peer devices */
        LOGD("PEER: Add/update permanent pairing for Peer %u\n", peer_id);
        AuthUpdatePermanentPairing(&theSink.a2dp_link_data->bd_addr[peer_id], peer_attributes);
        break;
    }
}

/*************************************************************************
NAME
    peerInitPeerStatus

DESCRIPTION
    Inits peer data structure and sets the initial required state

RETURNS
    None

**************************************************************************/
void peerInitPeerStatus (void)
{
    LOGD("peerInitPeerStatus\n");

    memset(&theSink.peer, 0, sizeof(theSink.peer));
    determineRequiredState();
#ifdef ENABLE_PEER_BATTERY_LEVEL
    theSink.peer_battery_level = BATTERY_LEVEL_INVALID;
#endif
}

/*************************************************************************
NAME
    peerIsTwsMaster

DESCRIPTION
    Is this device a streaming TWS Master

RETURNS
    None

**************************************************************************/
bool peerIsTwsMaster(void)
{
    u16 peerA2dpId;

    if(a2dpGetPeerIndex(&peerA2dpId))
    {
        /* Are we a streaming TWS master? */
        if((theSink.a2dp_link_data->seid[peerA2dpId] & (SOURCE_SEID_MASK | TWS_SEID_MASK)) == (SOURCE_SEID_MASK | TWS_SEID_MASK))
        {
            /* A streaming TWS Master */
            return TRUE;
        }
    }

    return FALSE;
}

/*************************************************************************
NAME
    peerGetLocalSupportedCodecs

DESCRIPTION
    Returns the list of optional codecs supported by the local peer device

RETURNS
    TRUE if the list is present othewise FALSE

**************************************************************************/
bool peerGetLocalSupportedCodecs(u16 *codecs)
{
    if(theSink.a2dp_link_data)
    {
        if(codecs)
        {
            *codecs = theSink.a2dp_link_data->local_peer_optional_codecs;
        }

        return TRUE;
    }

    return FALSE;
}


/*************************************************************************
NAME
    peerGetRemoteSupportedCodecs

DESCRIPTION
    Returns the list of optional codecs supported by the remote peer device

RETURNS
    TRUE if the list is present othewise FALSE


**************************************************************************/
bool peerGetRemoteSupportedCodecs(u16 *codecs)
{
    if(theSink.a2dp_link_data)
    {
        if(codecs)
        {
            *codecs = theSink.a2dp_link_data->remote_peer_optional_codecs;
        }

        return TRUE;
    }

    return FALSE;
}



/*************************************************************************
NAME
    peerSetLocalPeerSupportedCodecs

DESCRIPTION
    Stores the list of optional codecs supported by the local peer device

RETURNS
    None

**************************************************************************/
void peerSetLocalSupportedCodecs(u16 codecs)
{
    if(theSink.a2dp_link_data)
    {
        theSink.a2dp_link_data->local_peer_optional_codecs = codecs;
    }
}

/*************************************************************************
NAME
    peerSetRemoteSupportedCodecs

DESCRIPTION
    Stores the list of optional codecs supported by the remote peer device

RETURNS
    None

**************************************************************************/
void peerSetRemoteSupportedCodecs(u16 codecs)
{
    LOGD("PEER: peerSetRemoteSupportedCodecs 0x%x\n", codecs);

    if(theSink.a2dp_link_data)
    {
        theSink.a2dp_link_data->remote_peer_optional_codecs = codecs;
    }
}

/****************************************************************************
NAME
    peerLinkReservedCanDeviceConnect

DESCRIPTION
    Determine whether the given device can be connected based on whether we have reserved a link
    for a peer device to connect.

RETURNS
    TRUE or FALSE
*/
bool peerLinkReservedCanDeviceConnect(const bdaddr *bd_addr)
{
    u16 a2dp_index;

    if(!theSink.PeerLinkReserved)
    {
        /* We have not reserved a link for peer device to connect so just return TRUE*/
        return TRUE;
    }

    if(!(getA2dpIndexFromBdaddr(bd_addr , &a2dp_index) && a2dpIsIndexPeer(a2dp_index)))
    {
        if(deviceManagerNumOtherConnectedDevs(bd_addr) == deviceManagerNumConnectedPeerDevs())
        {
            /* We are only connected to a peer device so let the non-peer device connect.*/
            return TRUE;
        }
    }
    else
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
NAME
    peerReserveLink

DESCRIPTION
    Turns on or off the feature to reserve a link for the peer device to connect.

RETURNS
    void
*/
void peerReserveLink(bool enable)
{
    theSink.PeerLinkReserved = enable;
    configManagerWriteSessionData();

    if(enable)
    {
        /* If the user has requested to reserve a link for the peer and both links are connected then the
                 connection to non-gaia device will be dropped otherwise both the links are dropped*/
        deviceManagerDisconnectNonGaiaDevices();
    }
}

/*************************************************************************
NAME
    peerUpdateLocalStatusOnCallEnd

DESCRIPTION
    Responsible for updating the relay state when a call ends. The function
    call handles the scenarios when the device is in Single Device Mode (SDM)
    or Non Single Device Mode.

RETURNS
    None

**************************************************************************/
void peerUpdateLocalStatusOnCallEnd(void)
{
    a2dp_link_priority priority;

    /* When the call has ended/inactive/idle: Check if in NON single device mode AND whether it is connected to an AG */
    if( !theSink.features.TwsSingleDeviceOperation && findCurrentStreamingA2dpSource(&priority) && !a2dpIsIndexPeer(priority) )
    {
        /* mark the relay as unavailable */
        peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_CALL_INACTIVE | PEER_STATUS_CHANGE_RELAY_UNAVAILABLE);
    }
    else
    {
        /* in single device mode operation, mark the relay as available */
        peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_CALL_INACTIVE);
    }
}


/*************************************************************************
NAME
    peerUpdateBatteryLevel

DESCRIPTION
    This function is used by sink_avrcp to update the peer battery level when
    a AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL message is received from a connected
    peer.

RETURNS
    TRUE on success

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
bool peerUpdateBatteryLevel(u16 battery_level)
{
    LOGD("peerUpdateBatteryLevel %d\n", battery_level);
    theSink.peer_battery_level = battery_level;
    return TRUE;
}
#endif

/*************************************************************************
NAME
    peerGetBatteryLevel

DESCRIPTION
    This function can be used to retrive the cached battery level of connected
    peer device.

RETURNS
    Battery level in percentage
    BATTERY_LEVEL_INVALID on error

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
u16 peerGetBatteryLevel(void)
{
    return theSink.peer_battery_level;
}
#endif

#ifdef PEER_TWS
static RelaySource getPeerRelaySourceFromEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource)
{
    RelaySource relay_source = RELAY_SOURCE_NULL;

    switch(EventUsrSelectAudioSource)
    {
        case EventUsrSelectAudioSourceAnalog:
            relay_source = RELAY_SOURCE_ANALOGUE;
            break;

        case EventUsrSelectAudioSourceUSB:
            relay_source = RELAY_SOURCE_USB;
            break;

        case EventUsrSelectAudioSourceAG1:
        case EventUsrSelectAudioSourceAG2:
            relay_source = RELAY_SOURCE_A2DP;
            break;
        default:
            Panic(); /* Unexpected event */
            break;
    }
    return relay_source;
}

static void autoSourceSelectionHandleSourceEvent(const MessageId EventUsrSelectAudioSource)
{
    setPeerSource(getPeerRelaySourceFromEventUsrSelectAudioSource(EventUsrSelectAudioSource));
}

static void autoSourceSelectionHandleNextSourceEvent(void)
{
    /* Select next source based on the currently selected one */
    switch (getPeerSource())
    {
        case RELAY_SOURCE_NULL:
            MessageSend(&theSink.task, EventUsrSelectAudioSourceAnalog, 0);
            break;
        case RELAY_SOURCE_ANALOGUE:
            MessageSend(&theSink.task, EventUsrSelectAudioSourceUSB, 0);
            break;
        case RELAY_SOURCE_USB:
            MessageSend(&theSink.task, EventUsrSelectAudioSourceAG1, 0);
            break;
        case RELAY_SOURCE_A2DP:
            MessageSend(&theSink.task, EventUsrSelectAudioSourceAnalog, 0);
            break;
        default:
            Panic(); /* Unexpected peer source */
            break;
    }
}

bool peerAutoSourceSelectHandleEvent(const MessageId EventUsrSelectAudioSource)
{
    if(EventUsrSelectAudioSource == EventUsrSelectAudioSourceNext)
    {
        autoSourceSelectionHandleNextSourceEvent();
    }
    else
    {
        autoSourceSelectionHandleSourceEvent(EventUsrSelectAudioSource);
    }
    return TRUE;
}
#endif

/****************************************************************************
NAME
	getPeerDSPBufferingRequired

DESCRIPTION
    function to deteremine whether TWS backwards compatibility mode within the
    DSP is required, TWS version 3.1.0 uses an internal delay of 350ms to allow
    the use of larger buffers, this is incompatible with earlier versions of TWS
    which only use a 250ms internal delay, therefore to maintain backwards
    compatibility with older versions of TWS it is necessary to send a configuration
    message to the DSP to select the internal delay, STD or backwards compatibility
    required.

RETURNS
	peer_buffer_level
*/
peer_buffer_level getPeerDSPBufferingRequired(u16 remote_peer_version)
{

    /* check for older version of a peer device supporting lower buffering levels */
    if((remote_peer_version) && (remote_peer_version < PEER_DEVICE_MIN_SUPPORTED_BUFFER_COMPATIBILITY_VERSION))
    {
        /* reported as an older version of TWS, therefore backwards comaptiblity
           is required */
        return PEER_BUFFER_250MS ;
    }
    /* check for newer version of a peer device supporting higher buffering levels */
    else if(remote_peer_version)
    {
        /* peer device whose version is 3.1.0 or higher supporting higher buffering rates */
        return PEER_BUFFER_350MS;
    }
    else
    {
        /* no backwards compatiblity is required since this is not a peer device */
        return PEER_BUFFER_NON_PEER_DEVICE;
    }
}
#endif  /* ENABLE_PEER */

