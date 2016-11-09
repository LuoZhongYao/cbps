/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_DEVICE_INFO_CLIENT_PRIVATE_H_
#define GATT_DEVICE_INFO_CLIENT_PRIVATE_H_

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "gatt_device_info_client.h"

/* Macros for creating messages */
#define MAKE_DEVICE_INFO_CLIENT_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);

/* Assumes message struct with
 *    u16 size_value;
 *    u8 value[1];
 */
#define MAKE_DEVICE_INFO_CLIENT_MESSAGE_WITH_LEN(TYPE, LEN)                           \
    TYPE##_T *message = (TYPE##_T*)PanicUnlessMalloc(sizeof(TYPE##_T) + (LEN ? LEN - 1 : 0));	\
             memset(message, 0, (sizeof(TYPE##_T) + (LEN ? LEN - 1 : 0)))

 
/* GATT Invalid CID*/
#define INVALID_CID (0xffff)
/* Invalid start handle */
#define INVALID_GATT_START_HANDLE   (0xffff)
/*  Invalid end handle */
#define INVALID_GATT_END_HANDLE   (0x0)
/* Invalid Device Info Char handle*/
 #define INVALID_DEVICE_INFO_HANDLE (0xffff)
 
/* Enum for Device info client library internal message. */
typedef enum 
{
    DEVICE_INFO_CLIENT_INTERNAL_MSG_BASE = 0,
    DEVICE_INFO_CLIENT_INTERNAL_MSG_CONNECT,        /* For a connect Request */
    DEVICE_INFO_CLIENT_INTERNAL_MSG_READ_CHAR,      /* For reading device info characteristic */
    DEVICE_INFO_CLIENT_INTERNAL_MSG_TOP
} device_info_client_internal_msg_t;

/* Internal message structure for connect request  */
typedef struct
{
    u16 cid;           /* Connection Identifier for remote device */
    u16 start_handle;  /* Start handle of the service */
    u16 end_handle;    /* End handle of the service */
} DEVICE_INFO_CLIENT_INTERNAL_MSG_CONNECT_T;

/* Internal message structure for Read characteristic request  */
typedef struct
{
    gatt_device_info_type_t device_info_type;   /* Device information type to retrieve*/
    u16 device_info_char_handle;             /* Device information characteristic handle to retrieve*/
}DEVICE_INFO_CLIENT_INTERNAL_MSG_READ_CHAR_T;

u16 getDeviceInfoCharHandle(GDISC *const device_info_client, gatt_device_info_type_t device_info_type);

#endif
