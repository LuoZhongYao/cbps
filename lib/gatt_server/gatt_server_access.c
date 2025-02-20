/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_gatt_server_access.c

DESCRIPTION
    The GATT "GATT Server" access routines.
    
NOTES

*/


/* Firmware headers */
#include <csrtypes.h>
#include <stdlib.h>

/* External lib headers */
#include "gatt_manager.h"
#include "gatt.h"

/* "GATT Server" headers */
#include "gatt_server.h"
#include "gatt_server_access.h"
#include "gatt_server_api.h"
#include "gatt_server_db.h"


/*******************************************************************************
NAME
    handleGattServiceAccessRequest
    
DESCRIPTION
    Handle GATT_MANAGER_SERVER_ACCESS_IND message when it is for the Service of the
    "GATT Service"
    
PARAMETERS
    gatt_task   The "GATT Server" task.
    ind         Pointer to a GATT_MANAGER_SERVER_ACCESS_IND message that needs to be handled.
    
RETURN
    TRUE if the message was handled, FALSE otherwise.
*/
static bool handleGattServiceAccessRequest(Task gatt_task, GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    if (gatt_task && ind)
    {
        if (ind->flags & ATT_ACCESS_READ)
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_success, 0, 0);
        }
        else if (ind->flags & ATT_ACCESS_WRITE)
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_write_not_permitted, 0, 0);
        }
        else
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_request_not_supported, 0, 0);
        }
        return TRUE;
    }
    return FALSE;
}


/*******************************************************************************
NAME
    handleGattServiceChangedAccessRequest
    
DESCRIPTION
    Handle GATT_MANAGER_SERVER_ACCESS_IND message when it is for the Service Changed 
    characteristic of the "GATT Service"
    
PARAMETERS
    gatt_task   The "GATT Server" task.
    ind         Pointer to a GATT_MANAGER_SERVER_ACCESS_IND message that needs to be handled.
    
RETURN
    TRUE if the message was handled, FALSE otherwise.
*/
static bool handleGattServiceChangedAccessRequest(Task gatt_task, GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    if (ind && gatt_task)
    {
        if (ind->flags & ATT_ACCESS_READ)
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_read_not_permitted, 0, 0);
        }
        else if (ind->flags & ATT_ACCESS_WRITE)
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_write_not_permitted, 0, 0);
        }
        else
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_request_not_supported, 0, 0);
        }
        return TRUE;
    }
    return FALSE;
}


/*******************************************************************************
NAME
    handleGattServiceChangedCCfgAccessRequest
    
DESCRIPTION
    Handle GATT_MANAGER_SERVER_ACCESS_IND message when it is for the Service Changed 
    client configuration descriptor of the "GATT Service"
    
PARAMETERS
    gatt_task   The "GATT Server" task.
    ind         Pointer to a GATT_MANAGER_SERVER_ACCESS_IND message that needs to be
                handled.
    
RETURN
    TRUE if the message was handled, FALSE otherwise.
*/
static bool handleGattServiceChangedCCfgAccessRequest(Task gatt_task, GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    if (ind && gatt_task)
    {
        GGATTS *gatt_server = (GGATTS*)gatt_task;

        /* Pass the read request to the application, the application must respond with
         * GattServerReadClientConfigResponse()
         * */
        if (ind->flags & ATT_ACCESS_READ)
        {
            gattServerSendReadClientConfigInd(gatt_server->app_task, ind->cid, ind->handle);
        }
        else if (ind->flags & ATT_ACCESS_WRITE)
        {
            if(ind->size_value == CLIENT_CONFIG_ACCESS_SIZE)
            {
                u16 value = (ind->value[0] & 0xFF) | ((ind->value[1] << 8) & 0xFF00);
                gattServerSendWriteClientConfigInd(gatt_server->app_task, ind->cid, value);
                GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_success, 0, NULL);
            }
            else
            {
            	GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_invalid_length, 0, NULL);
            }
        }
        else
        {
            GattManagerServerAccessResponse(gatt_task, ind->cid, ind->handle, gatt_status_request_not_supported, 0, NULL);
        }
        return TRUE;
    }
    return FALSE;
}


/******************************************************************************/
bool gattServerHandleGattManagerAccessInd(Task gatt_task, GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    if (ind)
    {
        switch(ind->handle)
        {
            case HANDLE_GATT_SERVICE:
            {
                return handleGattServiceAccessRequest(gatt_task, ind);
            }
            break;
            case HANDLE_GATT_SERVICE_CHANGED:
            {
                return handleGattServiceChangedAccessRequest(gatt_task, ind);
            }
            break;
            case HANDLE_GATT_SERVICE_CHANGED_CLIENT_CONFIG:
            {
                return handleGattServiceChangedCCfgAccessRequest(gatt_task, ind);
            }
            break;
            default:
            {
                /* ERROR */
            }
        }
    }
    return FALSE;
}

/******************************************************************************/
bool GattServerReadClientConfigResponse(GGATTS *gatt_server, u16 cid, u16 handle, u16 config)
{
    u8 config_resp[CLIENT_CONFIG_ACCESS_SIZE];

    config_resp[0] = config & 0xFF;
    config_resp[1] = (config >> 8) & 0xFF;

    PanicNull((void*)gatt_server);
    return GattManagerServerAccessResponse(&gatt_server->lib_task, cid,
                                           handle, gatt_status_success,
                                           CLIENT_CONFIG_ACCESS_SIZE, config_resp);
}
