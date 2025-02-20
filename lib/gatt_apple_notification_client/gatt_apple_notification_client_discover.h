/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_


#include <gatt_manager.h>

#include "gatt_apple_notification_client.h"


/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM message for 'Discover All Characteristics' response.
*/
void handleAncsDiscoverAllCharacteristicsResp(GANCS *ancs, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm);


/***************************************************************************
NAME
    ancsDiscoverAllCharacteristicDescriptors

DESCRIPTION
    Discover all characteristics descriptors for the characteristic with range of start_handle to end_handle.
*/
bool ancsDiscoverAllCharacteristicDescriptors(GANCS *ancs, u16 start_handle, u16 end_handle);


/***************************************************************************
NAME
    handleAncsDiscoverAllCharacteristicDescriptorsResp

DESCRIPTION
    Handles GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM message  for 'Discover All Characteristic Descriptors' response..
*/
void handleAncsDiscoverAllCharacteristicDescriptorsResp(GANCS *ancs, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm);

u16 findEndHandleForCharDesc(GANCS *ancs, u16 startHandle, u16 endHandle, u8 charesteristic);

#endif /* GATT_APPLE_NOTIFICATION_CLIENT_DISCOVER_H_ */
