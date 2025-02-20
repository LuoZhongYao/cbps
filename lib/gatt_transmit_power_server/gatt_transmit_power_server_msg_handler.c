/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#include <message.h>
#include <stdio.h>
#include <stdlib.h>
#include <vm.h>
#include <gatt.h>
#include <connection_no_ble.h>

#include "gatt_transmit_power_server_msg_handler.h"
#include "gatt_transmit_power_server_private.h"
#include "gatt_transmit_power_server_db.h"
#include "gatt_transmit_power_server.h"
#include "gatt_transmit_power_server_debug.h"

/***************************************************************************
NAME
    send_transmit_power_access_rsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
static void send_transmit_power_access_rsp(Task task,
                                          u16 cid,
                                          u16 handle,
                                          u16 result,
                                          u16 size_value,
                                          const u8 *value)
{
    if (!GattManagerServerAccessResponse(task, cid, handle, result, size_value, value))
    {
        /* The GATT Manager should always know how to send this response */
        GATT_TRANSMIT_POWER_DEBUG_PANIC(("Couldn't send GATT access response\n"));
    }
}

/***************************************************************************
NAME
    send_transmit_power_level_access_rsp

DESCRIPTION
    Send an access response to the GATT Manager library.
*/
static void send_transmit_power_level_access_rsp(const GTPSS *const tps,
                                                 u16 cid,
                                                 u8 tx_level)
{
    send_transmit_power_access_rsp((Task)&tps->lib_task, 
                                   cid,
                                   HANDLE_TRANSMIT_POWER_LEVEL,
                                   gatt_status_success,
                                   1,
                                   &tx_level);
}

/***************************************************************************
NAME
    send_link_loss_access_error_rsp

DESCRIPTION
    Send an error access response to the GATT Manager library.
*/
static void send_transmit_power_access_error_rsp(const GTPSS *const tps,
                                                 const GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind,
                                                 u16 error)
{
    send_transmit_power_access_rsp((Task)&tps->lib_task,
                                   access_ind->cid,
                                   access_ind->handle,
                                   error,
                                   0,
                                   NULL);
}

/***************************************************************************
NAME
    handle_transmit_power_level_access_ind

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND_T message sent to the HANDLE_TRANSMIT_POWER_LEVEL handle.
*/
static void handle_transmit_power_level_access_ind(GTPSS *const tps,
                                                   const GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    gatt_status_t status = gatt_status_success;

    if (access_ind->flags & ATT_ACCESS_READ)
    {
        /* Send the request to connection library */
        tp_bdaddr tpaddr;
        VmGetBdAddrtFromCid(access_ind->cid, &tpaddr);
        tpaddr.transport = TRANSPORT_BLE_ACL;
        /* Call the connection manager api to read the tx power level */
        ConnectionReadTxPower((Task)&tps->lib_task, &tpaddr);
    }
    else
    {
        if (access_ind->flags & ATT_ACCESS_WRITE)
        {
            /* Read not permitted */
            status = gatt_status_write_not_permitted;
        }
        else
        {
            /* Other requests are not permitted */
            status = gatt_status_request_not_supported;
        }
        /* Send the error response back to GATT Manager */
        send_transmit_power_access_error_rsp(tps, access_ind, status);
    }
}

/***************************************************************************
NAME
    handle_transmit_power_service_access_ind

DESCRIPTION
    Deals with access of the HANDLE_TRANSMIT_POWER_SERVER_SERVICE handle.
*/
static void handle_transmit_power_service_access_ind(GTPSS *const tps,
                                                     const GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        send_transmit_power_access_rsp((Task)&tps->lib_task, 
                                       access_ind->cid,
                                       HANDLE_TRANSMIT_POWER_SERVER_SERVICE,
                                       gatt_status_success,
                                       0,
                                       NULL);
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        /* Write of transmit power servic handle not allowed. */
        send_transmit_power_access_rsp((Task)&tps->lib_task,
                                       access_ind->cid,
                                       HANDLE_TRANSMIT_POWER_SERVER_SERVICE,
                                       gatt_status_write_not_permitted,
                                       0,
                                       NULL);
    }
    else
    {
        /* Reject access requests that aren't read/write, which shouldn't happen. */
        send_transmit_power_access_rsp((Task)&tps->lib_task,
                                       access_ind->cid,
                                       HANDLE_TRANSMIT_POWER_SERVER_SERVICE,
                                       gatt_status_request_not_supported,
                                       0,
                                       NULL);
    }
}

/***************************************************************************
NAME
    handle_transmit_power_access_ind

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND_T message that was sent to the library.
*/
static void handle_transmit_power_access_ind(GTPSS *const tps,
                                             const GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    switch (access_ind->handle)
    {
        case HANDLE_TRANSMIT_POWER_LEVEL:
        {
            handle_transmit_power_level_access_ind(tps, access_ind);
        }
        break;

        case HANDLE_TRANSMIT_POWER_SERVER_SERVICE:
        {
            handle_transmit_power_service_access_ind(tps, access_ind);
        }
        break;

        default:
        {
            /* Respond to invalid handles */
            send_transmit_power_access_error_rsp(tps, access_ind, gatt_status_invalid_handle);
        }
        break;
    }
}

/****************************************************************************/
static void handleGattManagerMsgHandler(Task task, MessageId id, Message msg)
{
    GTPSS *const tps = (GTPSS*)task;
    switch (id)
    {
        case GATT_MANAGER_SERVER_ACCESS_IND:
        {
            handle_transmit_power_access_ind(tps, (GATT_MANAGER_SERVER_ACCESS_IND_T *)msg);
        }
        break;
        default:
        {
            /* Ignore unrecognised messages */
        }
        break;
    }
}

/****************************************************************************/
static void handleConnectionManagerMsgHandler(Task task, MessageId id, Message msg)
{
    GTPSS *const tps = (GTPSS*)task;
    switch (id)
    {
        case CL_DM_READ_TX_POWER_CFM:
        {
            CL_DM_READ_TX_POWER_CFM_T *readTxCfm = (CL_DM_READ_TX_POWER_CFM_T*)msg;
            if(readTxCfm != NULL)
            {
                u16 cid = GattGetCidForBdaddr(&readTxCfm->tpaddr.taddr);
                readTxCfm->tpaddr.transport = TRANSPORT_BLE_ACL;
                if(readTxCfm->status == hci_success)
                {
                    send_transmit_power_level_access_rsp(tps, cid, readTxCfm->tx_power);
                }
                else
                {
                    /* Since there is no mapping of HCI error to gatt_status, mapping it to
                        generic gatt failure */
                    send_transmit_power_access_rsp((Task)&tps->lib_task,
                                                   cid,
                                                   HANDLE_TRANSMIT_POWER_LEVEL,
                                                   gatt_status_failure,
                                                   0,
                                                   NULL);
                }
            }
        }
        break;
        default:
        {
            /* Ignore unrecognised messages */
        }
        break;
    }
}

/****************************************************************************/
void transmit_power_server_ext_msg_handler(Task task, MessageId id, Message msg)
{

    if((GATT_MANAGER_MESSAGE_BASE <= id) && (id < GATT_MANAGER_MESSAGE_TOP ))
    {
        handleGattManagerMsgHandler(task, id, msg);
    }
    else if((CL_MESSAGE_BASE <= id) && (id < CL_MESSAGE_TOP))
    {
        handleConnectionManagerMsgHandler(task, id, msg);
    }
    else
    {
         GATT_TRANSMIT_POWER_DEBUG_PANIC(("Unexpected MessageID : %d\n", id));
    }
}
