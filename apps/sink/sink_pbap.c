/****************************************************************************
Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

DESCRIPTION
	Implementation for handling PBAP library messages and functionality

FILE
	sink_pbap.c

*/

/****************************************************************************
    Header files
*/

#include <connection.h>
#include <hfp.h>
#include <print.h>
#include <panic.h>
#include <stdlib.h>
#include <bdaddr.h>
#include <stream.h>
#include <string.h>
#include <sink.h>
#include <source.h>

#ifdef ENABLE_DISPLAY
#include <display.h>

#endif /* ENABLE_DISPLAY */

#ifdef ENABLE_PBAP

#include <pbapc.h>
#include <md5.h>

#include "sink_pbap.h"
#include "sink_private.h"
#include "sink_init.h"
#include "sink_slc.h"
#include "sink_statemanager.h"
#include "sink_callmanager.h"
#include "sink_link_policy.h"
#include "sink_display.h"

#ifdef DEBUG_PBAP
#else
#endif

static const char gpbapbegin[] = "BEGIN:VCARD";
static const char gpbapname[]  = "\nN";
static const char gpbaptel[]   = "TEL";
static const char gpbapend[]   = "END:VCARD";

typedef struct
{
    u8 telLen;
    u8 *pTel;
    u8 nameLen;
    u8 pName[1];
}pbapMetaData;

/* Message Handler Prototypes */
static void handlePbapInitCfm(PBAPC_INIT_CFM_T *pMsg);
static void handlePbapConnectCfm(PBAPC_CONNECT_CFM_T *pMsg);
static void handleAuthRequestInd(PBAPC_AUTH_REQUEST_IND_T *pMsg);
static void handleAuthResponseCfm(PBAPC_AUTH_RESPONSE_CFM_T *pMsg);
static void handlePbapDisconnectInd(PBAPC_DISCONNECT_IND_T *pMsg);
static void handlePbapSetPhonebookCfm(PBAPC_SET_PHONEBOOK_CFM_T *pMsg);

static void handlePullVCardListCfm(PBAPC_PULL_VCARD_LISTING_CFM_T *pMsg);
static void handlePullVCardEntryCfm(PBAPC_PULL_VCARD_ENTRY_CFM_T *pMsg);
static void handlePullPhonebookCfm(PBAPC_PULL_PHONEBOOK_CFM_T *pMsg);

/* App functions */
static void handleAppPullVcardEntry( void );
static void handleAppPullVcardList( void );
static void handleAppPullPhoneBook( void );
static void handleAppPhoneBookSize( void );

static u8 VcardGetFirstTel(const u8* pVcard, const u16 vcardLen, pbapMetaData **pMetaData);
static bool handlePbapDialData(const u8* pVcard, const u16 vcardLen);
static void handlePbapRetrievedData(const u8 *pVcard, const u16 vcardLen);
static void handleVcardPhoneBookMessage(u16 device_id, pbapc_lib_status status, const u8 *lSource, const u16 dataLen);
static void pbapDial(u8 phonebook);

/*
  Initialise the PBAP System
*/
void initPbap(void)
{
    LOGD("initPbap\n");

    /* initialise defaults */
    theSink.pbapc_data.pbap_ready           = FALSE;
    theSink.pbapc_data.pbap_command         = pbapc_action_idle;
    theSink.pbapc_data.pbap_active_link     = pbapc_invalid_link;
    theSink.pbapc_data.pbap_active_pb       = pbap_pb;
    theSink.pbapc_data.PbapBrowseEntryIndex = 1;
    theSink.pbapc_data.pbap_hfp_link        = 0;

    /* Initialise the PBAP library */
	PbapcInit(&theSink.task);
}

/*
	Connect to PBAP Server of the HFP AGs
*/
bool pbapConnect( hfp_link_priority pbapc_hfp_link )
{
    tp_bdaddr tpaddr;
    Sink sink;
    LOGD("PBAP Connect\n");
    if(theSink.features.pbap_enabled)
    {
        if( HfpLinkGetSlcSink(pbapc_hfp_link, &sink) && SinkGetBdAddr(sink, &tpaddr) )
        {
            /* Set the link to active state */
            linkPolicySetLinkinActiveMode(sink);

            theSink.pbapc_data.pbap_command = pbapc_action_idle;

            LOGD("Connecting Pbap profile, Addr %x,%x,%lx\n", tpaddr.taddr.addr.nap, tpaddr.taddr.addr.uap, tpaddr.taddr.addr.lap );
            PbapcConnectRequest(&theSink.task, &tpaddr.taddr.addr);

            theSink.pbapc_data.pbap_hfp_link = pbapc_hfp_link;

            return TRUE;
        }
        else
            LOGD("Connecting Pbap, failed to get bdaddr\n" );
    }
    return FALSE;
}

/*
	Disconnect from all PBAP Servers
*/
void pbapDisconnect( void )
{
    u16 device_id = 0;

    LOGD("Disconnect all Pbap connections\n");

    for(device_id = 0; device_id < MAX_PBAPC_CONNECTIONS; device_id++)
    {
        PbapcDisconnectRequest( device_id );
    }

    theSink.pbapc_data.pbap_command = pbapc_action_idle;
}


/*
    Disconnect PBAP Server with the provided device.
*/
void pbapDisconnectDevice(const bdaddr *bd_addr)
{
    LOGD("Disconnect all Pbap connections\n");

    PbapcDisconnectRequest(PbapcGetLinkFrmAddr(bd_addr));

    theSink.pbapc_data.pbap_command = pbapc_action_idle;
}


/*
	Dial the first entry in the specified phonebook
*/
static void pbapDial(u8 phonebook)
{
    /* attempt to dial the first entry in the AG call history entries */
    if(theSink.pbapc_data.pbap_ready)
    {
        theSink.pbapc_data.pbap_active_pb = phonebook;

        if(theSink.pbapc_data.pbap_active_link != pbapc_invalid_link)
        {
            /* Set the link to active state */
            linkPolicySetLinkinActiveMode(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

            /* the Pbap profile of the primary HFP device has been connected, set the phonebook and dial */
            LOGD("Pbap dial, set the phonebook first\n");
            PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb);
        }
        else
        {
            /* Otherwise, try to connect Pbap profile of the primary HFP device first before dialling */
            /* If primary HFP device fails due to no supported PBAP, try secondary HFP devices        */

            LOGD("Pbap dial, connect the Pbap profile first\n");

            if( !pbapConnect( hfp_primary_link ) )
            {
                MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
                theSink.pbapc_data.pbap_command   = pbapc_action_idle;
                theSink.pbapc_data.pbap_active_pb = 0;
            }
        }
    }
    else
    {
        LOGD("PBAPC profile was not initialised\n");
        MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
        theSink.pbapc_data.pbap_command = pbapc_action_idle;
    }
}

/*
	Dial the first entry in the phonebook
*/
void pbapDialPhoneBook( u8 phonebook )
{
    if(theSink.pbapc_data.pbap_command == pbapc_action_idle)
    {
        if (!stateManagerIsConnected() )
        {
#ifdef ENABLE_AVRCP
            sinkAvrcpCheckManualConnectReset(NULL);
#endif
            LOGD("Pbap dial, Connect the HFP profile first\n");
            MessageSend ( &theSink.task , EventUsrEstablishSLC , 0 ) ;

            switch(phonebook)
            {
                case pbap_ich:
                    sinkQueueEvent( EventUsrPbapDialIch ) ;
                break;
                case pbap_mch:
                    sinkQueueEvent( EventUsrPbapDialMch ) ;
                break;
                default:
                break;
            }
        }
        else
        {
            LOGD("PBAP DialPhoneBook\n");
            pbapDial(phonebook);
        }

        theSink.pbapc_data.pbap_command = pbapc_dialling;
    }
}


/*********************************************************************************
    PBAP Message Handler:
	    Process the Pbapc profile message from library
*********************************************************************************/
void handlePbapMessages(Task task, MessageId pId, Message pMessage)
{
   	LOGD("handlePbapMessages, ID: [%x]\n",pId);

	switch (pId)
	{
	case PBAPC_INIT_CFM:
		handlePbapInitCfm((PBAPC_INIT_CFM_T *)pMessage);
		break;
    case PBAPC_AUTH_REQUEST_IND :
        handleAuthRequestInd((PBAPC_AUTH_REQUEST_IND_T *)pMessage);
        break;
    case PBAPC_AUTH_RESPONSE_CFM:
        handleAuthResponseCfm((PBAPC_AUTH_RESPONSE_CFM_T *)pMessage);
        break;

	case PBAPC_CONNECT_CFM:
		handlePbapConnectCfm((PBAPC_CONNECT_CFM_T *)pMessage);
		break;
	case PBAPC_DISCONNECT_IND:
		handlePbapDisconnectInd((PBAPC_DISCONNECT_IND_T *)pMessage);
		break;
	case PBAPC_SET_PHONEBOOK_CFM:
		handlePbapSetPhonebookCfm((PBAPC_SET_PHONEBOOK_CFM_T *)pMessage);
		break;

	case PBAPC_PULL_VCARD_LISTING_CFM:
        handlePullVCardListCfm((PBAPC_PULL_VCARD_LISTING_CFM_T *)pMessage);
		break;
    case PBAPC_PULL_VCARD_ENTRY_CFM:
        handlePullVCardEntryCfm((PBAPC_PULL_VCARD_ENTRY_CFM_T *)pMessage);
		break;
	case PBAPC_PULL_PHONEBOOK_CFM:
        handlePullPhonebookCfm((PBAPC_PULL_PHONEBOOK_CFM_T *)pMessage);
		break;

    /* Local PBAPC App message */
    case PBAPC_APP_PULL_VCARD_ENTRY:
        handleAppPullVcardEntry();
        break;
    case PBAPC_APP_PULL_VCARD_LIST:
        handleAppPullVcardList();
        break;
    case PBAPC_APP_PULL_PHONE_BOOK:
        handleAppPullPhoneBook();
        break;
    case PBAPC_APP_PHONE_BOOK_SIZE:
        handleAppPhoneBookSize();
        break;

    default:
        LOGD("PBAPC Unhandled message : 0x%X\n",pId);
        break;
	}
}

/* Message Handlers */
static void handlePbapInitCfm( PBAPC_INIT_CFM_T *pMsg)
{
	LOGD("PBAPC_INIT_CFM, status: [%x]\n", pMsg->status);

	if (pMsg->status == pbapc_success)
	{
		LOGD("success\n");
        theSink.pbapc_data.pbap_ready = TRUE;

        /* start initialising the configurable parameters*/
    	InitUserFeatures() ;
	}
	else
	{
        /* Failed to initialise PBAPC */
		LOGD("PBAP init failed   Status : %d\n", pMsg->status);
		Panic();
	}
}

static void handlePbapConnectCfm(PBAPC_CONNECT_CFM_T *pMsg)
{
    LOGD("PBAPC_CONNECT_CFM, device_id : %d,Status : %d, packet size:[%d], repositories:[%d]\n", pMsg->device_id, pMsg->status, pMsg->packetSize, pMsg->repositories);

    if(pMsg->status == pbapc_success)
    {
        if(stateManagerGetState() == deviceLimbo)
        {
            theSink.pbapc_data.pbap_command = pbapc_action_idle;
            PbapcDisconnectRequest(pMsg->device_id);
            return;
        }

        /* If the Pbap of primary HFP device has been connected, save its device_id as the active link */
        if( theSink.pbapc_data.pbap_hfp_link == hfp_primary_link )
        {
            theSink.pbapc_data.pbap_active_link      = pMsg->device_id;
            theSink.pbapc_data.pbap_phone_repository = pMsg->repositories;

            LOGD("PBAPC_CONNECT_CFM, Set the active Pbap link as [%d]\n", theSink.pbapc_data.pbap_active_link);
        }

        /* if we are making Pbapc dialing now if pbap dial is ongoing.*/
        if( (theSink.pbapc_data.pbap_active_link != pbapc_invalid_link) )
        {
            switch(theSink.pbapc_data.pbap_command)
            {
                case pbapc_dialling:
                case pbapc_browsing_entry:
                case pbapc_browsing_list:
                case pbapc_setting_phonebook:
                    PbapcSetPhonebookRequest(theSink.pbapc_data.pbap_active_link,
                                             theSink.pbapc_data.pbap_phone_repository,
                                             theSink.pbapc_data.pbap_active_pb);
                break;
                case pbapc_downloading:
                    MessageSend(&theSink.task , PBAPC_APP_PULL_PHONE_BOOK , 0 ) ;
                break;
                case pbapc_action_idle:
                    /* Set the link policy based on the HFP or A2DP state */
                    linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));
                default:
                break;
            }
        }
    }
    else if(pMsg->status != pbapc_pending)
    {
        Sink sink;

        /* pbapc profile connection failure */
        if(theSink.pbapc_data.pbap_command == pbapc_dialling)
        {
            MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
        }

        /* Set the link policy based on the HFP or A2DP state */
        HfpLinkGetSlcSink(theSink.pbapc_data.pbap_hfp_link, &sink);
        linkPolicyPhonebookAccessComplete( sink );

        theSink.pbapc_data.pbap_command = pbapc_action_idle;
    }

    theSink.pbapc_data.pbap_browsing_start_flag = 0;
}

static void handlePbapDisconnectInd(PBAPC_DISCONNECT_IND_T *pMsg)
{
    LOGD("PBAPC_DISCONNECT_IND, ");

    /* Reset the active pbapc link */
    if(theSink.pbapc_data.pbap_active_link == pMsg->device_id)
    {
        if(PbapcGetNoOfConnection() == 1)
        {
            theSink.pbapc_data.pbap_active_link      = pbapc_secondary_link - pMsg->device_id;
            theSink.pbapc_data.pbap_phone_repository = PbapcGetServerProperties( theSink.pbapc_data.pbap_active_link );
        }
        else
        {
            theSink.pbapc_data.pbap_active_link = pbapc_invalid_link;
        }
        LOGD("change the active pbap link id to [%d]\n", theSink.pbapc_data.pbap_active_link);
    }

    theSink.pbapc_data.pbap_command = pbapc_action_idle;

    theSink.pbapc_data.pbap_browsing_start_flag = 0;

    theSink.pbap_access = FALSE;
}

static void handlePbapSetPhonebookCfm(PBAPC_SET_PHONEBOOK_CFM_T *pMsg)
{
	LOGD("PBAPC_SET_PHONEBOOK_CFM, Status : %d\n", pMsg->status);

    switch(pMsg->status)
    {
        case pbapc_success:
            /* Successfully set the phonebook, pull first entry from the phone book */
            switch(theSink.pbapc_data.pbap_command)
            {
                case pbapc_dialling:
                    MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_ENTRY, 0 ) ;
                break;
                case pbapc_browsing_entry:
                    MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_ENTRY, 0 ) ;
                break;
                case pbapc_browsing_list:
                    MessageSend ( &theSink.task , PBAPC_APP_PULL_VCARD_LIST, 0 ) ;
                break;
                case pbapc_action_idle:
                case pbapc_downloading:
                case pbapc_setting_phonebook:
                    /* Set the link policy based on the HFP or A2DP state */
                    linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));
                default:
                break;
            }
        break;

        case pbapc_spb_unauthorised:
            /* access to this phonebook denied by PBAP server */
            LOGD("PBAP access to phonebook unauthorised\n");
        default:
            /* other error */
            LOGD("PBAP failed to set phonebook\n");
            if(theSink.pbapc_data.pbap_command == pbapc_dialling)
            {
                MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
                theSink.pbapc_data.pbap_command = pbapc_action_idle;
            }
            /* Set the link policy based on the HFP or A2DP state */
            linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));
        break;
    }
}

static void handleAuthResponseCfm(PBAPC_AUTH_RESPONSE_CFM_T *pMsg)
{
    LOGD("PBAPC_AUTH_RESPONSE_CFM");
    /* Client is currently not bothered about authenticating the server */
    return;
}

static void handleAuthRequestInd(PBAPC_AUTH_REQUEST_IND_T *pMsg)
{
    u8 digest[PBAPC_OBEX_SIZE_DIGEST];
    PRINT(("PBAPC_AUTH_REQUEST_IND. options = %d\n", pMsg->options ));

    {
        MD5_CTX context;
        /* Digest blocks */
        MD5Init(&context);
        MD5Update(&context, pMsg->nonce, strlen((char *)pMsg->nonce));
        MD5Update(&context, (u8 *)":",1);
        MD5Update(&context, (u8 *)"8888",4);
        MD5Final(digest,&context);
    }

    /* Client is not bothered about authenticating the Server, So just
       echo back the nonce. If the client wants to authenticate , it must
       send its own nonce and authenticate the server on receiving
       PBAPC_AUTH_RESPONSE_CFM message.
    */
    PbapcConnectAuthResponse(pMsg->device_id, &digest[0], 0, NULL, NULL);
}

static void handlePullVCardEntryCfm(PBAPC_PULL_VCARD_ENTRY_CFM_T *pMsg)
{
    const u8 *lSource = SourceMap(pMsg->src);

	LOGD("PBAPC_PULL_VCARD_ENTRY_CFM, source:[%x], size:[%d]\n", (u16)lSource, pMsg->dataLen);

    handleVcardPhoneBookMessage(pMsg->device_id, pMsg->status, lSource, pMsg->dataLen);

    if(pMsg->status != pbapc_pending && theSink.pbapc_data.pbap_command == pbapc_dialling)
    {
        MessageSend ( &theSink.task , EventSysPbapDialFail , 0 ) ;
    }

    if(pMsg->status != pbapc_pending)
    {
        /* Reset the flag */
        theSink.pbapc_data.pbap_command = pbapc_action_idle;
    }
}

static void handlePullVCardListCfm(PBAPC_PULL_VCARD_LISTING_CFM_T *pMsg)
{

#ifdef DEBUG_PBAP
    {
        const u8 *lSource = SourceMap(pMsg->src);
        u16 i;
        LOGD("PBAPC_PULL_VCARD_LIST_CFM, source:[%x], size:[%d]\n", (u16)lSource, pMsg->dataLen);
        LOGD("The pb data is: ");

        if (lSource == NULL)
        {
            LOGD("NULL");
        }
        else
        {
            for(i = 0; i < pMsg->dataLen; i++)
                LOGD("%c", *(lSource + i));
        }
        LOGD("\n");
    }
#endif

      /* Read more data for pbap dial fail or other pbap features */
    if (pMsg->status == pbapc_pending)
    {
        LOGD("    Requesting next Packet\n");
        PbapcPullContinue(pMsg->device_id);
    }
    else
    {
        LOGD("    Requesting complete.\n");
        /* Send Complete to Server */
        PbapcPullComplete(pMsg->device_id);

        /* Set the link policy based on the HFP or A2DP state */
        linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

        theSink.pbapc_data.pbap_command = pbapc_action_idle;
    }
}

static void handlePullPhonebookCfm(PBAPC_PULL_PHONEBOOK_CFM_T *pMsg)
{

#ifdef DEBUG_PBAP
    {
        const u8 *lSource = SourceMap(pMsg->src);
        u16 i;

        LOGD("PBAPC_PULL_PHONEBOOK_CFM, source:[%x], pbsize:[%d], datalen:[%d]\n", (u16)lSource,  pMsg->pbookSize, pMsg->dataLen);

        LOGD("The pb data is: ");

        if (lSource == NULL)
        {
            LOGD("NULL");
        }
        else
        {
            for(i = 0; i < pMsg->dataLen; i++)
                LOGD("%c", *(lSource + i));
        }
        LOGD("\n");
    }
#endif

      /* Read more data for pbap dial fail or other pbap features */
    if (pMsg->status == pbapc_pending)
    {
        LOGD("    Requesting next Packet\n");
        PbapcPullContinue(pMsg->device_id);
    }
    else
    {
        LOGD("    Requesting complete.\n");
        /* Send Complete to Server */
        PbapcPullComplete(pMsg->device_id);

        /* Set the link policy based on the HFP or A2DP state */
        linkPolicyPhonebookAccessComplete(PbapcGetSink(theSink.pbapc_data.pbap_active_link));

        theSink.pbapc_data.pbap_command = pbapc_action_idle;
    }
}

/****************************************************************************
    Internal PBAP App Message
****************************************************************************/
static void handleAppPullVcardEntry(void)
{
    LOGD("PBAPC_APP_PULL_VCARD_ENTRY %d\n", theSink.pbapc_data.PbapBrowseEntryIndex);
    if(theSink.pbapc_data.pbap_active_link != pbapc_invalid_link)
    {
        PbapcPullvCardEntryParams *pParams = (PbapcPullvCardEntryParams *)mallocPanic(sizeof(PbapcPullvCardEntryParams));

        if(pParams)
        {
            memset(pParams, 0, sizeof(PbapcPullvCardEntryParams));

            pParams->filter.filterHigh = PBAPC_FILTER_HIGH;
            pParams->filter.filterLow  = PBAPC_FILTER_LOW;
            pParams->format = pbap_format_21;

            PbapcPullVcardEntryRequest(theSink.pbapc_data.pbap_active_link, (u32)(theSink.pbapc_data.PbapBrowseEntryIndex), pParams );

        	LOGD("PBAPC_APP_PULL_VCARD_ENTRY free %x %x\n",(u16)pParams,(u16)&pParams);

            freePanic(pParams);
        }
    }
    else
    {
    	LOGD(" Pbap in incorrect state\n");
    }
}

static void handleAppPullVcardList(void)
{
    LOGD("PBAPC_APP_PULL_VCARD_LIST, ");
    if(theSink.pbapc_data.pbap_active_link != pbapc_invalid_link)
    {
        PbapcPullvCardListParams *pParams = (PbapcPullvCardListParams *)mallocPanic(sizeof(PbapcPullvCardListParams));
        if(pParams)
        {
            memset(pParams, 0, sizeof(PbapcPullvCardListParams));

            pParams->order    = pbap_order_idx;
            pParams->srchAttr = theSink.pbapc_data.pbap_srch_attr;
            pParams->srchVal  = theSink.pbapc_data.pbap_srch_val;
            pParams->srchValLen = (pParams->srchAttr == pbap_search_name) ? (strlen((char *)(pParams->srchVal)) + 1) : 0;
            pParams->maxList    = PBAPC_MAX_LIST;
            pParams->listStart  = PBAPC_LIST_START;

	   	    PbapcPullVcardListingRequest( theSink.pbapc_data.pbap_active_link, pbap_root, pParams );

            freePanic(pParams);
        }
    }
    else
	{
	    LOGD("    Pbap in incorrect state\n");
	}
}

static void handleAppPullPhoneBook(void)
{
    LOGD("PBAPC_APP_PULL_PHONE_BOOK, ");
    if(theSink.pbapc_data.pbap_active_link != pbapc_invalid_link)
    {
        PbapcPullPhonebookParams *pParams = (PbapcPullPhonebookParams *)mallocPanic(sizeof(PbapcPullPhonebookParams));

        if(pParams)
        {
            memset(pParams, 0, sizeof(PbapcPullPhonebookParams));

            pParams->filter.filterHigh = PBAPC_FILTER_HIGH;
            pParams->filter.filterLow  = PBAPC_FILTER_LOW;
            pParams->format    = pbap_format_21;
            pParams->maxList   = PBAPC_MAX_LIST;
            pParams->listStart = PBAPC_LIST_START;

            if(theSink.pbapc_data.pbap_command != pbapc_dialling)
            {
                theSink.pbapc_data.pbap_active_pb = pbap_pb;
            }

            PbapcPullPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb, pParams);

            freePanic(pParams);
        }
	}
	else
	{
	    LOGD("    Pbap in incorrect state\n");
    }

}

static void handleAppPhoneBookSize(void)
{
    LOGD("PBAPC_APP_PHONE_BOOK_SIZE, ");
    if(theSink.pbapc_data.pbap_active_link != pbapc_invalid_link)
    {
        PbapcPullPhonebookParams *pParams = (PbapcPullPhonebookParams *)mallocPanic(sizeof(PbapcPullPhonebookParams));

        if(pParams)
        {
            memset(pParams, 0, sizeof(PbapcPullPhonebookParams));

            pParams->filter.filterHigh = PBAPC_FILTER_HIGH;
            pParams->filter.filterLow  = PBAPC_FILTER_LOW;
            pParams->format    = pbap_format_21;
            pParams->maxList   = 0;
            pParams->listStart = 0;

            if(theSink.pbapc_data.pbap_command != pbapc_dialling)
            {
                theSink.pbapc_data.pbap_active_pb = pbap_pb;
            }

            PbapcPullPhonebookRequest(theSink.pbapc_data.pbap_active_link, theSink.pbapc_data.pbap_phone_repository, theSink.pbapc_data.pbap_active_pb, pParams);

            freePanic(pParams);
        }
	}
	else
	{
	    LOGD("    Pbap in incorrect state\n");
    }

}

/****************************************************************************
NAME
	VcardGetFirstTel

DESCRIPTION
    Find the first telephone number from the supplied VCARD data

PARAMS
    pVcard   pointer to supplied VCARD data
    pTel     pointer to section of pVcard where the telephone number begins

RETURNS
	u8    length of the found telephone number, 0 if not found
*/
static u8 *memstr( const u8 *buffer, const u16 buffer_size, const u8 *str, const u16 count )
{

    u8 *p = (u8 *)memchr(buffer, str[0], buffer_size);
    LOGD("PBAP memstr\n");

    while (p && p < buffer + buffer_size)
    {
        if(memcmp((char *)p, (char *)str, count) == 0)
        {
            return p;
        }
        p += 1;
        p = (u8 *)memchr(p, str[0], (u16)(buffer+buffer_size - p));
    }

    return 0;
}

static u16 VcardFindMetaData( const u8 *start, const u8 *end, u8 **metaData, const char *str, const u16 count)
{
    u16 len;
    u8 *p         = (u8 *)start;
    u8 *endstring = NULL;

    LOGD("PBAP VcardFindMetaData\n");

    /* find the MetaData */
    len         = (u16)(end - p);

    if((((*metaData) = (u8 *)memstr(p, len, (u8 *)str, strlen(str))) != NULL) &&
       (((*metaData) = (u8 *)memchr((u8 *)(*metaData), ':',  end - (*metaData))) != NULL))
    {
        (*metaData) += 1;
        endstring    = (u8 *)memchr((u8 *)(*metaData), '\n', end - (*metaData)) - 1;
    }
    else
    {
        /* There are some errors about the format of phonebook. */
        return 0;
    }

    return(endstring - (*metaData));
}

static u8 VcardGetFirstTel(const u8* pVcard, const u16 vcardLen, pbapMetaData **pMetaData)
{
    u16 len    = 0;
    u16 telLen, nameLen = 0;
    u8 *pTel   = NULL;
    u8 *pName  = NULL;

    /* Find the start and end position of the first Vcard Entry */
    u8 *start  = memstr(pVcard, vcardLen, (u8 *)gpbapbegin, strlen(gpbapbegin));
    u8 *end    = memstr(pVcard, vcardLen, (u8 *)gpbapend,   strlen(gpbapend));
    end           = (end == NULL) ? (u8 *)(pVcard + vcardLen - 1) : end;

#ifdef DEBUG_PBAP
    {
        u16 i;
        LOGD("The pVcard is: ");

        for(i = 0; i < vcardLen; i++)
            LOGD("%c", *(pVcard + i));

        LOGD("\n");
    }
#endif

    LOGD("First entry start:[%x], end:[%x]\n", (u16)start, (u16)end);

    while(start && start < end)
    {
        start = start + strlen(gpbapbegin);

        /* find the Tel */
        telLen = VcardFindMetaData(start, end, &pTel, gpbaptel, strlen(gpbaptel));

        if( telLen )
        {
            LOGD("VcardGetFirstTel:telephone number found ok\n");

            /* find the Name */
            nameLen = VcardFindMetaData(start, end, &pName, gpbapname, strlen(gpbapname));

            /* allocate the memory for pMetaData structure */
            *pMetaData = (pbapMetaData *)mallocPanic(sizeof(pbapMetaData) + nameLen);

            if(pMetaData)
            {
                (*pMetaData)->pTel    = pTel;
                (*pMetaData)->telLen  = telLen;
                (*pMetaData)->nameLen = nameLen;

                LOGD("CallerID pos:[%x], len:[%d]\n", (u16)pName, nameLen);

                if(nameLen)
                {
                    /* This memory should be freed after pbap dial command or Audio Prompt has completed */
                    memmove(&((*pMetaData)->pName), pName, nameLen);
                    (*pMetaData)->pName[nameLen] = '\0';

                    /* Remove the ';' between names */
                    /* Based on PBAP spec., the name format is:
                      LastName;FirstName;MiddleName;Prefix;Suffix
                    */
                    len = nameLen;
                    pName = (*pMetaData)->pName;
                    while(pName < (*pMetaData)->pName + nameLen)
                    {
                        pName    = (u8 *)memchr(pName, ';', len) ;
                        /*if no ; is found exit */
                        if(!pName)
                            break;
                        *pName++ = ' ';
                        /* determine how many characters are left */
                        len  = nameLen - (pName - (*pMetaData)->pName);
                    }

                    LOGD("VcardGetFirstTel:CallerID found ok\n");
                }

                return(telLen);
            }
            else
            {
                LOGD("VcardGetFirstTel:No memory slot to store MetaData\n");
                return 0;
            }
        }

        /* If the first Vcard Entry the Tel is enmty, try next Entry. */
        /* First find the next Vcard Entry start and end positions    */
        end = end + strlen(gpbapend);
        len = (u16)(pVcard + vcardLen - end);
        start = memstr(end, len , (u8 *)gpbapbegin, strlen(gpbapbegin));
        end   = memstr(end, len,  (u8 *)gpbapend,   strlen(gpbapend));
        end   = (end == NULL) ? (u8 *)(pVcard + vcardLen - 1) : end;

        LOGD("next start:[%x], end:[%x]\n", (u16)start, (u16)end);
    }

    LOGD("VcardGetFirstTel:telephone number not found\n");

    return 0;
}

static bool handlePbapDialData(const u8 *pVcard, const u16 vcardLen)
{
    pbapMetaData *pMetaData = NULL;
    bool success = FALSE;

    LOGD("handlePbapDial:The length of data is [%d]\n", vcardLen);

  	/* Process Data to find telephone number*/
    if(VcardGetFirstTel(pVcard, vcardLen, &pMetaData))
    {
        LOGD("handlePbapDial:dialling from PBAP Phonebook\n");

        /* Display the name of tel of pbap dial entry.*/
        /* Audio Prompts can be used to play the caller ID */
#ifdef DEBUG_PBAP
        {
            u8 i = 0;
            LOGD("The Name is: ");
            for(i = 0; i < pMetaData->nameLen; i++)
                LOGD("%c ", *(pMetaData->pName + i));

            LOGD("\nThe Tel is: ");
            for(i = 0; i < pMetaData->telLen; i++)
                LOGD("%c ", *(pMetaData->pTel + i));
            LOGD("\n");
        }
#endif

#ifdef ENABLE_DISPLAY
        displayShowText((char*)pMetaData->pName,  pMetaData->nameLen, 1, DISPLAY_TEXT_SCROLL_SCROLL, 1000, 2000, FALSE, 0);
#endif

        HfpDialNumberRequest(hfp_primary_link, pMetaData->telLen, (u8 *)(pMetaData->pTel));

        /* Task of Pbapc profile has completed and Hfp profile starts to work */
        theSink.pbapc_data.pbap_command = pbapc_action_idle;

        success = TRUE;

    }
    else
    {
         /* error, number not found */
         LOGD("handlePbapDial:no number found to dial.\n");
    }

    if(pMetaData)
    {
        freePanic(pMetaData);
    }

    return(success);
}

static void handlePbapRetrievedData(const u8 *pVcard, const u16 vcardLen)
{
    /* Display the content of the retrieved data.*/
    #ifdef DEBUG_PBAP
    {
        u8 i = 0;
        for(i = 0; i < vcardLen; i++)
            LOGD("%c ", *(pVcard + i));

        LOGD("\n");
    }
    #endif
}

static void handleVcardPhoneBookMessage(u16 device_id, pbapc_lib_status status, const u8 *lSource, const u16 dataLen)
{
    LOGD("PBAP vcardPhoneBookMessage\n");

    if(theSink.pbapc_data.pbap_command == pbapc_dialling)
    {
	    /* Process the data and dial the number */
        if(handlePbapDialData(lSource, dataLen))
        {
            PbapcPullComplete(device_id);
            return;
        }
    }
    else
    {
        /* Other pbap features, for example, downloading or browsing phonebook object. */
        /* As no external memory avaible, just display the data: Name, Tel */
        handlePbapRetrievedData(lSource, dataLen);
    }

    /* Read more data for pbap dial fail or other pbap features */
    if (status == pbapc_pending)
    {
        LOGD("    Requesting next Packet\n");
        PbapcPullContinue(device_id);
    }
    else
    {
        LOGD("    Requesting complete.\n");
	    /* Send Complete to Server */
        PbapcPullComplete(device_id);
    }
}

#endif /*ENABLE_PBAP*/
