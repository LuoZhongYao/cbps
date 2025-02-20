/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT!
 *
 * generated by gattdbgen from gatt_server/gatt_server_db.dbi_
 */
#ifndef __GATT_SERVER_DB_H
#define __GATT_SERVER_DB_H

#include <csrtypes.h>

#define HANDLE_GATT_SERVICE             (0x0001)
#define HANDLE_GATT_SERVICE_END         (0xffff)
#define HANDLE_GATT_SERVICE_CHANGED     (0x0003)
#define HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG (0x0004)

u16 *GattGetDatabase(u16 *len);
u16 GattGetDatabaseSize(void);

#endif

/* End-of-File */
