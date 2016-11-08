/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_IMM_ALERT_SERVER_HANDLER_H_
#define GATT_IMM_ALERT_SERVER_HANDLER_H_

#include <csrtypes.h>
#include <message.h>


/***************************************************************************
NAME
    imm_alert_server_ext_msg_handler

DESCRIPTION
    Handler for external messages sent to the library.
*/
void imm_alert_server_ext_msg_handler(Task task, MessageId id, Message msg);


#endif /*GATT_IMM_ALERT_SERVER_HANDLER_H_*/

