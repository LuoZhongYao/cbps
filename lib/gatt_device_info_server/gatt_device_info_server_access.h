/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_DEVICE_INFO_SERVER_ACCESS_H_
#define GATT_DEVICE_INFO_SERVER_ACCESS_H_


#include <gatt_manager.h>

#include "gatt_device_info_server.h"

/***************************************************************************
NAME
    handleDeviceInfoServerAccess

DESCRIPTION
    Deals with access of Device Information Service chanracteristics.
*/

void handleDeviceInfoServerAccess(gdiss_t *dev_info_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

/***************************************************************************
NAME
    sendDeviceInfoAccessRsp

DESCRIPTION
    Sends a server access read response back to the GATT Manager.
*/
void sendDeviceInfoAccessRsp(Task task,
                                    u16 cid,
                                    u16 handle,
                                    u16 result,
                                    u16 size_value,
                                    const u8 *value);

#endif

