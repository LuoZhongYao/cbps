/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_access.c   

DESCRIPTION
    GATT Access IND/RES message scenario functions.

NOTES

*/

/***************************************************************************
    Header Files
*/

#include "gatt.h"
#include "gatt_private.h"

#include <string.h>
#include <stdlib.h>
#include <vm.h>

/****************************************************************************
NAME
    gattHandleAttAccessInd

DESCRIPTION
    Handles the ATT_ACCESS_IND message (non-blocking).

RETURNS

*/
void gattHandleAttAccessInd(ATT_ACCESS_IND_T *ind)
{
    u8 *value = 0;
    if (ind->size_value)
        value = (u8 *) VmGetPointerFromHandle(ind->value);

    if ( gattCidIsValid(ind->cid) )
    {
        MAKE_GATT_MESSAGE_WITH_VALUE(
            GATT_ACCESS_IND, 
            ind->size_value,
            value
            ); 

        message->cid = ind->cid;
        message->handle = ind->handle;
        message->flags = ind->flags;
        message->offset = ind->offset;

        MessageSend(gattGetCidMappedTask(ind->cid), GATT_ACCESS_IND, message);
    }
    else
        GATT_DEBUG_INFO(("gattHandleAttAccessInd: cid 0x%04x not found\n", ind->cid));

    free(value);
}

/****************************************************************************
NAME
    GattAccessResponse

DESCRIPTION
    API function

RETURNS

*/
void GattAccessResponse(u16 cid, u16 handle, u16 result, u16 size_value, const u8 *value)
{
    MAKE_GATT_MESSAGE_WITH_VALUE(
        GATT_INTERNAL_ACCESS_RES, 
        size_value,
        value
        );

    message->cid = cid;
    message->handle = handle;
    message->result = result;

    MessageSend(gattGetTask(), GATT_INTERNAL_ACCESS_RES, message);
}

/****************************************************************************
NAME
    gattHandleInternalAccessRes
           
DESCRIPTION
   Handle the internal Access Reponse. 

RETURN

*/
void gattHandleInternalAccessRes(GATT_INTERNAL_ACCESS_RES_T *res)
{
    if (gattCidIsValid(res->cid))
    {
        MAKE_ATT_PRIM(ATT_ACCESS_RSP);
           
        if (res->size_value)
        {
            u8 *value = (u8 *)PanicUnlessMalloc(res->size_value);
            memmove(value, res->value, res->size_value);
            prim->value = (u8 *)VmGetHandleFromPointer(value);
        }
        else
        {
            prim->value = 0;   
        }

        prim->cid = res->cid;
        prim->handle = res->handle;
        prim->result = res->result;
        prim->size_value = res->size_value;

        VmSendAttPrim(prim);

    }
    else
        GATT_DEBUG(("gattHandleInternalAccessRes: Invalid CID 0x%x\n", res->cid));
}


