/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    hfp_subscriber_num_handler.c

DESCRIPTION
    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_private.h"
#include "hfp_link_manager.h"
#include "hfp_parse.h"
#include "hfp_current_calls_handler.h"
#include "hfp_call_handler.h"
#include "hfp_common.h"
#include "hfp_send_data.h"

#include <panic.h>
#include <string.h>


/****************************************************************************
NAME    
    hfpHandleCurrentCallsGetReq

DESCRIPTION
    Request current calls list from the AG.

RETURNS
    TRUE if successful (command supported by AG and HF), FALSE otherwise
*/
bool hfpHandleCurrentCallsGetReq(hfp_link_data* link)
{
    /* Only send the cmd if the AG and local device support the enhanced call status feature.*/
    if (hfFeatureEnabled(HFP_ENHANCED_CALL_STATUS) && agFeatureEnabled(link, AG_ENHANCED_CALL_STATUS))
    {
        /* Send the AT cmd over the air */
        char clcc[] = "AT+CLCC\r";
        hfpSendAtCmd(link, strlen(clcc), clcc, hfpClccCmdPending);
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
NAME    
    hfpHandleCurrentCallsInd

DESCRIPTION
    Send current calls info to the app and change state if necessary

AT INDICATION
    +CLCC

RETURNS
    void
*/
static void hfpHandleCurrentCallsInd(hfp_link_data* link, u16 call_idx, u8 direction, u8 status, u8 mode, u8 multiparty, u8 type, u8 number_length, const u8 *number)
{
    /* Validate parameters - ignore silently if any are invalid */
    if(direction > hfp_call_mobile_terminated || status > hfp_call_waiting || mode > hfp_call_fax || multiparty > hfp_multiparty_call)
        return;
    
    /* Silently ignore AT command if HF does not support the enhanced call status */
    if (hfFeatureEnabled(HFP_ENHANCED_CALL_STATUS))
    {
        /* Don't exceed max length */
        if(number_length >= HFP_MAX_ARRAY_LEN)
            number_length = HFP_MAX_ARRAY_LEN - 1;
        
        /* Send this information to the application */
        if(link->at_cmd_resp_pending == hfpClccCmdPending)
        {
            /* Leave space to NULL terminate string */
            MAKE_HFP_MESSAGE_WITH_LEN(HFP_CURRENT_CALLS_IND, number_length + 1);
            message->priority    = hfpGetLinkPriority(link);
            message->call_idx    = call_idx;
            message->direction   = (hfp_call_direction)direction;
            message->status      = (hfp_call_status)status;
            message->mode        = (hfp_call_mode)mode;
            message->multiparty  = (hfp_call_multiparty)multiparty;
            message->number_type = hfpConvertNumberType(type);
            /* Don't include NULL in size */
            message->size_number = number_length;
            /* Copy number into message */
            memmove(message->number, number, number_length);
            /* NULL terminate number */
            message->number[number_length] = '\0';
            MessageSend(theHfp->clientTask, HFP_CURRENT_CALLS_IND, message);
        }
        /* Change state if necessary */
        hfpHandleCallCurrentIndication(link, multiparty);
    }
}


/****************************************************************************
NAME    
    hfpHandleCurrentCalls

DESCRIPTION
    Current calls info indication received from the AG

AT INDICATION
    +CLCC

RETURNS
    void
*/
void hfpHandleCurrentCalls(Task link_ptr, const struct hfpHandleCurrentCalls *ind)
{
    hfpHandleCurrentCallsInd((hfp_link_data*)link_ptr, ind->idx, ind->dir, ind->status, ind->mode, ind->mprty, 0, 0, 0);
}


/****************************************************************************
NAME    
    hfpHandleCurrentCallsWithNumber

DESCRIPTION
    Current calls info indication received from the AG

AT INDICATION
    +CLCC

RETURNS
    void
*/
void hfpHandleCurrentCallsWithNumber(Task link_ptr, const struct hfpHandleCurrentCallsWithNumber *ind)
{
    hfpHandleCurrentCallsInd((hfp_link_data*)link_ptr, ind->idx, ind->dir, ind->status, ind->mode, ind->mprty, ind->type, ind->number.length, ind->number.data);
}
