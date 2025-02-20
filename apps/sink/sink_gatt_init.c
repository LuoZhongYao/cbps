/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_init.c

DESCRIPTION
    Contains GATT initialisation routines.
*/

#include "sink_gatt_db.h"
#include "sink_gatt_init.h"

#include "sink_debug.h"
#include "sink_gatt_client_battery.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_device.h"
#include "sink_gatt_manager.h"
#include "sink_gatt_server.h"
#include "sink_gatt_server_battery.h"
#include "sink_gatt_server_tps.h"
#include "sink_gatt_server_lls.h"
#include "sink_gatt_server_ias.h"
#include "sink_gatt_server_gap.h"
#include "sink_gatt_server_gatt.h"
#include "sink_gatt_client_ias.h"
#include "sink_private.h"

#include <gatt_server.h>
#include <gatt_gap_server.h>
#include <gatt_battery_server.h>
#include <gatt_manager.h>
#include <connection.h>

#include <csrtypes.h>


#ifdef GATT_ENABLED


#ifdef DEBUG_GATT
#else
#endif

extern const u16 gattDatabase[];

/*******************************************************************************
NAME
    calculateOptionalGattServerSize

DESCRIPTION
    Calculates the size of memory required to hold the optional GATT servers.

PARAMETERS
    None

RETURNS
    The size of memory required to hold the optional GATT servers.
*/
static u16 calculateOptionalGattServerSize(void)
{
    u16 size = 0;

    /* Get size used by battery server */
    size += sinkGattBatteryServerCalculateSize();

    /* Calculates size of other optional GATT servers here */
    size += sinkGattLinkLossServerGetSize();
    size += sinkGattTxPowerServerGetSize();
    size += sinkGattImmAlertServerGetSize();
    return size;
}


/*******************************************************************************
NAME
    calculateCompleteGattServerSize

DESCRIPTION
    Calculates the size of memory required to hold all the GATT servers.

PARAMETERS
    None

RETURNS
    The size of memory required to hold all the GATT servers.
*/
static u16 calculateCompleteGattServerSize(void)
{
    u16 size = 0;
    LOGD("Calculate memory size for server tasks:\n");

    /* GATT Server must always be included */
    size += sinkGattServerGetSize();
    LOGD("+GATT=[%d]\n", size);

    /* GAP Server must always be included */
    size += sinkGapServerGetSize();
    LOGD("+GAP=[%d]\n", size);

    /* DIS Server should be always included */

    /* Add any optional tasks that have been requested... */
    size += calculateOptionalGattServerSize();
    LOGD("+Optional=[%d]\n", size);

    return size;
}


/*******************************************************************************
NAME
    initialiseOptionalServerTasks

DESCRIPTION
    Initialise each of the optional server tasks that have been configured.

PARAMETERS
    ptr     pointer to allocated memory to store server tasks rundata.

RETURNS
    TRUE if all requested server tasks were initialised, FALSE otherwise.
*/
static bool initialiseOptionalServerTasks(u16 **ptr)
{
    if (ptr)
    {
        if (!sinkGattBatteryServerInitialise(ptr))
        {
            return FALSE;
        }

        if(sinkGattLinkLossServerInitialiseTask(ptr) != TRUE)
        {
            return FALSE;
        }

        /* Initialize the Tx Power server */
        if(sinkGattTxPowerServerInitialiseTask(ptr) != TRUE)
        {
            return FALSE;
        }

        /* Initialize the Immediate Alert server */
        if (!sinkGattImmAlertServerInitialise(ptr))
        {
            return FALSE;
        }

        /* Initialise GAIA server */
        gaiaGattServerInitialise();

        /* Initialise other optional GATT Servers here */

        /* All requested server tasks were registered successfully */
        return TRUE;
    }

    return FALSE;
}


/*******************************************************************************
NAME
    initialiseGattServers

DESCRIPTION
    Prepare the GATT Manager for server registration, then Initialise each of
    the GATT Server tasks that have been requested and finally register the
    GATT database for the device.

PARAMETERS
    None

RETURNS
    TRUE if the initialisation was successful, FALSE otherwise.
*/
static bool initialiseGattServers(void)
{
    u16 server_malloc_size = calculateCompleteGattServerSize();
    GATT_SERVER.servers_ptr = malloc(server_malloc_size);
    if (GATT_SERVER.servers_ptr)
    {
        u16 **ptr = (u16**)&GATT_SERVER.servers_ptr;

        /* GATT Service MUST always be included as per the Bluetooth spec */
        if (sinkGattServerInitialiseTask(ptr))
        {
            /* GAP Server must always be included as per the Bluetooth spec */
            if (sinkGattGapServerInitialiseTask(ptr))
            {
                /* DIS Server should be always included */

                /* Initialise Optional services */
                if (initialiseOptionalServerTasks(ptr))
                {
                    /* All server tasks have been initialised */
                    return TRUE;
                }
            }
        }

        /* Free allocated memory used to store rundata for the GATT Server libraries */
        free(GATT_SERVER.servers_ptr);
    }

    return FALSE;
}


/*******************************************************************************
NAME
    initialiseGattWithServers

DESCRIPTION
    Function to initialise GATT for the device when server roles have been
    requested.

PARAMETERS
    None

RETURNS
    TRUE if the initialisation request was successful, FALSE otherwise.
*/
static bool initialiseGattWithServers(void)
{
    LOGD("Initialise GATT Manager - with servers\n");
    if (GattManagerInit(sinkGetBleTask()))
    {
        if (GattManagerRegisterConstDB(&gattDatabase[0], GattGetDatabaseSize()))
        {
            if (initialiseGattServers())
            {
                GattManagerRegisterWithGatt();

                return TRUE;
            }
        }
    }

    return FALSE;
}


/*******************************************************************************
NAME
    initialiseGattClient

DESCRIPTION
    Function to initialise GATT for client role.

PARAMETERS
    None

RETURNS
    None
*/
static void initialiseGattClient(void)
{
    /* Init of client services */
    gattBatteryClientInit();
    sinkGattIasClientInit();
    sinkGattHidClientInit();
}


/******************************************************************************/
bool sinkGattInitInitialiseDevice(void)
{
    /*Init sink GATT database*/
    memset(&GATT, 0, sizeof(gatt_data_t));

    /* Client role init */
    initialiseGattClient();

    /* Server role init */
    return initialiseGattWithServers();
}
#endif /* GATT_ENABLED */

