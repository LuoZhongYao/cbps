/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sink_gatt_server_dis.h

DESCRIPTION
    Routines to handle messages sent from the GATT Device Information Server Task.
    
NOTES

*/

#ifndef _SINK_GATT_SERVER_DIS_H_
#define _SINK_GATT_SERVER_DIS_H_


#ifndef GATT_ENABLED
#undef GATT_DIS_SERVER
#endif


#include <gatt_device_info_server.h>

#include <csrtypes.h>
#include <message.h>

#ifdef GATT_DIS_SERVER
#define sinkGattDeviceInfoServerGetSize() sizeof(gdiss_t)
#else
#define sinkGattDeviceInfoServerGetSize() 0
#endif

/*******************************************************************************
NAME
    sinkGattDeviceInfoServerInitialise
    
DESCRIPTION
    Initialise DIS server task.
    
PARAMETERS
    ptr - pointer to allocated memory to store server tasks rundata.
    
RETURNS
    TRUE if the DIS server task was initialised, FALSE otherwise.
*/
#ifdef GATT_DIS_SERVER
bool sinkGattDeviceInfoServerInitialise(u16 **ptr);
#else
#define sinkGattDeviceInfoServerInitialise(ptr) (TRUE)
#endif

#endif /* _SINK_GATT_SERVER_DIS_H_ */
