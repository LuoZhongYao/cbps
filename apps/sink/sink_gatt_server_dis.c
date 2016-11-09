/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_dis.c

DESCRIPTION
    Routines to handle messages sent from the GATT Device Information Server Task.
*/


/* Firmware headers */
#include <csrtypes.h>
#include <message.h>

/* Application headers */
#include "sink_gatt_db.h"
#include "sink_gatt_server_dis.h"

#include "sink_debug.h"
#include "sink_gatt_server.h"
#include "sink_private.h"

#ifdef GATT_DIS_SERVER

/* The device information is to be made PS configurable, for now
    this is retained as static due to unavailability of free User PS key.
*/
#define DEVICE_INFO_MANUFACTURER_NAME_STRING    "CSR"

#ifdef DEBUG_GATT
#else
#endif

static gatt_dis_init_params_t dis_init_params;

/*******************************************************************************/
static bool sinkGattGetDeviceInfoParams(void)
{
    dis_init_params.dis_strings= malloc(sizeof(gatt_dis_strings_t));

    if(dis_init_params.dis_strings != NULL)
    {
        /* Initialize device manufacturer name string
            Note: this is currently made static however this needs to be made configurable using a PSKEY.
        */
        dis_init_params.dis_strings->manufacturer_name_string = DEVICE_INFO_MANUFACTURER_NAME_STRING;
        dis_init_params.dis_strings->model_num_string = NULL;
        dis_init_params.dis_strings->serial_num_string = NULL;
        dis_init_params.dis_strings->hw_revision_string = NULL;
        dis_init_params.dis_strings->fw_revision_string = NULL;
        dis_init_params.dis_strings->sw_revision_string = NULL;
        return TRUE;
    }
    /* Failed to allocate memory */
    LOGD("GATT Device Info Server failed to allocate memory\n");
    return FALSE;
}

/*******************************************************************************/
bool sinkGattDeviceInfoServerInitialise(u16 **ptr)
{
    if (ptr)
    {
        gatt_dis_status_t dis_status;

        /* Read the device information service to be initialized */
        if(sinkGattGetDeviceInfoParams())
        {
            dis_status = GattDeviceInfoServerInit(sinkGetBleTask(), (gdiss_t*)*ptr, &dis_init_params,
                                    HANDLE_DEVICE_INFORMATION_SERVICE,
                                    HANDLE_DEVICE_INFORMATION_SERVICE_END);

            if (dis_status == gatt_dis_status_success)
            {
                LOGD("GATT Device Info Server initialised\n");
                /* The size of DISS is also calculated and memory is alocated.
                 * So advance the ptr so that the next Server while initializing.
                 * shall not over write the same memory */
               *ptr += sizeof(gdiss_t);
                return TRUE;
            }
            /* Failed to initialize Device Information server */
            LOGD("GATT Device Info Server init failed [%x]\n", dis_status);
            /* Free the allocated memory */
            free(dis_init_params.dis_strings);
        }
    }
    return FALSE;
}

#endif /* GATT_DIS_SERVER */
