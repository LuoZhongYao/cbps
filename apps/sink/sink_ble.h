/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sink_ble.h

DESCRIPTION
    BLE functionality
*/

#ifndef _SINK_BLE_H_
#define _SINK_BLE_H_

#include "sink_ble_gap.h"
#include "sink_gatt_device.h"

#include <csrtypes.h>
#include <message.h>


#ifdef GATT_ENABLED


#define BLE theSink.rundata->ble
#define BLE_INTERNAL_MESSAGE_BASE 0

/* The bits used to enable BLE Advertising filters, as read from CONFIG_FEATURE_BLOCK */
#define ANCS_AD_BIT           0           /*!< @brief Bit used to enable ANCS advertising filter */
#define BATTERY_AD_BIT        1           /*!< @brief Bit used to enable Battery advertising filter */
#define DIS_AD_BIT            2           /*!< @brief Bit used to enable DIS advertising filter */
#define HID_AD_BIT            3           /*!< @brief Bit used to enable HID advertising filter */
#define IAS_AD_BIT            4           /*!< @brief Bit used to enable IAS advertising filter */
#define SPC_AD_BIT            5           /*!< @brief Bit used to enable SPC advertising filter */

typedef struct __ble_configuration
{
    u16 bonding_pairing_period_s;
    u16 bonding_connection_period_s;
    u16 scan_interval_fast;
    u16 scan_window_fast;
    u16 gap_role_switch_timer_s;
    u16 whitelist_scan_timeout_when_private_addr_s;
    u16 scan_timer_fast_s;
    u16 scan_interval_slow;
    u16 scan_window_slow;
    u16 adv_interval_min_fast;
    u16 adv_interval_max_fast;
    u16 adv_timer_fast_s;
    u16 adv_interval_min_slow;
    u16 adv_interval_max_slow;
} ble_configuration_t;


typedef struct __ble_connection_initial_parameters
{
    u16 conn_interval_min;
    u16 conn_interval_max;
    u16 conn_latency;
    u16 supervision_timeout;
    u16 conn_attempt_timeout;
    u16 conn_latency_max;
    u16 supervision_timeout_min;
    u16 supervision_timeout_max;
} ble_connection_initial_parameters_t; 


typedef struct __ble_connection_update_parameters
{
    u16 conn_interval_min;
    u16 conn_interval_max;
    u16 conn_latency;
    u16 supervision_timeout;
    u16 ce_length_min;
    u16 ce_length_max;
} ble_connection_update_parameters_t;


typedef struct __ble_advertising_parameters
{
    u16 interval_min;
    u16 interval_max;
} ble_advertising_parameters_t;


typedef struct __ble_scanning_parameters
{
    u16 interval;
    u16 window;
} ble_scanning_parameters_t;


typedef struct __ble_data
{
    TaskData task;              /* Handler for BLE messages */
    ble_gap_t gap;              /* GAP related data */
    gatt_data_t gatt;           /* GATT related data */
} ble_data_t;


typedef enum __ble_internal_message_id
{
    BLE_INTERNAL_MESSAGE_INIT_COMPLETE = BLE_INTERNAL_MESSAGE_BASE,
    BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS,
    BLE_INTERNAL_MESSAGE_WHITELIST_TIMER,
    BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER,
    BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER,
    BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER,
    BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES,
    BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED,
    BLE_INTERNAL_MESSAGE_BATTERY_READ_TIMER,

    /* End message limit */
    BLE_INTERNAL_MESSAGE_TOP
} ble_internal_message_id_t;


typedef struct __BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES
{
    u16 cid;
} BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES_T;

typedef struct __BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED
{
    u16 cid;
} BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED_T;


#endif /* GATT_ENABLED */


/*******************************************************************************
NAME
    sinkBleInitialiseDevice
    
DESCRIPTION
    Initialises BLE functionality in the application.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleInitialiseDevice(void);
#else
#define sinkBleInitialiseDevice() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleMsgHandler
    
DESCRIPTION
    Handle BLE related messages
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the message
    payload The message payload
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkBleMsgHandler(task, id, message) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleIsActiveOnPowerOff
    
DESCRIPTION
    Finds if BLE operation is still active when the application is powered off.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#if defined(GATT_ENABLED) && defined(BLE_ENABLED_WHEN_OFF)
#define sinkBleIsActiveOnPowerOff() (TRUE)
#else
#define sinkBleIsActiveOnPowerOff() (FALSE)
#endif


/*******************************************************************************
NAME
    sinkBleBondableEvent
    
DESCRIPTION
    Acts on a BLE bondable event, which is an attempt to make the application bondable.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleBondableEvent(void);
#else
#define sinkBleBondableEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleBondablePairingTimeoutEvent
    
DESCRIPTION
    Acts on a BLE bondable timeout event, which should exit bondable mode.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleBondablePairingTimeoutEvent(void);
#else
#define sinkBleBondablePairingTimeoutEvent() ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleBondableConnectionTimeoutEvent
    
DESCRIPTION
    Acts on a BLE bond connection timeout event, which should exit bondable connection mode.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleBondableConnectionTimeoutEvent(void);
#else
#define sinkBleBondableConnectionTimeoutEvent() ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleCentralConnCompleteEvent
    
DESCRIPTION
    Acts on a BLE central connection complete event, which should exit bondable mode.
    
PARAMETERS
    cid     The connection ID for the completed connection.
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleCentralConnCompleteEvent(u16 cid);
#else
#define sinkBleCentralConnCompleteEvent(cid) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBlePairingCompleteEvent 
    
DESCRIPTION
    Acts on a BLE pairing complete event while in peripheral/central role
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBlePairingCompleteEvent (void);
#else
#define sinkBlePairingCompleteEvent () ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSwitchPeripheralEvent
    
DESCRIPTION
    Acts on a BLE switch peripheral event which should put the sink into 
    peripheral mode.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleSwitchPeripheralEvent(void);
#else
#define sinkBleSwitchPeripheralEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSwitchCentralEvent
    
DESCRIPTION
    Acts on a BLE switch central event which should put the sink into 
    central mode.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleSwitchCentralEvent(void);
#else
#define sinkBleSwitchCentralEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleCentralDisconnectionEvent
    
DESCRIPTION
    Acts on a BLE disconnection event linked to central role,
    which should resume scanning.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleCentralDisconnectionEvent(void);
#else
#define sinkBleCentralDisconnectionEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleCancelAdvertisingEvent
    
DESCRIPTION
    Acts on a BLE cancel advertising event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleCancelAdvertisingEvent(void);
#else
#define sinkBleCancelAdvertisingEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleRemoteConnectionSuccessEvent
    
DESCRIPTION
    Acts on a BLE remote connection success event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleRemoteConnectionSuccessEvent(u16 cid);
#else
#define sinkBleRemoteConnectionSuccessEvent(cid) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleRemoteConnectionFailEvent
    
DESCRIPTION
    Acts on a BLE remote connection fail event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleRemoteConnectionFailEvent(void);
#else
#define sinkBleRemoteConnectionFailEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleLocalConnectionSuccessEvent
    
DESCRIPTION
    Acts on a BLE local connection success event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleLocalConnectionSuccessEvent(void);
#else
#define sinkBleLocalConnectionSuccessEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleRoleTimeoutEvent
    
DESCRIPTION
    Acts on a BLE GAP role timeout event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleRoleTimeoutEvent(void);
#else
#define sinkBleRoleTimeoutEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleCheckNoConnectionsEvent
    
DESCRIPTION
    Check connections and report if no connections exist.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleCheckNoConnectionsEvent(void);
#else
#define sinkBleCheckNoConnectionsEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleRetryEncryptionEvent
    
DESCRIPTION
    Retry Encryption upon host busy pairing encryption status.
    
PARAMETERS
    cid     The connection ID for the connection.
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleRetryEncryptionEvent(u16 cid);
#else
#define sinkBleRetryEncryptionEvent(cid) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBlePowerOnEvent
    
DESCRIPTION
    BLE power on event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBlePowerOnEvent(void);
#else
#define sinkBlePowerOnEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBlePowerOffEvent
    
DESCRIPTION
    BLE power off event.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBlePowerOffEvent(void);
#else
#define sinkBlePowerOffEvent() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleGetConfiguration
    
DESCRIPTION
    Gets the BLE configurable data.
    
PARAMETERS
    None.
    
RETURNS
    The BLE configuration data.
*/
#ifdef GATT_ENABLED
const ble_configuration_t *sinkBleGetConfiguration(void);
#else
#define sinkBleGetConfiguration() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSetAdvertisingParamsDefault
    
DESCRIPTION
    Sets the default advertising parameters.
    
PARAMETERS
    None.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleSetAdvertisingParamsDefault(u16 adv_interval_min, u16 adv_interval_max);
#else
#define sinkBleSetAdvertisingParamsDefault(adv_interval_min, adv_interval_max) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSetMasterConnectionParamsDefault
    
DESCRIPTION
    Sets the default connection parameters when Master of the connection.
    
PARAMETERS
    None.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleSetMasterConnectionParamsDefault(u16 scan_interval, u16 scan_window);
#else
#define sinkBleSetMasterConnectionParamsDefault(scan_interval, scan_window) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSetMasterConnectionParamsUpdate
    
DESCRIPTION
    Sets the updated connection parameters when Master of the connection.
    
PARAMETERS
    taddr  Address of the remote device to update connection parameters with.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleSetMasterConnectionParamsUpdate(typed_bdaddr *taddr);
#else
#define sinkBleSetMasterConnectionParamsUpdate(taddr) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleSetSlaveConnectionParamsUpdate
    
DESCRIPTION
    Sets the updated connection parameters when Slave of the connection.
    
PARAMETERS
    taddr  Address of the remote device to update connection parameters with.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleSetSlaveConnectionParamsUpdate(typed_bdaddr *taddr);
#else
#define sinkBleSetSlaveConnectionParamsUpdate(taddr) ((void)(0))
#endif



/*******************************************************************************
NAME
    sinkBleGetAdvertisingParameters
    
DESCRIPTION
    Gets the advertising parameters based on speed setting.
    
PARAMETERS
    fast_adv    Should be TRUE if fast advertising. FALSE otherwise.
    adv_params  The advertising parameters will be set into this variable.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleGetAdvertisingParameters(bool fast_adv, ble_advertising_parameters_t *adv_params);
#else
#define sinkBleGetAdvertisingParameters(fast_adv, adv_params) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleGetScanningParameters
    
DESCRIPTION
    Gets the scanning parameters based on speed setting.
    
PARAMETERS
    fast_scan    Should be TRUE if fast advertising. FALSE otherwise.
    scan_params  The scan parameters will be set into this variable.
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleGetScanningParameters(bool fast_scan, ble_scanning_parameters_t *scan_params);
#else
#define sinkBleGetScanningParameters(fast_scan, scan_params) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleDeleteMarkedDevices
    
DESCRIPTION
    Delete the last failed encryption paired device
    
PARAMETERS
    None
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleDeleteMarkedDevices(void);
#else
#define sinkBleDeleteMarkedDevices() ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleSimplePairingCompleteInd
    
DESCRIPTION
    Handle received CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND as a result of BLE bonding procedure.
    
PARAMETERS
    None
    
RETURNS
    None.
*/
#ifdef GATT_ENABLED
void sinkBleSimplePairingCompleteInd(const CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T *ind);
#else
#define sinkBleSimplePairingCompleteInd(message) ((void)(0))
#endif

#endif /* _SINK_BLE_H_ */
