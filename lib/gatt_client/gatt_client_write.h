/* Copyright (c) 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_CLIENT_WRITE_H_
#define GATT_CLIENT_WRITE_H_


#include "gatt_client.h"

#include <gatt.h>
#include "gatt_manager.h"

/***************************************************************************
NAME
    writeGattClientConfigValue

DESCRIPTION
    Write Client Configuration descriptor value by handle.
*/
void writeGattClientConfigValue(GGATTC *gatt_client, u16 handle);


/***************************************************************************
NAME
    handleWriteValueResp

DESCRIPTION
    Handle response as a result of writing a characteristic value.
*/
void handleWriteValueResp(GGATTC *gatt_client, const GATT_MANAGER_WRITE_CHARACTERISTIC_VALUE_CFM_T *cfm);


#endif
