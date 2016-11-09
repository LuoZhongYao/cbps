/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_BATTERY_CLIENT_INIT_H_
#define GATT_BATTERY_CLIENT_INIT_H_


#include <gatt_manager.h>

#include "gatt_battery_client.h"


/***************************************************************************
NAME
    gattBatteryInitSendInitCfm
    
PARAMETERS
    battery_client The battery client task.
    handle The battery level handle to add to the message.
    status The status code to add to the message.

DESCRIPTION
    Send a GATT_BATTERY_CLIENT_INIT_CFM message to the registered client task.
    
RETURN
    void
*/
void gattBatteryInitSendInitCfm(GBASC *battery_client, u16 handle, gatt_battery_client_status_t status);


#endif
