/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_BATTERY_CLIENT_PRIVATE_H_
#define GATT_BATTERY_CLIENT_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>


/* Macros for creating messages */
#define MAKE_BATTERY_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T)
#define MAKE_BATTERY_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicUnlessMalloc(sizeof(TYPE##_T) + LEN - sizeof(u8))


typedef struct
{
    bool notifications_enable;
} BATTERY_INTERNAL_MSG_SET_NOTIFICATION_T;

typedef struct
{
    u16 descriptor_uuid;
} BATTERY_INTERNAL_MSG_READ_DESCRIPTOR_T;

/* Enum for battery library internal message. */
typedef enum __battery_internal_msg_t
{
    BATTERY_INTERNAL_MSG_READ_LEVEL,
    BATTERY_INTERNAL_MSG_SET_NOTIFICATION,
    BATTERY_INTERNAL_MSG_READ_DESCRIPTOR
} battery_internal_msg_t;

/* Enum for battery read type. Only insert new entries at the end. */
typedef enum __battery_pending_cmd_t
{
    battery_pending_none = 0,
    battery_pending_discover_all_characteristics,
    battery_pending_discover_all_characteristic_descriptors,
    battery_pending_discover_descriptor,
    battery_pending_read_level,
    battery_pending_read_descriptor,
    battery_pending_write_cconfig,
    battery_pending_set_notify_enable,
    battery_pending_set_notify_disable
} battery_pending_cmd_t;


#endif
