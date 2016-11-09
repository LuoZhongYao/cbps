/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_BATTERY_SERVER_ACCESS_H_
#define GATT_BATTERY_SERVER_ACCESS_H_


#include <gatt_manager.h>

#include "gatt_battery_server.h"


/***************************************************************************
NAME
    handleBatteryAccess

DESCRIPTION
    Handles the GATT_MANAGER_SERVER_ACCESS_IND message that was sent to the battery library.
*/
void handleBatteryAccess(GBASS *battery_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);


/***************************************************************************
NAME
    sendBatteryLevelAccessRsp

DESCRIPTION
    Sends a battery level response back to the GATT Manager.
*/
void sendBatteryLevelAccessRsp(const GBASS *battery_server, u16 cid, u8 battery_level, u16 result);


/***************************************************************************
NAME
    sendBatteryConfigAccessRsp

DESCRIPTION
    Sends an client configuration access response back to the GATT Manager library.
*/
void sendBatteryConfigAccessRsp(const GBASS *battery_server, u16 cid, u16 client_config);


/***************************************************************************
NAME
    sendBatteryPresentationAccessRsp

DESCRIPTION
    Sends an presentation access response back to the GATT Manager library.
*/
void sendBatteryPresentationAccessRsp(const GBASS *battery_server, u16 cid, u8 name_space, u16 description);


#endif
