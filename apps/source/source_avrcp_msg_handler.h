/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.

PROJECT
    source
    
FILE NAME
    source_avrcp.h

DESCRIPTION
    AVRCP profile message handling.
    
*/


#ifndef _SOURCE_AVRCP_MSG_HANDLER_H_
#define _SOURCE_AVRCP_MSG_HANDLER_H_


/* profile/library headers */
#include <avrcp.h>
/* VM headers */
#include <message.h>


/* Application defined AVRCP messages */
#define AVRCP_INTERNAL_MESSAGE_BASE 0

typedef enum
{
    AVRCP_INTERNAL_CONNECT_REQ = AVRCP_INTERNAL_MESSAGE_BASE, 
    AVRCP_INTERNAL_VENDOR_COMMAND_REQ,
    AVRCP_SOURCE_VENDOR_COMMAND_REQ,

    /* End message limit */
    AVRCP_INTERNAL_MESSAGE_TOP
} AvrcpInternalMessageId;


typedef struct
{    
    avc_subunit_type subunit_type;
    avc_subunit_id subunit_id;
    u8 ctype;
    u32 company_id;
    u16 cmd_id;
    u16 size_data;
    Source data;
} AVRCP_INTERNAL_VENDOR_COMMAND_REQ_T;



typedef struct
{    
    u32 company_id;
    u16 cmd_id;
    u16 size_data;
    u8 data[1];
} AVRCP_SOURCE_VENDOR_COMMAND_REQ_T;


/***************************************************************************
Function definitions
****************************************************************************
*/


/****************************************************************************
NAME    
    avrcp_msg_handler

DESCRIPTION
    Handles AVRCP messages.

*/
void avrcp_msg_handler(Task task, MessageId id, Message message);


#endif /* _SOURCE_AVRCP_MSG_HANDLER_H_ */
