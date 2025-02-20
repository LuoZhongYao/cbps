/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    avrcp_common.c

DESCRIPTION
    This file defines all common utility functions used through out the library.

NOTES

*/


/****************************************************************************
    Header files
*/
#include <stream.h>
#include <sink.h>
#include <source.h>
#include <panic.h>
#include <stdlib.h>
#include <memory.h>

#include "avrcp_private.h"
#include "avrcp_common.h"

#ifndef AVRCP_TG_ONLY_LIB /* Disable CT for TG only lib */
/****************************************************************************
* NAME    
*  avrcpUpdateTransactionLabel
*
* DESCRIPTION
*  update to an unique transaction label
***************************************************************************/
static void avrcpUpdateTransactionLabel( AVRCP* avrcp )
{
    u8 id;
    u8 events =  avrcp->registered_events;
    u8 tl = AVRCP_NEXT_TRANSACTION(avrcp->cmd_transaction_label); 
                                    
    for(id=0; (id < AVRCP_MAX_NUM_EVENTS) && events ; id++, events>>=1)
    {
        if( ( events & 0x01 ) &&
            ( avrcp->notify_transaction_label[id] == tl ) )
        {
            tl = AVRCP_NEXT_TRANSACTION( tl );
        }
    } 

    avrcp->cmd_transaction_label= tl;
}


/****************************************************************************
* NAME    
*   avrcpGetNextTransactionLabel    
*
* DESCRIPTION
*  Get the transaction label and update it to the next unique label.
***************************************************************************/
u8 avrcpGetNextTransactionLabel(AVRCP *avrcp)
{
   u8 tl = avrcp->cmd_transaction_label;

   /* Post update the transaction label to an unique label */
   avrcpUpdateTransactionLabel( avrcp ); 
   return tl;
}

/****************************************************************************
* NAME    
*   convertResponseToStatus    
*
* DESCRIPTION
*  Map the received response code to a corresponding Status value.
*
*******************************************************************************/
avrcp_status_code convertResponseToStatus(avrcp_response_type resp)
{
    /* Turn a response into a simplified state that the AVRCP library
     can return to the application. */
    switch (resp)
    {
    case avctp_response_accepted:
    case avctp_response_in_transition:
    case avctp_response_stable:
    case avctp_response_changed:
        return avrcp_success;
    case avctp_response_not_implemented:
    case avctp_response_bad_profile:
        return avrcp_unsupported;
    case avctp_response_interim:
        return avrcp_interim_success;
    case avctp_response_rejected:
        return avrcp_rejected;
    default:
        return avrcp_fail;
    }    
}

/****************************************************************************
*NAME    
*   avrcpSendCommonStatusCfm
*
*DESCRIPTION
*    Send a Common Status Message
*
*RETURNS
*    void
******************************************************************************/
void avrcpSendCommonStatusCfm(AVRCP*            avrcp,
                              avrcp_status_code status,
                              u16            message_id)
{
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_STATUS_CFM);
    message->status = status;
    message->avrcp = avrcp;
    MessageSend(avrcp->clientTask, message_id, message);
}


/****************************************************************************
* NAME    
* avrcpSendCommonFragmentedMetadataCfm    
*
* DESCRIPTION
*   Send MetaData confirm to the CT application with the extracted data from 
*   the response
*
* RETURNS
*    void
*******************************************************************************/
void avrcpSendCommonFragmentedMetadataCfm(AVRCP         *avrcp, 
                                          avrcp_status_code  status,
                                          u16         id, 
                                          u16         metadata_packet_type,
                                          u16         data_length, 
                                          const u8*   data)
{
    u16 offset=0;
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_FRAGMENTED_METADATA_CFM);
    
    message->avrcp = avrcp;
    message->status = status;

#ifdef AVRCP_ENABLE_DEPRECATED
    if(avrcp->av_msg)
    {
        message->transaction = (avrcp->av_msg[AVCTP_HEADER_START_OFFSET] >> 
                                AVCTP0_TRANSACTION_SHIFT);
    }
    else
    {
        message->transaction =0;
    }

    message->no_packets = 1;
    message->ctp_packet_type = AVCTP0_PACKET_TYPE_SINGLE;
    message->data_offset = 0;
#endif

    message->metadata_packet_type = metadata_packet_type;
    message->number_of_data_items = 0;
    message->data=0;
    message->size_data=0;

    if(!(status & AVRCP_ERROR_STATUS_BASE) && 
        (data_length))
    {
        if ((metadata_packet_type == avrcp_packet_type_single) || 
            (metadata_packet_type == avrcp_packet_type_start))
        {
            /* Extract the first 2 bytes which is capability ID
               and capability length */
            offset=1;
            message->number_of_data_items = data[0];
        }

        if(data_length > offset)
        {        
            message->size_data = data_length-offset;
            message->data = avrcpSourceFromConstData(avrcp, 
                                                 data+offset, 
                                                 message->size_data);
        }
    }

    MessageSend(avrcp->clientTask, id, message);
    return;
}

#ifdef AVRCP_ENABLE_DEPRECATED
/****************************************************************************
* NAME    
* avrcpSendCommonMetadataCfm    
*
* DESCRIPTION
* This is a deprecated function and will be replaced by 
* avrcpSendCommonStatusCfm
* 
* PARAMETERS
*    *response       - Application Response , standardised by this function
*   command_type    - Should be one of the values AVRCP0_CTYPE_CONTROL, 
*                        AVRCP0_CTYPE_STATUS or AVRCP0_CTYPE_NOTIFY
*
* RETURNS
*    void
*******************************************************************************/
void avrcpSendCommonMetadataCfm(AVRCP *avrcp, 
                                avrcp_status_code status, 
                                u16 id)
{
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_METADATA_CFM_MESSAGE);

    message->avrcp = avrcp;
    message->status = status;

    if(avrcp->av_msg)
    {
        message->transaction = (avrcp->av_msg[AVCTP_HEADER_START_OFFSET] >> 
                                AVCTP0_TRANSACTION_SHIFT);
    }
    
    MessageSend(avrcp->clientTask, id, message);
}
#endif /* AVRCP_ENABLE_DEPRECATED */

#endif /* !AVRCP_TG_ONLY_LIB */

#ifndef AVRCP_CT_ONLY_LIB /* Disable TG for CT only lib */


/****************************************************************************
* NAME    
*    avrcpGetErrorStatusCode
*
* DESCRIPTION
*   Responses to AVRCP Specific Browsing Commands and AVRCP Specific 
*   AV/C Commands. Contain 1 byte Error Status Code. This function extracts
*   the Error Status code from the application response. Append the error
*   code to the pdu if required.
*
* PARAMETERS
*   response       - Application Response , standardised by this function
*   command_type   - Should be one of the values AVRCP0_CTYPE_CONTROL, 
*                        AVRCP0_CTYPE_STATUS or AVRCP0_CTYPE_NOTIFY
*
* RETURNS
*    void
********************************************************************************/
u8 avrcpGetErrorStatusCode(avrcp_response_type *response, u8 command_type)
{
    u8 error_code = AVRCP_STATUS_SUCCESS;

    /* if response value is any specific rejected error response values,
       convert the response to avrcp_response_rejected after extracting the 
       error status  code from  the response value, otherwise standardize the
       response.*/

    if(*response < avrcp_response_rejected_invalid_pdu)
    {
        switch(*response)
        {
        case avctp_response_accepted:
        case avctp_response_changed: /* fall through */
        case avctp_response_stable: /* fall through */
            
            if(command_type == AVRCP0_CTYPE_STATUS)
            {
                *response = avctp_response_stable;
            }
            else if(command_type == AVRCP0_CTYPE_CONTROL)
            {
                *response = avctp_response_accepted;
            }
            else
            {
                *response = avctp_response_changed;
            }
            break;
        
        case avctp_response_interim:
            /* accept */
            break;
        case avctp_response_rejected:
            error_code = AVRCP_STATUS_INTERNAL_ERROR;
            break;
        default:
            /* ignore */
            break;            
        }
    }
    else
    {
        /* return correspondent status code */
        error_code = *response & AVRCP_RESPONSE_ERROR_STATUS_MASK;
        *response = avctp_response_rejected;
    }

    return error_code;

}


/****************************************************************************
* NAME    
* avrcpSendCommonMetadataInd    
*
* DESCRIPTION
*   Send MetaData Indication to the TG application if there was no data 
*   received part of Metadata request message.
*
* RETURNS
*    void
*******************************************************************************/
void avrcpSendCommonMetadataInd(AVRCP *avrcp, u16 id)
{
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_METADATA_IND_MESSAGE);

    message->avrcp = avrcp;
#ifdef AVRCP_ENABLE_DEPRECATED
    message->transaction = avrcp->rsp_transaction_label;
#endif

    MessageSend(avrcp->clientTask, id, message);
}

/****************************************************************************
* NAME    
* avrcpSendCommonFragmentedMetadataInd    
*
* DESCRIPTION
*   Send MetaData Indication to the TG application if there was data 
*   received part of Metadata request message.
*
* RETURNS
*    void
*******************************************************************************/
void avrcpSendCommonFragmentedMetadataInd(  AVRCP     *avrcp, 
                                            u16     id, 
                                            u16     number_of_data_items, 
                                            u16     data_length, 
                                            Source     source)
{
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_FRAGMENTED_METADATA_IND);

    message->avrcp = avrcp;

#ifdef AVRCP_ENABLE_DEPRECATED
    message->transaction = avrcp->rsp_transaction_label;
    message->no_packets = 1; /* Deprecated */
    message->ctp_packet_type = AVCTP0_PACKET_TYPE_SINGLE;
    message->data_offset = 0; /* Deprecated */
    message->metadata_packet_type = avrcp_packet_type_single;
#endif

    message->number_of_data_items = number_of_data_items;
    message->size_data = data_length;
    message->data = source;

    MessageSend(avrcp->clientTask, id, message);
}

/****************************************************************************
*NAME    
*    AvrcpSetMetadataResponsePDUDataSize   
*
*DESCRIPTION
*    API function to restrict metadata response PDU size from AVRCP_AVC_MAX_DATA_SIZE 
*    to data_size.
*
*RETURNS
* status
******************************************************************************/
bool AvrcpSetMetadataResponsePDUDataSize(AVRCP *avrcp, 
                                     u16 data_size)
{
    /* data_size should less than or equal to the max AVRCP_AVC_MAX_DATA_SIZE. 
    * If it is more than max AVRCP_AVC_MAX_DATA_SIZE then library will return FALSE
    */
    if(avrcp->av_max_data_size <= AVRCP_AVC_MAX_DATA_SIZE)
    {
       avrcp->av_max_data_size = data_size;
       return TRUE;       
    }
    
    return FALSE;
}

#endif /* !AVRCP_CT_ONLY_LIB*/

/****************************************************************************
*NAME    
*    avrcpSendCommonCfmMessageToApp
*
*DESCRIPTION
*    Create a common CFM message (many messages sent to the app
*    have the form of the message below and a common function can be used to
*    allocate them). Send the message not forgetting to set the correct 
*    message id.
*
******************************************************************************/
void avrcpSendCommonCfmMessageToApp(u16              message_id,
                                    avrcp_status_code   status, 
                                    Sink                sink, 
                                    AVRCP               *avrcp)
{
    MAKE_AVRCP_MESSAGE(AVRCP_COMMON_CFM_MESSAGE);
    message->status = status;
    message->sink = sink;
    message->avrcp = avrcp;
    MessageSend(avrcp->clientTask, message_id, message);
}

/****************************************************************************
* NAME    
*   avrcpGetCompanyId    
*
* DESCRIPTION
*  Utility function to extract the 3 byte company ID from the data packet.
*
*PARAMETRS:
*   ptr     -   Data packet
*   offset  -   Offset to start of company ID in the data packet.
*******************************************************************************/
u32 avrcpGetCompanyId(const u8 *ptr, u16 offset)
{
    u32 cid = 0;
    u32 cid_tmp = 0;

    cid_tmp = (u32) ptr[offset];
    cid = (cid_tmp << 16);
    cid_tmp = (u32) ptr[offset+1];
    cid |= (cid_tmp << 8);
    cid_tmp = (u32) ptr[offset+2];
    cid |= cid_tmp;

    return cid;
}

/****************************************************************************
* NAME    
*   avrcpGrabSink    
*
* DESCRIPTION
*  Grab Sink space  to send AVCTP data packet
*
*PARAMETRS:
*   Sink     -   L2CAP sink
*   size     -   Requested Sink Space to reserve.
*******************************************************************************/
u8 *avrcpGrabSink(Sink sink, u16 size)
{
    u8 *dest = SinkMap(sink);
    u16 claim_result = SinkClaim(sink, size);
    if (claim_result == 0xffff)
    {
        AVRCP_DEBUG(("SinkClaim return Invalid Offset"));
        return NULL;
    }

    return (dest + claim_result);
}

/**************************************************************************
 * NAME
 *  avrcpSourceDrop
 *
 * DESCRIPTION 
 *  Drop the AVRCP Source 
 *************************************************************************/
void avrcpSourceDrop(AVRCP* avrcp)
{
    Source source = StreamSourceFromSink(avrcp->sink);

    if(avrcp->av_msg_len)   
    {
        u16 data_drop = avrcp->av_msg_len;

        if( SourceMap(source) != avrcp->av_msg ) 
        {
            if( avrcp->fragment == avrcp_packet_type_single)
            {
                data_drop = 0;
                free(avrcp->av_msg);
                avrcp->av_msg= NULL;
                avrcp->av_msg_len = 0; 
                avrcp->fragment = avrcp_packet_type_single;
            }
            else
            {
                data_drop -= AVRCP_AVC_START_HEADER_SIZE;
                avrcp->av_msg_len = AVRCP_AVC_START_HEADER_SIZE;
            }
        }
        else
        {
            avrcp->av_msg= NULL;
            avrcp->av_msg_len = 0;
        }

        if( data_drop )
        {
            /* Data is from the Source. Flush it */
            SourceDrop(source, data_drop );
            if(source == avrcp->data_app_ind) avrcp->data_app_ind = 0;
        }
    }
    else
    {
        avrcp->av_msg = NULL;
    }
}


/****************************************************************************
* NAME    
*   avrcpSourceProcessed    
*
* DESCRIPTION
*  Free all memory resources. 
*
*  PARAMETERS
*  avrcp  - AVRCP instance
*  intern - TRUE - Call initiated by internal function. Free all resources.
*           FALSE- Call Initiated by app. Free only the external resource if
*                  other shared resources are not yet processed by the library.
*
*******************************************************************************/
void avrcpSourceProcessed(AVRCP *avrcp, bool intern)
{
    if(avrcp->fragment == avrcp_packet_type_end )
    {
        avrcp->fragment = avrcp_packet_type_single;
    }

    if(!avrcp->block_received_data)
    {
        Source source = StreamSourceFromSink(avrcp->sink);

        avrcpSourceDrop(avrcp);

        if(SourceBoundary(source))
        {
            /* There is data in the L2CAP sink to be processed.
                 Message Internally to trigger*/
            MessageSend(&avrcp->task, AVRCP_INTERNAL_MESSAGE_MORE_DATA, 0);
        }

        if(avrcp->data_app_ind && (!intern))
        {
            MessageSinkTask(StreamSinkFromSource(avrcp->data_app_ind), 0);
            SourceEmpty(avrcp->data_app_ind);
            avrcp->data_app_ind = 0;
            if(avrcp->dataFreeTask.sent_data)
            {
                /* Source empty will trigger message MESSAGE_SOURCE_EMPTY
                   and free sent_data in the CleanUpTask. But this may 
                   create a race as well. So don't wait for that message 
                   and free the sent_data here for the application data */
                free(avrcp->dataFreeTask.sent_data);
                avrcp->dataFreeTask.sent_data = 0;
            }
        }
    }
}


/****************************************************************************
* NAME    
*    convertUint8ValuesToUint32
*
* DESCRIPTION
*   Utility functions to convert 4 data octets to a UINT32 value.
*****************************************************************************/
u32 convertUint8ValuesToUint32(const u8 *ptr)
{
    return ((((u32)ptr[0] << 24) & 0xFF000000) | 
            (((u32)ptr[1] << 16) & 0x00FF0000) | 
            (((u32)ptr[2] << 8) & 0x0000FF00)  | 
            ((u32)ptr[3] & 0x000000FF));
}

/****************************************************************************
* NAME    
*   convertUint32ToUint8Values
*
* DESCRIPTION
*   Utility functions to convert 32 bit value to 4 data octets.
*****************************************************************************/
void convertUint32ToUint8Values(u8 *ptr, u32 value)
{
    ptr[0] = (value >> 24) & 0xFF;
    ptr[1] = (value >> 16) & 0xFF;
    ptr[2] = (value >> 8) & 0xFF;
    ptr[3] = value & 0xFF;
}

/****************************************************************************
* NAME    
* avrcpSourceFromConstData    
*
* DESCRIPTION
*  Create a Source by allocating memory and copying data from constant memory.   
*
* RETURNS
*    void
*******************************************************************************/
Source avrcpSourceFromConstData(AVRCP *avrcp, const u8 *data, u16 length)
{
    u8* ptr = NULL;
    Source src = StreamSourceFromSink(avrcp->sink);
    u16 data_drop = avrcp->av_msg_len;

    /* If application does not free the data yet. Force free */
    if( avrcp->data_app_ind && (src != avrcp->data_app_ind) )
    {
        MessageSinkTask(StreamSinkFromSource(avrcp->data_app_ind), 0);
        if(avrcp->dataFreeTask.sent_data)
        {
            free(avrcp->dataFreeTask.sent_data);
            avrcp->dataFreeTask.sent_data = 0;
        }
        SourceEmpty(avrcp->data_app_ind);
    }

    if(avrcp->fragment == avrcp_packet_type_single)
    {
        /* allocate memory */
        ptr = (u8*)malloc(length);
    }
    else
    {
        data_drop -= AVRCP_AVC_START_HEADER_SIZE;
    }

    if( ptr )
    {
        memmove(ptr, data, length);
        src = avrcpSourceFromData(avrcp, ptr, length);
    }
    else
    {
         if(avrcp->block_received_data== avrcp_none)
         {
            avrcpBlockReceivedData(avrcp, avrcp_app_unknown, 0);
         }
         data_drop -= length;
         SourceDrop(src, data_drop );
         avrcp->av_msg_len -= data_drop;
    }

    avrcp->data_app_ind = src;
    return src;
}

/****************************************************************************
* NAME    
* avrcpSourceFromData    
*
* DESCRIPTION
*  Create a Source from the passed memory location.
*
* RETURNS
*    void
*******************************************************************************/
Source avrcpSourceFromData(AVRCP *avrcp, u8 *data, u16 length)
{
    /* Create a source from the data */
    Source src = StreamRegionSource(data, length);

    /* Register a task for freeing the data and store a ptr to it */
    avrcp->dataFreeTask.sent_data = data;
    MessageSinkTask(StreamSinkFromSource(src), 
                    &avrcp->dataFreeTask.cleanUpTask);

    return src;
}

