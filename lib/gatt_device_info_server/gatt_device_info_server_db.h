/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT!
 *
 * generated by gattdbgen from gatt_device_info_server/gatt_device_info_server_db.dbi_
 */
#ifndef __GATT_DEVICE_INFO_SERVER_DB_H
#define __GATT_DEVICE_INFO_SERVER_DB_H

#include <csrtypes.h>

#define HANDLE_DEVICE_INFORMATION_SERVICE (0x0001)
#define HANDLE_DEVICE_INFORMATION_SERVICE_END (0xffff)
#define HANDLE_MANUFACTURER_NAME        (0x0003)
#define HANDLE_MODEL_NUMBER             (0x0005)
#define HANDLE_SERIAL_NUMBER            (0x0007)
#define HANDLE_HARDWARE_REVISION        (0x0009)
#define HANDLE_FIRMWARE_REVISION        (0x000b)
#define HANDLE_SOFTWARE_REVISION        (0x000d)
#define HANDLE_SYSTEM_ID                (0x000f)
#define HANDLE_IEEE_DATA                (0x0011)
#define HANDLE_PNP_ID                   (0x0013)

u16 *GattGetDatabase(u16 *len);
u16 GattGetDatabaseSize(void);

#endif

/* End-of-File */
