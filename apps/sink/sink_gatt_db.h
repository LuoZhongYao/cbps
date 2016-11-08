/*
 * THIS FILE IS AUTOGENERATED, DO NOT EDIT!
 *
 * generated by gattdbgen from sink_gatt_db.db_
 */
#ifndef __SINK_GATT_DB_H
#define __SINK_GATT_DB_H

#include <csrtypes.h>

#define HANDLE_GATT_SERVICE             (0x0001)
#define HANDLE_GATT_SERVICE_END         (0x0004)
#define HANDLE_GATT_SERVICE_CHANGED     (0x0003)
#define HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG (0x0004)
#define HANDLE_GAP_SERVICE              (0x0005)
#define HANDLE_GAP_SERVICE_END          (0x0009)
#define HANDLE_DEVICE_NAME              (0x0007)
#define HANDLE_DEVICE_APPEARANCE        (0x0009)
#define HANDLE_TRANSMIT_POWER_SERVER_SERVICE (0x000a)
#define HANDLE_TRANSMIT_POWER_SERVER_SERVICE_END (0x000c)
#define HANDLE_TRANSMIT_POWER_LEVEL     (0x000c)
#define HANDLE_IMM_ALERT_SERVICE        (0x000d)
#define HANDLE_IMM_ALERT_SERVICE_END    (0x000f)
#define HANDLE_IMM_ALERT_LEVEL          (0x000f)
#define HANDLE_LINK_LOSS_SERVICE        (0x0010)
#define HANDLE_LINK_LOSS_SERVICE_END    (0x0012)
#define HANDLE_LINK_LOSS_ALERT_LEVEL    (0x0012)
#define HANDLE_GAIA_SERVICE             (0x0013)
#define HANDLE_GAIA_SERVICE_END         (0x001a)
#define HANDLE_GAIA_COMMAND_ENDPOINT    (0x0015)
#define HANDLE_GAIA_RESPONSE_ENDPOINT   (0x0017)
#define HANDLE_GAIA_RESPONSE_CLIENT_CONFIG (0x0018)
#define HANDLE_GAIA_DATA_ENDPOINT       (0x001a)
#define HANDLE_BATTERY_SERVICE1         (0x001b)
#define HANDLE_BATTERY_SERVICE1_END     (0x001f)
#define HANDLE_BATTERY_LEVEL1           (0x001d)
#define HANDLE_BATTERY_LEVEL_PRESENTATION1 (0x001e)
#define HANDLE_BATTERY_LEVEL_CLIENT_CONFIG1 (0x001f)
#define HANDLE_BATTERY_SERVICE2         (0x0020)
#define HANDLE_BATTERY_SERVICE2_END     (0x0024)
#define HANDLE_BATTERY_LEVEL2           (0x0022)
#define HANDLE_BATTERY_LEVEL_PRESENTATION2 (0x0023)
#define HANDLE_BATTERY_LEVEL_CLIENT_CONFIG2 (0x0024)
#define HANDLE_BATTERY_SERVICE3         (0x0025)
#define HANDLE_BATTERY_SERVICE3_END     (0xffff)
#define HANDLE_BATTERY_LEVEL3           (0x0027)
#define HANDLE_BATTERY_LEVEL_PRESENTATION3 (0x0028)
#define HANDLE_BATTERY_LEVEL_CLIENT_CONFIG3 (0x0029)

uint16 *GattGetDatabase(uint16 *len);
uint16 GattGetDatabaseSize(void);

#endif

/* End-of-File */
