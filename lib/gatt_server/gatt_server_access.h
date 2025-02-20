/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_server_access.h
    
DESCRIPTION
    Handle GATT Data access requests for requests made to the GATT database
    owned by the "GATT Server" library.
*/


#ifndef GATT_SERVER_ACCESS_H_
#define GATT_SERVER_ACCESS_H_


/* Firmware includes */
#include <csrtypes.h>

/* External lib headers */
#include "gatt_manager.h"


#define    CLIENT_CONFIG_ACCESS_SIZE    2

/*******************************************************************************
NAME
    gattServerHandleGattManagerAccessInd
    
DESCRIPTION
    Handle GATT_ACCESS_IND messages.
    
PARAMETERS
    gatt_task   The "GATT Server" task.
    ind         Pointer to a GATT_ACCESS_IND message that needs to be handled.
    
RETURN
    TRUE if the message was handled, FALSE otherwise.
*/
bool gattServerHandleGattManagerAccessInd(Task gatt_task, GATT_MANAGER_SERVER_ACCESS_IND_T *ind);


#endif
