/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0
*/

#include <csrtypes.h>
#include <stdlib.h>
#include <string.h>

#include "gatt_manager.h"
#include "gatt_manager_internal.h"
#include "gatt_manager_handler.h"
#include "gatt_manager_data.h"


/******************************************************************************
 *                      GATT MANAGER PUBLIC API                               *
 ******************************************************************************/

bool GattManagerInit(Task task)
{
    if (NULL == task)
    {
        return FALSE;
    }

    gattManagerDataInit(gattManagerMessageHandler, task);
    gattManagerDataInitialisationState_Registration();
    return TRUE;
}

void GattManagerDeInit(void)
{
    gattManagerDataDeInit();
}

bool GattManagerRegisterConstDB(const u16* db_ptr, u16 size)
{
    if (!gattManagerDataIsInit() ||
        gattManagerDataGetInitialisationState() != gatt_manager_initialisation_state_registration ||
        gattManagerDataGetDB() != NULL ||
        NULL == db_ptr ||
        0 == size)
    {
        return FALSE;
    }

    gattManagerDataSetConstDB(db_ptr, size);
    return TRUE;
}

void GattManagerRegisterWithGatt(void)
{
    if (!gattManagerDataIsInit())
    {
        registerWithGattCfm(gatt_manager_status_not_initialised);
    }

    if (gattManagerDataGetInitialisationState() != gatt_manager_initialisation_state_registration)
    {
        registerWithGattCfm(gatt_manager_status_wrong_state);
    }

    gattManagerDataInitialisationState_Registering();
    /* When using the const DB discard the const qualifier as the GattInit API
       doesn't support it. We could have discarded the qualifier earlier but it
       makes more sense to discard it at the point where we are forced to do
       so. */
    GattInit(gattManagerDataGetTask(), gattManagerDataGetDBSize(),
             (u16 *)gattManagerDataGetDB());
}

void GattManagerDisconnectRequest(u16 cid)
{
    if (gattManagerDataIsInit())
    {
        GattDisconnectRequest(cid);
    }
}

void GattManagerIndicationResponse(u16 cid)
{
    if (gattManagerDataIsInit())
    {
        GattIndicationResponse(cid);
    }
}
