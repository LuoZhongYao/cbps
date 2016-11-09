/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble_advertising.c

DESCRIPTION
    BLE Advertising functionality
*/

#include "sink_ble_advertising.h"

#include "sink_gatt_device.h"
#include "sink_gatt_server_battery.h"
#include "sink_private.h"
#include "sink_debug.h"
#include "sink_development.h"
#include "sink_utils.h"

#include <connection.h>
#include <gatt.h>

#include <csrtypes.h>
#include <stdlib.h>
#include <string.h>


#ifdef GATT_ENABLED


/* Macro for BLE AD Data Debug */
#ifdef DEBUG_BLE
#include <stdio.h>
#define BLE_AD_INFO(x) DEBUG(x)
#define BLE_AD_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
#else
#define BLE_AD_INFO(x)
#define BLE_AD_ERROR(x)
#endif


/******************************************************************************/
void bleStopAdvertising(void)
{
    BLE_AD_INFO(("BLE : Stop AD\n"));
    ConnectionDmBleSetAdvertiseEnable(FALSE);
}

/*******************************************************************************
NAME
    setupFlagsAdData

RETURN
     size of flag block

DESCRIPTION
    Helper function to setup the advertising data, flags (MUST be advertised)

PARAMETERS
    ad_data_ptr  Pointer to the advertising data.

RETURNS
    void
*/
static u16 setupFlagsAdData(u8 *ad_data, adv_discoverable_mode_t mode)
{
    #define FLAGS_AD_DATA_LENGTH 0x3

    u16 flags = 0;

    if (mode == adv_discoverable_mode_general)
        flags |= BLE_FLAGS_GENERAL_DISCOVERABLE_MODE;
    else if (mode == adv_discoverable_mode_limited)
        flags |= BLE_FLAGS_LIMITED_DISCOVERABLE_MODE;

    /* Setup the flags ad data */
    ad_data[0] = 0x02;
    ad_data[1] = ble_ad_type_flags;
    ad_data[2] = flags;
#ifdef DEBUG_BLE
    {
        u16 counter;
        BLE_AD_INFO(("AD Data: flags = ["));
        for (counter=0; counter < FLAGS_AD_DATA_LENGTH; counter++)
        {
            BLE_AD_INFO(("%02x,", ad_data[counter]));
        }
        BLE_AD_INFO(("]\n"));
    }
#endif

	return FLAGS_AD_DATA_LENGTH;
}


/*******************************************************************************
NAME
    updateServicesAdData

DESCRIPTION
    Helper function to update the services UUID enabled in advertisement data

PARAMETERS
    ad_data_ptr      Pointer to the advertising data.
    counter_ptr       Pointer to start adding services from this position in the ad_data memory
    num_services    Number of services enabled

RETURNS
    void
*/
static void updateServicesAdData(u8* ad_data_ptr, u16 *counter_ptr , u16 num_services)
{
    if (sinkGattBatteryServiceEnabled() && num_services)
    {
        BLE_AD_INFO(("Battery Service"));
        ad_data_ptr[*counter_ptr] = (GATT_SERVICE_UUID_BATTERY_SERVICE & 0xFF);
        (*counter_ptr)++;
        ad_data_ptr[*counter_ptr] = (GATT_SERVICE_UUID_BATTERY_SERVICE >> 8);
        (*counter_ptr)++;
        num_services--;
    }
}


/*******************************************************************************
NAME
    setupServicesAdData

DESCRIPTION
    Helper function to setup the services advertisement data

RETURN
    the offset of the last byte to be written in.

PARAMETERS
    ad_data_ptr      Pointer to the advertising data.
    num_free_octets  Number of free bytes in the advertising data.

RETURNS
    void
*/
static u16 setupServicesAdData(u8* ad_data, u16 ad_data_index )
{
    /* How many services have been defined? */
    u16 counter = ad_data_index;
    u16 num_services = 0;
    u16 ad_size;
    u8 ad_tag;
    u16 num_free_octets = MAX_AD_DATA_SIZE_IN_OCTETS - ad_data_index;

    /* Check if battery service enabled and add to total number of services enabled */
    if (sinkGattBatteryServiceEnabled())
    {
        num_services++;
    }

    if (num_services)
    {
         /* Is there enough room to store the complete list of services defined for the device? */
         if ( (AD_DATA_HEADER_SIZE + (num_services*OCTETS_PER_SERVICE)) <= num_free_octets )
         {
               /* Advertise complete list */
               ad_tag = ble_ad_type_complete_uuid16;

               /* Allocate enough memory to store the complete list of services defined for the device */
               ad_size = (AD_DATA_HEADER_SIZE + (num_services*OCTETS_PER_SERVICE));
         }
         else
         {
               /* Advertise incomplete list (only advertise the first service based on alpabetical priority) */
               ad_tag = ble_ad_type_more_uuid16;

               /* Allocate enough memory to store the services defined for the device */
               ad_size = (AD_DATA_HEADER_SIZE + OCTETS_PER_SERVICE);
         }

         /* Setup AD data for the services */
         ad_data[ad_data_index] = ad_size -1;    /* Do not count the 'length' value; length is AD_TAG + AD_DATA */
         ad_data[ad_data_index+1] = ad_tag ;     /* AD_TAG (either complete or incomplete list of u16 UUIDs */

         /* Start adding services from this position in the ad_data memory */
         counter = AD_DATA_HEADER_SIZE + ad_data_index;

         /* Depending on which services have been defined, build the AD data */
         BLE_AD_INFO(("AD Data: services num=%d, included=", num_services));

         /* Add UUID of enabled services to advertising list */
         updateServicesAdData(ad_data, &counter, num_services);

         BLE_AD_INFO(("\n"));

         #ifdef DEBUG_BLE
         {
               u16 idx;
               BLE_AD_INFO(("AD Data: services = ["));
               for (idx=ad_data_index; idx < counter; idx++)
               {
                      BLE_AD_INFO(("%02x,", ad_data[idx]));
               }
               BLE_AD_INFO(("]\n"));
         }
         #endif
    }
    /* return the advertising data counter as next available index based on configured number of services */
    return counter;
}


/*******************************************************************************
NAME
    setupLocalNameAdvertisingData

RETURN
    the offset of the last byte to be written in.

DESCRIPTION
    Helper function to setup advertising data to advertise the devices local
    name used by remote devices scanning for BLE services
*/
static u16 setupLocalNameAdvertisingData(u8 *ad_data, u16 ad_index, u16 size_local_name, const u8 * local_name)
{
	u16 ad_data_free_space = MAX_AD_DATA_SIZE_IN_OCTETS - ad_index;

    /* Is there a local name to be advertised? If so, is there enough free space in AD Data to advertise it? */
    if ( (0 == size_local_name) || ((AD_DATA_HEADER_SIZE+1) >= ad_data_free_space) )
    {
        return 0;
    }
    else if ((AD_DATA_HEADER_SIZE + size_local_name) <= ad_data_free_space)
    {
        /* advertise the name in full */
        ad_data[ad_index] = size_local_name + 1;
		ad_data[ad_index+1] = ble_ad_type_complete_local_name;
	}
    else
    {
        /* Can advertise a shortened local name */
        ad_data[ad_index] = ad_data_free_space - AD_DATA_HEADER_SIZE +1;
		ad_data[ad_index+1] = ble_ad_type_shortened_local_name;
	}

    /* Setup the local name advertising data */
    memmove( ad_data + ad_index + AD_DATA_HEADER_SIZE, local_name, ad_data[ad_index] -1 );
#ifdef DEBUG_BLE
    {
        u16 i;
        BLE_AD_INFO(("AD Data: local name=[%02x,%02x,", ad_data[ad_index], ad_data[ad_index+1]));
        for (i=0; i<ad_data[ad_index]; i++)
        {
            BLE_AD_INFO(("%c,", ad_data[ad_index+AD_DATA_HEADER_SIZE+i-1]));
        }
        BLE_AD_INFO(("]\n"));
    }
#endif
    return ad_data[ad_index] + AD_DATA_HEADER_SIZE + ad_index -1 ;
}

/******************************************************************************/
void bleSetupAdvertisingData(u16 size_local_name, const u8 *local_name, adv_discoverable_mode_t mode)
{
    u16 ad_data_index = 0;
    u8 *ad_data = malloc( MAX_AD_DATA_SIZE_IN_OCTETS );

	if( NULL != ad_data )
	{
    	/* Setup the flags advertising data */
        ad_data_index = setupFlagsAdData(ad_data, mode);

		/* Setup the services advertising data */
		ad_data_index = setupServicesAdData(ad_data, ad_data_index);

    	/* Setup the local name advertising data */
    	ad_data_index = setupLocalNameAdvertisingData(ad_data, ad_data_index, size_local_name, local_name );

    	/* Register AD data with the Connection library & Tidy up allocated memory*/
    	ConnectionDmBleSetAdvertisingDataReq(ad_data_index, ad_data);

        /* free the buffer after use */
		free (ad_data);
	}
	else
	{
		/* Panic, not enough room to BLE Advertise */
	}
}

/******************************************************************************/
void bleHandleSetAdvertisingData(CL_DM_BLE_SET_ADVERTISING_DATA_CFM_T * cfm)
{
    ble_gap_event_t event;

    BLE_AD_INFO(("CL_DM_BLE_SET_ADVERTISING_DATA_CFM [%x]\n", cfm->status));

    if (cfm->status != success)
    {
        BLE_AD_ERROR(("  Failed!\n"));
    }

    /* Send GAP event after set of advertising data */
    event.id = ble_gap_event_set_advertising_complete;
    event.args = NULL;
    sinkBleGapEvent(event);
}

#endif /* GATT_ENABLED */

