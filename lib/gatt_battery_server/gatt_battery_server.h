/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

/*!
@file    gatt_battery_server.h
@brief   Header file for the GATT battery server library.

        This file provides documentation for the GATT battery server library
        API (library name: gatt_battery_server).
*/

#ifndef GATT_BATTERY_SERVER_H_
#define GATT_BATTERY_SERVER_H_

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"

/*! @brief The battery server internal structure for the server role.

    This structure is visible to the application as it is responsible for managing resource to pass to the battery library.
    The elements of this structure are only modified by the battery library.
    The application SHOULD NOT modify the values at any time as it could lead to undefined behaviour.
    
 */
typedef struct __GBASS
{
    TaskData lib_task;
    Task app_task;
    unsigned notifications_enabled:1;
} GBASS;


/*!
    @brief Status code returned from the GATT battery server library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    gatt_battery_server_status_success,
    gatt_battery_server_status_registration_failed,
    gatt_battery_server_status_invalid_parameter,
    gatt_battery_server_status_not_allowed,
    gatt_battery_server_status_failed
} gatt_battery_server_status_t;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_LEVEL_IND message that is sent by the library,
    due to a read of the local battery level characteristic.
 */
typedef struct PACK_STRUCT __GATT_BATTERY_SERVER_READ_LEVEL_IND
{
    const GBASS *battery_server;
    u16 cid;
} GATT_BATTERY_SERVER_READ_LEVEL_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message that is sent by the library,
    due to a read of the local battery level client configuration characteristic.
 */
typedef struct PACK_STRUCT __GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND
{
    const GBASS *battery_server;
    u16 cid;
} GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND message that is sent by the library,
    due to a write of the local battery level client configuration characteristic.
 */
typedef struct PACK_STRUCT __GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND
{
    const GBASS *battery_server;
    u16 cid;
    u16 config_value;
} GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND_T;

/*! @brief Contents of the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message that is sent by the library,
    due to a read of the local presentation characteristic.
 */
typedef struct PACK_STRUCT __GATT_BATTERY_SERVER_READ_PRESENTATION_IND
{
    const GBASS *battery_server;
    u16 cid;
} GATT_BATTERY_SERVER_READ_PRESENTATION_IND_T;


/*! @brief Enumeration of messages an application task can receive from the battery server library.
 */
typedef enum
{
    /* Server messages */
    GATT_BATTERY_SERVER_READ_LEVEL_IND = GATT_BATTERY_SERVER_MESSAGE_BASE, /* 00 */
    GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND,                            /* 01 */
    GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND,                           /* 02 */
    GATT_BATTERY_SERVER_READ_PRESENTATION_IND,                             /* 03 */
    
    /* Library message limit */
    GATT_BATTERY_SERVER_MESSAGE_TOP
} gatt_battery_server_message_id_t;

/*!
    @brief Optional parameters used by the Initialisation API
    
    Parameters that can define how the battery server library is initialised.
 */
typedef struct
{
   bool enable_notifications;     /*! Flag that can be used to enable or disable notifications */
} gatt_battery_server_init_params_t;


/*!
    @brief Initialises the Battery Service Library in the Server role.

    @param app_task The Task that will receive the messages sent from this battery server library.
    @param battery_server A valid area of memory that the service library can use.
    
    @return The status result of calling the API.

*/
gatt_battery_server_status_t GattBatteryServerInit(GBASS *battery_server, 
                                                   Task app_task, 
                                                   const gatt_battery_server_init_params_t *init_params,
                                                   u16 start_handle,
                                                   u16 end_hanlde);


/*!
    @brief This API is used to return a battery level to the library when a GATT_BATTERY_SERVER_READ_LEVEL_IND message is received.

    @param battery_server The instance pointer that was in the payload of the GATT_BATTERY_SERVER_READ_LEVEL_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_LEVEL_IND message.
    @param battery_level The battery level to return to the library. Valid values from 0-100, a value greater than 100% indicates error.
    @param status Success or failure to pass to the client.

    @return The status result of calling the API.

*/
gatt_battery_server_status_t GattBatteryServerReadLevelResponse(const GBASS *battery_server, u16 cid, u8 battery_level);

/*!
    @brief This API is used to return a battery level client configuration value to the library when a GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message is received.

    @param battery_server The pointer that was in the payload of the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND message.
    @param client_config The client configuration to return to the library.
    
    @return The status result of calling the API.

*/
gatt_battery_server_status_t GattBatteryServerReadClientConfigResponse(const GBASS *battery_server, u16 cid, u16 client_config);

/*!
    @brief This API is used to return a battery level to the library when a GATT_BATTERY_SERVER_READ_PRESENTATION_IND message is received.

    @param battery_server The instance pointer that was in the payload of the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message.
    @param cid The connection identifier from the GATT_BATTERY_SERVER_READ_PRESENTATION_IND message.
    @param name_space The name space of the description (see Bluetooth.org Assigned Numbers for allowed values)
    @param description The description of this characteristic (see Bluetooth.org Assigned Numbers for Bluetooth SIG descriptions)
    
    @return The status result of calling the API.

*/
gatt_battery_server_status_t GattBatteryServerReadPresentationResponse(const GBASS *battery_server, u16 cid, u8 name_space, u16 description);

/*!
    @brief This API is used to notify a remote client of the battery level. 
    This will only be allowed if notifications have been enabled by the remote device.

    @param battery_server The instance pointer that was passed into the GattBatteryServerInit API.
    @param number_cids The number of connection ID values passed in the cids parameter.
    @param cids The connection ID list. The battery level notification will be sent on each connection that is supplied in the list.
    @param battery_level The battery level to send in the notification.
    
    @return The status result of calling the API.

*/
gatt_battery_server_status_t GattBatteryServerSendLevelNotification(const GBASS *battery_server, u16 number_cids, const u16 *cids, u8 battery_level);


#endif
