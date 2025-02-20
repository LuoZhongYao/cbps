/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_server_api.h
    
DESCRIPTION
    Contains routines responsible for building & sending external messages.
*/

#ifndef GATT_SERVER_API_H_
#define GATT_SERVER_API_H_

/* Firmware headers */
#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

/* GATT Server headers */
#include "gatt_server.h"

/* Macros for creating messages */
#define MAKE_GATT_SERVER_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T)

/*******************************************************************************
NAME
    gattServerSendReadClientConfigInd
    
DESCRIPTION
    Build and send a GATT_SERVER_READ_CLIENT_CONFIG_IND message.
    
PARAMETERS
    task        The task to send the message to.
    cid         The connection ID of the device requesting to read the
                descriptor.
    handle      The Handle being accessed.
RETURNS
    TRUE if the message was sent, FALSE otherwise.
*/
bool gattServerSendReadClientConfigInd(Task task, u16 cid, u16  handle);


/*******************************************************************************
NAME
    gattServerSendWriteClientConfigInd
    
DESCRIPTION
    Build and send a GATT_SERVER_WRITE_CLIENT_CONFIG_IND message.
    
PARAMETERS
    task        The task to send the message to.
    cid         The connection ID of the device requesting to read the
                descriptor.
    value       The value the remote device wants to write to the descriptor.
    
RETURNS
    TRUE if the message was sent, FALSE otherwise.
*/
bool gattServerSendWriteClientConfigInd(Task task, u16 cid, u16 value);


/*******************************************************************************
NAME
    gattServerSendServiceChangedIndicationCfm
    
DESCRIPTION
    Build and send a GATT_SERVER_SERVICE_CHANGED_INDICATION_CFM message.
    
PARAMETERS
    task        The task to send the message to.
    cid         The connection ID of the device requesting to read the
                descriptor.
    status      The GATT Status to add to the message.
    
RETURNS
    TRUE if the message was sent, FALSE otherwise.
*/
bool gattServerSendServiceChangedIndicationCfm(Task task,
                                               u16 cid,
                                               gatt_status_t status);

#endif
