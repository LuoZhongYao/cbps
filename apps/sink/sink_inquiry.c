
/****************************************************************************

Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_inquiry.c

DESCRIPTION
    manages inquiry/pairing with devices based on RSSI

NOTES

*/

/***************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_inquiry.h"
#include "sink_slc.h"
#include "sink_statemanager.h"
#include "sink_devicemanager.h"
#include "sink_device_id.h"
#include "sink_peer.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <bdaddr.h>

static void inquiryConnect (u8 index);

#ifdef DEBUG_INQ
#else
#endif

/* RSSI configuration block */
#define NUM_INQ_DEVS        ((u8)RSSI_CONF.num_results)
#define NUM_INQ_RESULTS     (NUM_INQ_DEVS + 1)
/* Is RSSI action to connect or pair? */
#define RSSI_CONNECTING     (theSink.inquiry.action == rssi_connecting)
#define RSSI_PAIRING        (theSink.inquiry.action == rssi_pairing)
/* Get thresholds dependant on whether we're connecting or pairing */
#define RSSI_DIFF_THRESHOLD (RSSI_CONNECTING ? (int)(RSSI_CONF.conn_diff_threshold) : (int)(RSSI_CONF.diff_threshold))
#define RSSI_THRESHOLD      (RSSI_CONNECTING ? RSSI_CONF.conn_threshold : RSSI_CONF.threshold)
/* Check if device is in the PDL */
#define RSSI_CAN_CONNECT(x) (RSSI_CONF.connect_if_in_pdl ? deviceManagerGetAttributes(NULL, x) : TRUE)
#define RSSI_CAN_PAIR(x)    (RSSI_CONF.dont_pair_if_in_pdl ? !deviceManagerGetAttributes(NULL, x) : TRUE)
#define RSSI_CHECK_PDL(x)   (RSSI_CONNECTING ? RSSI_CAN_CONNECT(x) : RSSI_CAN_PAIR(x))

#define LIAC 0x9E8B00
#define GIAC 0x9E8B33

#ifdef ENABLE_PEER
/* Service search patterns */
/* DataElSeq(0x35), Length(0x03), 16-bit UUID(0x19), HSP AG(0x1112) */
static const u8 hsp_service_search_pattern[] = {0x35, 0x03, 0x19, 0x11, 0x12};
/* DataElSeq(0x35), Length(0x03), 16-bit UUID(0x19), HFP AG(0x111F) */
static const u8 hfp_service_search_pattern[] = {0x35, 0x03, 0x19, 0x11, 0x1F};
/* DataElSeq(0x35), Length(0x03), 16-bit UUID(0x19), Audio Source(0x110A) */
static const u8 a2dp_service_search_pattern[] = {0x35, 0x03, 0x19, 0x11, 0x0A};
/* DataElSeq(0x35), Length(0x03), 16-bit UUID(0x19), AVRCP Target(0x110C) */
static const u8 avrcp_service_search_pattern[] = {0x35, 0x03, 0x19, 0x11, 0x0C};

/* Service search list */
static const struct
{
    supported_profiles profile;
	u16             size;
	const u8 *const pattern;
} service_searches[] =
{
	{ profile_hsp  , sizeof(hsp_service_search_pattern)  , hsp_service_search_pattern   },
    { profile_hfp  , sizeof(hfp_service_search_pattern)  , hfp_service_search_pattern   },
	{ profile_a2dp , sizeof(a2dp_service_search_pattern) , a2dp_service_search_pattern  },
    { profile_avrcp, sizeof(avrcp_service_search_pattern), avrcp_service_search_pattern }
};


/* Prototype and task structure for RSSI Inquiry specific SDP searches */
static void handleInquiryMessage (Task task, MessageId id, Message message);
static const struct TaskData inquiry_task = { handleInquiryMessage };
#endif


#ifdef ENABLE_PEER
static bool performSdpSearch (const bdaddr *bd_addr, u16 search_idx)
{
    LOGD("performSdpSearch ");
	if (search_idx < SUPPORTED_PROFILE_COUNT)
	{
        LOGD("- profile %u\n", service_searches[search_idx].profile);
		ConnectionSdpServiceSearchRequest((Task)&inquiry_task, bd_addr, 1, service_searches[search_idx].size, service_searches[search_idx].pattern);
		return TRUE;
	}
    else
    {
        LOGD("- completed\n");
        return FALSE;
    }
}
#endif

#ifdef ENABLE_PEER
static void initiateSdpSearch (const bdaddr *bd_addr)
{
    LOGD("initiateSdpSearch ");
	/*ConnectionSmSetSdpSecurityOut(TRUE, bd_addr);*/ /* Allow SDP searches to be performed without locally enforcing authentication */
	ConnectionSdpOpenSearchRequest((Task)&inquiry_task, bd_addr);  /* Generates a CL_SDP_OPEN_SEARCH_CFM */

    theSink.inquiry.profile_search_idx = 0;
    theSink.inquiry.remote_profiles = 0;
}
#endif

#ifdef ENABLE_PEER
static sink_link_type getFirstConnectableProfile (supported_profiles remote_profiles)
{
    LOGD("First connectable profile ");
    if (remote_profiles & (profile_hsp | profile_hfp))
    {
        LOGD("= hfp/hsp\n");
        return sink_hfp;
    }
    else if (remote_profiles & profile_a2dp)
    {
        LOGD("= a2dp\n");
        return sink_a2dp;
    }
    else
    {
        LOGD("= none\n");
        return sink_none;
    }
}
#endif

#ifdef ENABLE_PEER
static void handleSdpOpenCfm (CL_SDP_OPEN_SEARCH_CFM_T *cfm)
{
    u8 index = theSink.inquiry.attempting;
    inquiry_result_t* device = &theSink.inquiry.results[index];
    performSdpSearch(&device->bd_addr, theSink.inquiry.profile_search_idx);
}

static void handleSdpCloseCfm (CL_SDP_CLOSE_SEARCH_CFM_T *cfm)
{
    u8 index = theSink.inquiry.attempting;
    inquiry_result_t* device = &theSink.inquiry.results[index];
    sink_link_type first_profile = getFirstConnectableProfile(theSink.inquiry.remote_profiles);

    if (first_profile)
    {   /* Attempt connection */
        LOGD("slcConnectDevice\n");
        slcConnectDevice(&device->bd_addr, first_profile);
    }
    else
    {   /* Device does not support a profile we can connect to */
        LOGD("inquiryConnectNext\n");
        inquiryConnectNext();
    }
}

static void handleSdpServiceSearchAttributeCfm (CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    if(theSink.inquiry.results)
    {
        /* We are inquiring, so no A2DP connect request has been issued yet to this device, thus there is no entry in a2dp_link_data */
        inquiry_result_t *inquiry_results = &theSink.inquiry.results[theSink.inquiry.attempting];


         /* Response is due to a request for the Peer Device service record */
        if ( (cfm->status==sdp_response_success) && !cfm->error_code )
        {   /* Response obtained, examine it */
            u16 remote_peer_version = peerGetRemoteVersionFromServiceRecord(cfm->attributes, cfm->size_attributes);
            remote_features remote_peer_features = peerGetRemoteSupportedFeaturesFromServiceRecord(cfm->attributes, cfm->size_attributes);

            peerSetRemoteSupportedCodecsFromServiceRecord(cfm->attributes, cfm->size_attributes);

            inquiry_results->peer_features = peerDetermineCompatibleRemoteFeatures(remote_peer_features, remote_peer_version);
            inquiry_results->peer_device = remote_device_peer;
            theSink.inquiry.peer_version = remote_peer_version; /* Stored here to save RAM usage in theSink.inquiry.results[] */
        }
        else
        {
            inquiry_results->peer_features = remote_features_none;
            inquiry_results->peer_device = remote_device_nonpeer;
            theSink.inquiry.peer_version = 0;   /* Stored here to save RAM usage in theSink.inquiry.results[] */
        }

        if ((inquiry_results->peer_device != remote_device_peer) ||                                                                 /* A non-Peer device */
            ((inquiry_results->peer_features != remote_features_none) && peerIsCompatibleDevice(theSink.inquiry.peer_version) ) )   /* A Peer device     */
        {   /* Always allow non peer devices to connect.  Peer devices are restricted by version number and feature set */
            LOGD("Device compatible - inquiryConnect\n");
            inquiryConnect(theSink.inquiry.attempting);
        }
        else
        {   /* Device not suitable for connection */
            LOGD("Device NOT compatible - inquiryConnectNext\n");
            inquiryConnectNext();
        }
    }
}

static void handleSdpServiceSearchCfm (CL_SDP_SERVICE_SEARCH_CFM_T *cfm)
{
    if (cfm->status != sdp_search_busy)
    {
#ifdef DEBUG_PRINT_ENABLED
        u16 n=cfm->size_records;
        u16 i;
        LOGD("Sdp service search result [ ");
        for (i=0; i<n; i++)
        {
            LOGD("0x%X ",cfm->records[i]);
        }
        LOGD("]\n");
#endif

        if (cfm->status == sdp_response_success)
        {
            LOGD("Device supports profile %u\n", service_searches[theSink.inquiry.profile_search_idx].profile);
            theSink.inquiry.remote_profiles |= service_searches[theSink.inquiry.profile_search_idx].profile;
        }
        else
        {
            LOGD("Device does NOT support %u\n", service_searches[theSink.inquiry.profile_search_idx].profile);
            theSink.inquiry.remote_profiles &= ~service_searches[theSink.inquiry.profile_search_idx].profile;
        }

        if ( !performSdpSearch(&cfm->bd_addr, ++theSink.inquiry.profile_search_idx) )
        {
            ConnectionSdpCloseSearchRequest((Task)&inquiry_task);  /* Generates a CL_SDP_CLOSE_SEARCH_CFM */
        }
    }
}




/****************************************************************************
NAME
	handleInquiryMessage

DESCRIPTION
    Task function to handle responses to SDP search requests made during RSSI inquiry

RETURNS
	void
*/
static void handleInquiryMessage (Task task, MessageId id, Message message)
{
    switch (id)
    {
    case CL_SDP_OPEN_SEARCH_CFM:
        LOGD("CL_SDP_OPEN_SEARCH_CFM\n");
        handleSdpOpenCfm((CL_SDP_OPEN_SEARCH_CFM_T *) message);
        break;

    case CL_SDP_CLOSE_SEARCH_CFM:
        LOGD("CL_SDP_CLOSE_SEARCH_CFM\n");
        handleSdpCloseCfm((CL_SDP_CLOSE_SEARCH_CFM_T *) message);
        break;

    case CL_SDP_SERVICE_SEARCH_CFM:
        LOGD("CL_SDP_SERVICE_SEARCH_CFM\n");
        handleSdpServiceSearchCfm ((CL_SDP_SERVICE_SEARCH_CFM_T*) message);
        break;

    case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
        LOGD("CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM\n");
        handleSdpServiceSearchAttributeCfm ((CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T*) message);
        break;

    default:
        LOGD("Unexpected message, id=0x%X\n", id);
        break;
    }
}
#endif  /* ENABLE_PEER */


/****************************************************************************
NAME
    inquiryResetEntry

DESCRIPTION
    Reset inquiry data to initial values
RETURNS
    void
*/
static void inquiryResetEntry(u8 index)
{
    BdaddrSetZero(&theSink.inquiry.results[index].bd_addr);
    theSink.inquiry.results[index].rssi = RSSI_THRESHOLD;
}


/****************************************************************************
NAME
    inquiryReset

DESCRIPTION
    Reset inquiry data to initial values
RETURNS
    void
*/
void inquiryReset(void)
{
    u8 index;
    LOGD("Reset\n");
    if(theSink.inquiry.results)
        for(index=0; index < NUM_INQ_RESULTS; index++)
            inquiryResetEntry(index);

    theSink.inquiry.attempting = 0;
}


/****************************************************************************
NAME
    inquiryResume

DESCRIPTION
    Reset inquiry results and resume
RETURNS
    void
*/
void inquiryResume(void)
{
    if(theSink.inquiry.state == inquiry_idle)
    {
        inquiryReset();
        theSink.inquiry.state = inquiry_searching;

        if (theSink.inquiry.session == inquiry_session_normal)
        {
            LOGD("Resume GIAC %d, %d, 0x%lX\n", RSSI_CONF.max_responses, RSSI_CONF.timeout, RSSI_CONF.cod_filter);
            /* Using GIAC */
            ConnectionInquire(&theSink.task, GIAC, RSSI_CONF.max_responses, RSSI_CONF.timeout, RSSI_CONF.cod_filter);
        }
        else
        {
            if (theSink.features.PeerUseLiac)
            {
                LOGD("Resume LIAC %d, %d, 0x%lX\n", 4, 46, AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS );
                /* Obtain upto 4 responses, using LIAC with 46*1.28sec=58.88sec timeout */
                ConnectionInquire(&theSink.task, LIAC, 4, 46, AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS );
            }
            else
            {
                LOGD("Resume GIAC %d, %d, 0x%lX\n", 4, 46, AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS );
                /* Obtain upto 4 responses, using GIAC with 46*1.28sec=58.88sec timeout */
                ConnectionInquire(&theSink.task, GIAC, 4, 46, AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS );
            }
        }
    }
}


/****************************************************************************
NAME
    inquiryPair

DESCRIPTION
    Kick off inquiry and pairing
RETURNS
    bool
*/
bool inquiryPair (inquiry_session session, bool req_disc)
{
    LOGD("Pair %d \n", theSink.inquiry.action);

    if(theSink.inquiry.action == rssi_none)
    {
        theSink.inquiry.action = rssi_pairing;
        theSink.inquiry.session = session;
        inquiryStart( req_disc );
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
NAME
    inquiryStart

DESCRIPTION
    Kick off inquiry
RETURNS
    void
*/
void inquiryStart (bool req_disc)
{
    if(!theSink.inquiry.results)
    {
        LOGD("Start\n" );

        /* Go discoverable (and disconnect any active SLC) */
        if(RSSI_PAIRING)
        {
            LOGD("RSSI Pairing\n" );
            MessageCancelAll(&theSink.task, EventUsrEstablishSLC);
            stateManagerEnterConnDiscoverableState( req_disc ) ;
        }
        slcReset();

        /* Allocate space to store inquiry results */
        theSink.inquiry.results = mallocPanic(NUM_INQ_RESULTS * sizeof(inquiry_result_t));
        theSink.inquiry.state = inquiry_idle;

        /* Increase page timeout */
        ConnectionSetPageTimeout(16384);

        /* Start a periodic inquiry, this will keep going until we cancel */
        inquiryResume();

        /* Send a reminder event */
        MessageSendLater(&theSink.task, EventSysRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));

        /* Send timeout if enabled */
        if(theSink.conf1->timeouts.InquiryTimeout_s)
            MessageSendLater(&theSink.task, EventSysRssiPairTimeout, 0, D_SEC(theSink.conf1->timeouts.InquiryTimeout_s));
    }
}


/****************************************************************************
NAME
    inquiryStop

DESCRIPTION
    Stop inquiry in progress.
RETURNS
    void
*/
void inquiryStop(void)
{
    /* Free space used for inquiry results */
    if(theSink.inquiry.results)
    {
        LOGD("Stopped\n");

        /* Cancel the inquiry */
        ConnectionInquireCancel(&theSink.task);
        MessageCancelFirst(&theSink.task, EventSysRssiResume);
        MessageCancelFirst(&theSink.task, EventSysRssiPairReminder);
        MessageCancelFirst(&theSink.task, EventSysRssiPairTimeout);
        MessageCancelFirst(&theSink.task, EventSysContinueSlcConnectRequest);

        freePanic(theSink.inquiry.results);
        theSink.inquiry.results = NULL;
        theSink.inquiry.state = inquiry_idle;

        /* Restore Page Timeout */
        ConnectionSetPageTimeout(0);

        /* Continue standard connection procedure */
        if(theSink.inquiry.action == rssi_connecting)
            MessageSend(&theSink.task, EventSysContinueSlcConnectRequest, 0);

        theSink.inquiry.action = rssi_none;
    }

    /* Only change state if we are not in Limbo */
    if (stateManagerGetState() != deviceLimbo)
    {
        if(deviceManagerNumConnectedDevs())
        {
            /* We connected, enter correct state */
            theSink.inquiry.action = rssi_none;
            /* change to connected state if not in a call state */
            if (stateManagerGetState() < deviceConnected)
                stateManagerEnterConnectedState();
        }
        else
        {
            /*No connected devices, enter Connectable/Discoverable appropriately*/
            u16 lNumDevices = ConnectionTrustedDeviceListSize();

            /* Check if we want to go discoverable */
            if ( lNumDevices < theSink.features.DiscoIfPDLLessThan )
            {
                theSink.inquiry.session = inquiry_session_normal;
                stateManagerEnterConnDiscoverableState ( TRUE );
            }
        }
    }
}


/****************************************************************************
NAME
    inquiryTimeout

DESCRIPTION
    Inquiry process has timed out, wait for connections in progress to
    complete and stop inquiring
RETURNS
    void
*/
void inquiryTimeout(void)
{
    LOGD("Timeout - ");
    if(theSink.inquiry.results)
    {
        /* Wait for connect attempts to complete */
        if(theSink.inquiry.state == inquiry_connecting)
        {
            LOGD("Wait for SLC Complete\n");
            theSink.inquiry.state = inquiry_complete;
        }
        else
        {
            LOGD("Stop\n");
            inquiryStop();
        }
    }
    else
    {
        LOGD("Stop\n");
        inquiryStop();
    }
}


/****************************************************************************
NAME
    inquiryCheckBdaddr

DESCRIPTION
    Helper function to check if an address is in inquiry results
*/
static bool inquiryCheckBdaddr(bdaddr *bd_addr, u8* index)
{
    /* Check all devices for a previous entry */
    for((*index) = 0; (*index) < NUM_INQ_RESULTS; (*index)++)
        if(BdaddrIsSame(bd_addr, &theSink.inquiry.results[*index].bd_addr))
            return TRUE;
    return FALSE;
}


/****************************************************************************
NAME
    inquiryGetIndex

DESCRIPTION
    Work out the correct index in inquiry data for a given RSSI value
*/
static void inquiryGetIndex(i16 rssi, u8* index)
{
    for((*index) = 0; (*index) < NUM_INQ_RESULTS; (*index)++)
        if(rssi > theSink.inquiry.results[*index].rssi)
            return;
    (*index)++;
}


/****************************************************************************
NAME
    inquiryConnect

DESCRIPTION
    Connect to the specified inquiry result

RETURNS
    void
*/
static void inquiryConnect (u8 index)
{
    LOGD("State [%d\n]", theSink.inquiry.state);
    LOGD("Connecting [%d]\n", index);

    if(theSink.inquiry.results)
    {
        if(index < NUM_INQ_DEVS || (RSSI_CONF.try_all_discovered && index < NUM_INQ_RESULTS))
        {
            inquiry_result_t* device = &theSink.inquiry.results[index];
            LOGD("Address %04x,%02x,%06lx\n", device->bd_addr.nap, device->bd_addr.uap, device->bd_addr.lap);

            /* Check there's a valid result at position idx */
            if(!BdaddrIsZero(&device->bd_addr))
            {
                /* Allow 2 close devices if multipoint enabled, otherwise just one */
                i16 rssi = theSink.inquiry.results[theSink.MultipointEnable ? 2 : 1].rssi;
                LOGD("RSSI %d Difference %d (%d)\n", device->rssi, (device->rssi - rssi), RSSI_DIFF_THRESHOLD);

                /* Check that difference threshold criteria are met */
                if(RSSI_CONF.try_all_discovered || (device->rssi - rssi) >= RSSI_DIFF_THRESHOLD)
                {
                    /* Don't continue connecting after timeout if try_all_discovered enabled */
                    if(!RSSI_CONF.try_all_discovered || (theSink.inquiry.state != inquiry_complete))
                    {
                        LOGD("Connect\n");

                        /* Ensure the link key for this device is deleted before the connection attempt,
                        prevents reconnection problems with BT2.1 devices.*/
                        if(theSink.inquiry.action == rssi_pairing)
                            deviceManagerRemoveDevice(&device->bd_addr);

                        theSink.inquiry.state = inquiry_connecting;
#ifdef ENABLE_PEER
                        if ((device->peer_device != remote_device_nonpeer) && (device->peer_features == remote_features_none))
                        {   /* A Peer device with unknown features or an unknown device (also with unknown features) */
                            RequestRemotePeerServiceRecord((Task)&inquiry_task, &device->bd_addr);
                        }
                        else if (!device->remote_profiles)
                        {   /* Remote device supported profiles not contained in EIR data - search using SDP */
                            initiateSdpSearch(&device->bd_addr);
                        }
                        else
                        {
                            sink_link_type first_profile = getFirstConnectableProfile(device->remote_profiles);

                            theSink.inquiry.remote_profiles = device->remote_profiles;   /* TODO: Refactor inquiry.remote_profiles out */

                            if (first_profile)
                            {   /* Device supports a profile we can connect to - attempt connection */
                                LOGD("slcConnectDevice\n");
                                slcConnectDevice(&device->bd_addr, first_profile);
                            }
                            else
                            {   /* TODO: Temporary mechanism until this routine is modified to allow for repetition */
                                /* Issue a connect request for HFP */
                                slcConnectDevice(&device->bd_addr, sink_hfp);
                            }
                        }
#else
                        /* Issue a connect request for HFP */
                        slcConnectDevice(&device->bd_addr, sink_hfp);
#endif
                        return;
                    }
                }
            }
        }

        /* We're no longer attempting a connection */
        theSink.inquiry.attempting = 0;

        /* If complete, connecting or pairing with second AG failed - stop, otherwise - resume inquiry */
        if (theSink.inquiry.action == rssi_connecting || theSink.inquiry.state == inquiry_complete ||
            (deviceManagerNumConnectedDevs() && theSink.inquiry.session == inquiry_session_normal ))
        {
            inquiryStop();
        }
        else
        {
            theSink.inquiry.state = inquiry_idle;
            MessageSendLater(&theSink.task, EventSysRssiResume, 0, D_SEC(RSSI_CONF.resume_timeout));
        }
    }
}

static void inquiryConnectFirst(void)
{
    theSink.inquiry.attempting = 0;
    inquiryConnect(theSink.inquiry.attempting);
}

inquiry_result_t* inquiryGetConnectingDevice (void)
{
    inquiry_result_t* connecting_device = NULL;

    if(theSink.inquiry.results != NULL)
    {
        connecting_device = &theSink.inquiry.results[theSink.inquiry.attempting];
    }

    return connecting_device;
}

void inquiryConnectNext(void)
{
    inquiryConnect(++theSink.inquiry.attempting);
}

#ifdef ENABLE_PEER
static supported_profiles getEirRemoteProfiles (u16 size_eir_data, const u8 *eir_data)
{
    u16 i;
    LOGD("\n");
    for (i=0; i<size_eir_data; i++) { LOGD("0x%X ",eir_data[i]); }
    LOGD("\n");

    LOGD("EIR Data Size = %u\n",size_eir_data);

    while (size_eir_data >= 4)  /* Min possible size for a 16-bit service class UUID list EIR record */
    {
        u16 eir_record_size = eir_data[0] + 1;   /* Record size in eir_data[0] does not include length byte, just tag and data size */

        LOGD("EIR Record Size = %u, Tag = 0x%X\n",eir_record_size, eir_data[1]);
        if ((eir_data[1] == 0x02) || (eir_data[1] == 0x03))     /* Partial or complete list of 16-bit service class UUIDs */
        {
            supported_profiles remote_profiles = profile_none;

            do
            {
                eir_record_size -= 2;
                eir_data += 2;

                LOGD("EIR Service UUID:0x%X\n",((eir_data[1]<<8) + eir_data[0]));

                /* Attempt to match next service uuid in EIR record */
                switch ((eir_data[1]<<8) + eir_data[0])
                {
                case 0x110A:    /* A2DP Audio Source */
                    remote_profiles |= profile_a2dp;
                    break;
                case 0x110C:    /* AVRCP Target */
                    remote_profiles |= profile_avrcp;
                    break;
                case 0x1112:    /* HSP AG */
                    remote_profiles |= profile_hsp;
                    break;
                case 0x111F:    /* HFP AG */
                    remote_profiles |= profile_hfp;
                    break;
                }
            }
            while (eir_record_size);

            return remote_profiles;
        }

        if (size_eir_data > eir_record_size)
        {
            size_eir_data -= eir_record_size;
            eir_data += eir_record_size;
        }
        else
        {
            size_eir_data = 0;
        }
    }

    return profile_none;
}
#endif

/****************************************************************************
NAME
    inquiryHandleResult

DESCRIPTION
    Inquiry result received
RETURNS
    void
*/
void inquiryHandleResult( CL_DM_INQUIRE_RESULT_T* result )
{
#ifdef ENABLE_SUBWOOFER
    /* Is the inquiry action searching for a subwoofer device? */
    if (theSink.inquiry.action == rssi_subwoofer)
    {
        handleSubwooferInquiryResult(result);
        return; /* Nothing more to do here as the inquiry result has been handled */
    }
#endif

    /* Check inquiry data is valid (if not we must have cancelled) */
    if(theSink.inquiry.results)
    {
#ifdef DEBUG_INQ
        u8 debug_idx;
        LOGD("Inquiry Result %x Addr %04x,%02x,%06lx RSSI: %d\n", result->status,
             result->bd_addr.nap,
             result->bd_addr.uap,
             result->bd_addr.lap,
             result->rssi ) ;

        for(debug_idx=0; debug_idx<NUM_INQ_RESULTS; debug_idx++)
            LOGD("[Addr %04x,%02x,%06lx RSSI: %d]\n", theSink.inquiry.results[debug_idx].bd_addr.nap,
                 theSink.inquiry.results[debug_idx].bd_addr.uap,
                 theSink.inquiry.results[debug_idx].bd_addr.lap,
                 theSink.inquiry.results[debug_idx].rssi);
#endif
        if(result->status == inquiry_status_result)
        {
#ifdef ENABLE_PEER
            remote_device peer_device = remote_device_unknown;

            if (theSink.features.PeerUseDeviceId)
            {   /* Check for a peer device by matching device id records */
                if (CheckEirDeviceIdData(result->size_eir_data, result->eir_data))
                {   /* Mark device as a peer */
                    LOGD("Matched device id record on remote device\n");
                    peer_device = remote_device_peer;
                    result->rssi += 0x100; /* Bump rssi value by maxiumum possible range so peer devices will be at top of sorted list */
                }
                else
                {
                    LOGD("Did NOT match device id record on remote device\n");
                    peer_device = remote_device_nonpeer;
                }
            }
#endif

#ifdef ENABLE_PEER
            /* Filter out peer/non-peer devices depending on inquiry session */
            LOGD("session=%u device=%u\n", theSink.inquiry.session, peer_device);
            if (((theSink.inquiry.session == inquiry_session_peer) && (peer_device != remote_device_nonpeer)) ||
                ((theSink.inquiry.session == inquiry_session_normal) && (peer_device != remote_device_peer)))
#endif
            {
                /* Check if device is in PDL */
                LOGD("RSSI_CHECK_PDL = %u\n",RSSI_CHECK_PDL(&result->bd_addr));
                if(RSSI_CHECK_PDL(&result->bd_addr))
                {
                    u8 old_index;
                    u8 new_index;
                    inquiry_result_t res;
                    res.bd_addr = result->bd_addr;
                    res.rssi = result->rssi;
#ifdef ENABLE_PEER
                    res.peer_device = peer_device;
                    res.peer_features = remote_features_none;
                    res.remote_profiles = getEirRemoteProfiles(result->size_eir_data, result->eir_data);
                    LOGD("EIR Remote Profiles = %u\n",res.remote_profiles);
#endif
                    inquiryGetIndex(res.rssi, &new_index);
                    LOGD("new_index = %u\n",new_index);

                    /* Check if an entry exists for this device */
                    if(inquiryCheckBdaddr(&res.bd_addr, &old_index))
                    {
                        /* Don't update if new entry further down the list */
                        if(new_index > old_index)
                            return;
                        /* Reset the old entry (it should fall out the bottom) */
                        inquiryResetEntry(old_index);
                    }

                    /* While new index is valid  */
                    while(new_index < NUM_INQ_RESULTS)
                    {
                        inquiry_result_t prev;
                        /* Remember the previous result for this index */
                        prev = theSink.inquiry.results[new_index];
                        /* Put this result in its place */
                        theSink.inquiry.results[new_index] = res;
                        res = prev;
                        /* Get the new index of previous result */
                        inquiryGetIndex(res.rssi, &new_index);
                    }
                }
            }
        }
        else
        {
            LOGD("Inquiry Complete\n");
            /* Attempt to connect to device */
            inquiryConnectFirst();
        }
    }
}
