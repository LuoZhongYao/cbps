/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server.c

DESCRIPTION
    Routines to handle the GATT Servers.
*/

#include "sink_gatt_server.h"

#include "sink_debug.h"
#include "sink_private.h"

#include <gatt_manager.h>


#ifdef GATT_ENABLED


/****************************************************************************/
bool gattServerConnectionAdd(u16 cid)
{
    /* Currently only ever one server connection */
    GATT_SERVER.cid = cid;
    return TRUE;
}


/****************************************************************************/
bool gattServerConnectionFindByCid(u16 cid)
{
    if (cid == GATT_SERVER.cid)
    {
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************/
bool gattServerConnectionRemove(u16 cid)
{
    if (cid == GATT_SERVER.cid)
    {
        GATT_SERVER.cid = 0;

        return TRUE;
    }

    return FALSE;
}


/****************************************************************************/
bool gattServerDisconnectAll(void)
{
    /* Call into gatt_manager to remove connections */
    if (GATT_SERVER.cid)
    {
        GattManagerDisconnectRequest(GATT_SERVER.cid);

        return TRUE;
    }

    return FALSE;
}


/***************************************************************************/
bool gattServerIsFullyDisconnected(void)
{
    return (GATT_SERVER.cid == 0);
}


#endif /* GATT_ENABLED */

