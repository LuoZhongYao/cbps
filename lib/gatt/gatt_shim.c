/* Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#include <panic.h>
#include <message.h>
#include <stdlib.h>
#include <string.h> /* for memset */
#include <vm.h>
#include "gatt_shim.h"
#include "gatt_private.h"

#include <stdio.h>

/* Shorthand macro */
#define MAKE_TEST_EXTRA_WITH_UUID(TYPE, UUID_TYPE) \
    TYPE##_T *new_msg; \
    if ((UUID_TYPE) == gatt_uuid16) \
        new_msg = malloc(sizeof(TYPE##_T) + 1); \
    else if ((UUID_TYPE) == gatt_uuid32) \
        new_msg = malloc(sizeof(TYPE##_T) + 3); \
    else \
        new_msg = malloc(sizeof(TYPE##_T) + 15);

/* Flatten UUID to array of u8 */
static u16 flatten_uuid(
                    const u32 *uuid,         /* from */
                    u8 *ptr,                 /* to */
                    gatt_uuid_type_t uuid_type  /* type */
                    )
{
    u16 size_uuid;
    if (uuid_type == gatt_uuid16)
    {
        size_uuid = 2;
        /* Big endian octet order */
        *ptr++ = (u8)(uuid[0]>>8);
        *ptr++ = (u8)uuid[0];
    }
    else if (uuid_type == gatt_uuid32)
    {
        size_uuid = 4;
        /* Big endian octet order */
        *ptr++ = (u8)(uuid[0]>>24);
        *ptr++ = (u8)(uuid[0]>>16);
        *ptr++ = (u8)(uuid[0]>>8);
        *ptr++ = (u8)uuid[0];
    }
    else
    {
        u8 i;
        size_uuid = 16;
        for (i=0; i<4; i++)
            /* 4 octets at a time, 4 times is 16 octets */
        {
            /* Big endian octet order */
            *ptr++ = (u8)(uuid[i]>>24);
            *ptr++ = (u8)(uuid[i]>>16);
            *ptr++ = (u8)(uuid[i]>>8);
            *ptr++ = (u8)uuid[i];
        }
    }
    return size_uuid;
}


void GattHandleComplexMessage(Task task, MessageId id, Message message)
{
    switch (id)
    {
        case GATT_INIT_CFM:
            {
                GATT_INIT_CFM_T *original = (GATT_INIT_CFM_T *)message;
                GATT_INIT_CFM_TEST_EXTRA_T *new_msg = 
                    malloc(sizeof(GATT_INIT_CFM_TEST_EXTRA_T));

                new_msg->status = original->status;
                MessageSend(task, GATT_INIT_CFM_TEST_EXTRA, new_msg);
            }
            break;

        case GATT_CONNECT_CFM:
            {
                GATT_CONNECT_CFM_T *original = (GATT_CONNECT_CFM_T *)message;
                GATT_CONNECT_CFM_TEST_EXTRA_T *new_msg =
                    malloc(sizeof(GATT_CONNECT_CFM_TEST_EXTRA_T));

                new_msg->status = original->status;
                new_msg->type = original->taddr.type;
                new_msg->addr = original->taddr.addr;
                new_msg->flags = original->flags;
                new_msg->cid = original->cid;
                new_msg->mtu = original->mtu;

                MessageSend(task, GATT_CONNECT_CFM_TEST_EXTRA, new_msg);
            }
            break;


        case GATT_CONNECT_IND:
            {
                GATT_CONNECT_IND_T *original = (GATT_CONNECT_IND_T *)message;
                GATT_CONNECT_IND_TEST_EXTRA_T *new_msg = 
                    malloc(sizeof(GATT_CONNECT_IND_TEST_EXTRA_T));

                new_msg->type = original->taddr.type;
                new_msg->addr = original->taddr.addr;
                new_msg->flags = original->flags;
                new_msg->cid = original->cid;
                new_msg->mtu = original->mtu;

                MessageSend(task, GATT_CONNECT_IND_TEST_EXTRA, new_msg);
            }
            break;
            
        /* These are identical so same code to deal with all. */
        case GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM:
        case GATT_DISCOVER_PRIMARY_SERVICE_CFM:
        case GATT_FIND_INCLUDED_SERVICES_CFM:
            {
                MessageId send_id;
                
                GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_T *original = 
                    (GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_T *)message;

                MAKE_TEST_EXTRA_WITH_UUID( 
                    GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_TEST_EXTRA,
                    original->uuid_type
                    );
                    
                new_msg->status         = original->status;
                new_msg->cid            = original->cid;
                new_msg->handle         = original->handle;
                new_msg->end            = original->end;
                new_msg->more_to_come   = original->more_to_come;

                new_msg->size_uuid = flatten_uuid(
                    original->uuid,
                    new_msg->uuid,
                    original->uuid_type
                    );

                switch(id)
                {
                    case GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM:
                        send_id = 
                            GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_TEST_EXTRA;
                        break;
                    case GATT_DISCOVER_PRIMARY_SERVICE_CFM:
                        send_id = GATT_DISCOVER_PRIMARY_SERVICE_CFM_TEST_EXTRA;
                        break;
                    default:
                        send_id = GATT_FIND_INCLUDED_SERVICES_CFM_TEST_EXTRA;
                        break;
                }
                
                MessageSend(task, send_id, new_msg);
            }
            break;

        case GATT_DISCOVER_ALL_CHARACTERISTICS_CFM:
            {
                GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_T *original = 
                    (GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_T *)message;
                
                MAKE_TEST_EXTRA_WITH_UUID( 
                    GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_TEST_EXTRA,
                    original->uuid_type
                    );
                
                new_msg->status         = original->status;
                new_msg->cid            = original->cid;
                new_msg->handle         = original->handle;
                new_msg->properties     = original->properties;
                new_msg->more_to_come   = original->more_to_come;

                new_msg->size_uuid = flatten_uuid(
                                        original->uuid,
                                        new_msg->uuid,
                                        original->uuid_type
                                        );

                MessageSend(
                    task,
                    GATT_DISCOVER_ALL_CHARACTERISTICS_CFM_TEST_EXTRA,
                    new_msg
                    );
            }
            break;

        case GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM:
            {
                GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *original =
                    (GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *)message;

                MAKE_TEST_EXTRA_WITH_UUID(
                    GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_TEST_EXTRA,
                    original->uuid_type
                    );

                new_msg->cid            = original->cid;
                new_msg->handle         = original->handle;
                new_msg->more_to_come   = original->more_to_come;
                new_msg->status         = original->status;

                new_msg->size_uuid = flatten_uuid(
                                        original->uuid,
                                        new_msg->uuid,
                                        original->uuid_type
                                        );
                
                MessageSend(
                    task,
                    GATT_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_TEST_EXTRA,
                    new_msg
                    );
                }
                break;

        default:
            GATT_DEBUG(("Unknown Message %d %04x\n", id, id));
            break;
    }
}

void GattInitTestExtraDefault(
        Task    theAppTask, 
        u16  size_database, 
        u8*  database
        )
{
    if (size_database)
    {
        u16 *real_db = malloc(sizeof(u16) * (size_database/2));
        u16 i;

        for (i=0; i<size_database/2; i++)
        {
            real_db[i] = *database++ << 8;
            real_db[i] |= *database++;
        }
        GattInit(theAppTask, size_database/2, real_db);
    }
    else
    {
        GattInit(theAppTask, 0, 0);
    }
}

void GattConnectRequestTestExtraDefault(
        Task                    theAppTask,
        bdaddr                  *bd_addr,
        u8                   bdaddr_type,
        gatt_connection_type    conn_type
        )
{
    typed_bdaddr taddr;
    taddr.type = bdaddr_type;
    taddr.addr = *bd_addr;

    GattConnectRequest(theAppTask, &taddr, conn_type, 1);
}

void GattConnectRequestTestExtra(
        Task                    theAppTask,
        bdaddr                  *bd_addr,
        u8                   bdaddr_type,
        gatt_connection_type    conn_type,
        bool                    conn_timeout
        )
{
    typed_bdaddr taddr;
    taddr.type = bdaddr_type;
    taddr.addr = *bd_addr;

    GattConnectRequest(theAppTask, &taddr, conn_type, conn_timeout);
}


void GattDiscoverPrimaryServiceRequestTestExtraDefault(
        Task            theAppTask, 
        u16          cid, 
        u16          size_uuid, 
        const u8     *uuid
        )
{
    gatt_uuid_type_t uuid_type = gatt_uuid_none;
    gatt_uuid_t gatt_uuid[4];
    
    memset(gatt_uuid, 0, 8);

    if (size_uuid == 2)
        /* UUID16 */
    {
        uuid_type = gatt_uuid16;
        /* Big endian octet order */
        gatt_uuid[0] = ((u32)uuid[0]<<8) | ((u32)uuid[1]);
    }
    else if (size_uuid == 4)
        /* UUID32 */
    {
        uuid_type = gatt_uuid32;
        /* Big endian octet order */
        gatt_uuid[0] = ((u32)uuid[0]<<24) | ((u32)uuid[1]<<16)
                | ((u32)uuid[2]<<8) | ((u32)uuid[3]);
    }
    else if (size_uuid == 16)
        /* UUID128 */
    {
        u8 i;

        uuid_type = gatt_uuid128;    
        
        for (i=0; i<4; i++)
            /* 4 octets at a time, 4 times is 16 octets */
        {
            /* Big endian octet order */
            gatt_uuid[i] =  ((u32)(*uuid++))<<24;
            gatt_uuid[i] |= ((u32)(*uuid++))<<16;
            gatt_uuid[i] |= ((u32)(*uuid++))<<8;
            gatt_uuid[i] |= (u32)*uuid++;
        }
    }
    else
    {
        /* size_uuid has to be 2, 4 or 16 and nothing else. */
        Panic();
    }

    GattDiscoverPrimaryServiceRequest(
        theAppTask,
        cid,
        uuid_type,
        gatt_uuid
        );
}

