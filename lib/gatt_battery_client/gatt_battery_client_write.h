/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_BATTERY_CLIENT_WRITE_H_
#define GATT_BATTERY_CLIENT_WRITE_H_


#include <gatt_manager.h>

#include "gatt_battery_client.h"


/***************************************************************************
NAME
    writeClientConfigValue

DESCRIPTION
    Write Client Configuration descriptor value by handle.
*/
void writeClientConfigValue(GBASC *battery_client, bool notifications_enable, u16 handle);


/***************************************************************************
NAME
    handleBatteryWriteValueResp

DESCRIPTION
    Handle response as a result of writing a characteristic value.
*/
void handleBatteryWriteValueResp(GBASC *battery_client, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *write_cfm);


#endif
