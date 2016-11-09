/* Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_SHIM_LAYER_H
#define GATT_SHIM_LAYER_H

#include <gatt.h>

typedef struct 
{
    gatt_status_t     status;     
} GATT_INIT_CFM_TEST_EXTRA_T;

/* flatten typed_addr to type and bdaddr. */
typedef struct
{
    gatt_status_t           status;  
    u8                   type;
    bdaddr                  addr;      
    u16                  flags;      
    u16                  cid;        
    u16                  mtu;        
} GATT_CONNECT_CFM_TEST_EXTRA_T;   

/* flatten the typed_bdaddr to type and bdaddr */
typedef struct
{
    u8                   type;
    bdaddr                  addr;
    u16                  flags;
    u16                  cid;
    u16                  mtu;
} GATT_CONNECT_IND_TEST_EXTRA_T;

/* flatten the uuid u32 array into a bigendian u8 array. */
typedef struct
{
    gatt_status_t status;    
    u16 cid;
    u16 handle;
    u16 end;
    bool more_to_come;
    u16 size_uuid;
    u8 uuid[1];
} GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_TEST_EXTRA_T;

typedef struct
{
    gatt_status_t status;    
    u16 cid;
    u16 handle;
    u16 end;
    bool more_to_come;
    u16 size_uuid;
    u8 uuid[1];
} GATT_DISCOVER_PRIMARY_SERVICE_CFM_TEST_EXTRA_T;

typedef struct
{
    gatt_status_t status;    
    u16 cid;
    u16 handle;
    u16 end;
    bool more_to_come;
    u16 size_uuid;
    u8 uuid[1];
} GATT_FIND_INCLUDED_SERVICES_CFM_TEST_EXTRA_T;

typedef struct 
{
    gatt_status_t status;
    u16 cid;
    u16 handle;
    u8 properties;
    bool more_to_come;
    u16 size_uuid;
    u8 uuid[1];
} GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_TEST_EXTRA_T;

typedef struct 
{
    gatt_status_t status;
    u16 cid;
    u16 handle;
    bool more_to_come;
    u16 size_uuid;
    u8 uuid[1];
} GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_TEST_EXTRA_T;


#define GATT_SHIM_MESSAGE_BASE
typedef enum
{
    GATT_INIT_CFM_TEST_EXTRA = GATT_MESSAGE_TOP,
    GATT_CONNECT_CFM_TEST_EXTRA,
    GATT_CONNECT_IND_TEST_EXTRA,
    GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_TEST_EXTRA, 
    GATT_DISCOVER_PRIMARY_SERVICE_CFM_TEST_EXTRA,
    GATT_FIND_INCLUDED_SERVICES_CFM_TEST_EXTRA,
    GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_TEST_EXTRA,
    GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_TEST_EXTRA,

    /* Library message limit*/
    GATT_SHIM_MESSAGE_TOP
} GattShimMessageId;


void GattHandleComplexMessage(Task task, MessageId id, Message message);

void GattInitTestExtraDefault(Task theAppTask, u16 size_database, u8* database);

void GattConnectRequestTestExtraDefault(
        Task                    theAppTask,
        bdaddr                  *bd_addr,
        u8                   bdaddr_type,
        gatt_connection_type    conn_type
        );

void GattConnectRequestTestExtra(
        Task                    theAppTask,
        bdaddr                  *bd_addr,
        u8                   bdaddr_type,
        gatt_connection_type    conn_type,
        bool                    conn_timeout
        );

void GattDiscoverPrimaryServiceRequestTestExtraDefault(
        Task                    theAppTask,
        u16                  cid,
        u16                  size_uuid,
        const                   u8 *uuid
        );


#endif /* GATT_SHIM_LAYER_H */
