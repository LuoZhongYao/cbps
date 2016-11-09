/*
 * Copyright 2015 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.0
 */

#ifndef __BDADDR__H

#define __BDADDR__H

#include <csrtypes.h>
/*! @file bdaddr_.h @brief bdaddr structure*/
/* Yet another variant of Bluetooth address to avoid pulling in bluetooth.h */
/*! Bluetooth address structure. */
typedef struct { u32 lap; u8 uap; u16 nap; } bdaddr;
/*! Typed Bluetooth address structure.*/
typedef struct { u8   type; bdaddr  addr; } typed_bdaddr;
#define TYPED_BDADDR_PUBLIC    ((u8)0x00)
#define TYPED_BDADDR_RANDOM    ((u8)0x01)
#define TYPED_BDADDR_INVALID   ((u8)0xFF)
/*! ACL type logical transport. */
typedef enum {TRANSPORT_BREDR_ACL, TRANSPORT_BLE_ACL, TRANSPORT_NONE = 0xFF} TRANSPORT_T;
/*! Typed Bluetooth address structure with Transport */
typedef struct { typed_bdaddr taddr; TRANSPORT_T transport; } tp_bdaddr;

#endif
