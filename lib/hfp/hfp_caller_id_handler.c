/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    hfp_caller_id_handler.c

DESCRIPTION
    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_private.h"
#include "hfp_parse.h"
#include "hfp_caller_id_handler.h"
#include "hfp_common.h"
#include "hfp_send_data.h"
#include "hfp_link_manager.h"

#include <panic.h>
#include <string.h>


/*****************************************************************************/
static void sendCallerIdMsgToApp(hfp_link_data* link, u8 type, u16 size_name, const u8* name, u16 size_number, const u8 *number)
{
    if(size_number || size_name)
    {
        /* Don't exceed max length */
        if(size_number >= HFP_MAX_ARRAY_LEN)
            size_number = HFP_MAX_ARRAY_LEN - 1;
            
        if(size_name >= (HFP_MAX_ARRAY_LEN - size_number))
            size_name = (HFP_MAX_ARRAY_LEN - size_number - 1);
        
        /* If the local device does not support CLI presentation just ignore this */
        if(hfFeatureEnabled(HFP_CLI_PRESENTATION))
        {
            MAKE_HFP_MESSAGE_WITH_LEN(HFP_CALLER_ID_IND, size_number + size_name + 2);
            message->priority      = hfpGetLinkPriority(link);
            message->number_type   = hfpConvertNumberType(type);
            /* Set offsets */
            message->offset_number = 0;
            message->offset_name   = size_number + 1;
            /* Don't report NULLs in sizes */
            message->size_number   = size_number;
            message->size_name     = size_name;
            /* Copy data into the message */
            memmove(message->caller_info + message->offset_number, number, size_number);
            memmove(message->caller_info + message->offset_name,   name,   size_name);
            /* NULL terminate strings */
            message->caller_info[message->offset_number + size_number] = '\0';
            message->caller_info[message->offset_name + size_name]     = '\0';
            
            MessageSend(theHfp->clientTask, HFP_CALLER_ID_IND, message);
        }
    }
}


/*****************************************************************************/
bool hfpHandleCallerIdEnableReq(const HFP_INTERNAL_AT_CLIP_REQ_T *req)
{
    /* Make sure the local device supports caller id */
    if(hfFeatureEnabled(HFP_CLI_PRESENTATION))
    {
        char *caller_id;

        if (req->enable)
            caller_id = "AT+CLIP=1\r";
        else
            caller_id = "AT+CLIP=0\r";

        /* Send the AT cmd over the air */
        hfpSendAtCmd(req->link, strlen(caller_id), caller_id, hfpClipCmdPending);
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
AT INDICATION
    +CLIP
*/
void hfpHandleCallerId(Task link_ptr, const struct hfpHandleCallerId *ind)
{
    sendCallerIdMsgToApp((hfp_link_data*)link_ptr, ind->type, 0, NULL, ind->num.length, ind->num.data);
}


/****************************************************************************
AT INDICATION
    +CLIP
*/
void hfpHandleCallerIdIllegal(Task link_ptr, const struct hfpHandleCallerIdIllegal *ind)
{
    sendCallerIdMsgToApp((hfp_link_data*)link_ptr, 0, 0, NULL, ind->num.length, ind->num.data);
}


/****************************************************************************
AT INDICATION
    +CLIP
*/
void hfpHandleCallerIdWithName(Task link_ptr, const struct hfpHandleCallerIdWithName *ind)
{
    sendCallerIdMsgToApp((hfp_link_data*)link_ptr, ind->type, ind->name.length, ind->name.data, ind->num.length, ind->num.data);
}
