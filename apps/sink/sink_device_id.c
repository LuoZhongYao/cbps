/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_init.c

DESCRIPTION


NOTES

*/


/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_configmanager.h"
#include "sink_device_id.h"
#include "sink_config.h"

#include <ps.h>
#include <panic.h>
#include <sdp_parse.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_DI
#else
#endif

/* DEVICE_ID_PSKEY allows for PS configurable Device Id information, from
   CONFIG_DEVICE_ID, but requires permanent use of a memory pool to hold the
   SDP record.

   DEVICE_ID_CONST for Device Id information fixed at compile time, defined
   in sink_device_id.h.  This mechanism does not require use of a memory
   pool.
*/
#if defined DEVICE_ID_PSKEY || defined DEVICE_ID_CONST

#define EIR_DATA_SIZE_DEVICE_ID (0x08)  /* 4*u16 (does not include length or tag bytes) */
#define EIR_TAG_DEVICE_ID       (0x10)
#define EIR_TAG_SIZE_DEVICE_ID  (0x01)  /* Single byte EIR tag */
#define EIR_LEN_SIZE_DEVICE_ID  (0x01)  /* Single byte EIR record length */
#define EIR_FULL_SIZE_DEVICE_ID (EIR_LEN_SIZE_DEVICE_ID + EIR_TAG_SIZE_DEVICE_ID + EIR_DATA_SIZE_DEVICE_ID)

#define DATA_EL_UINT_16(value)  (((value)>>8)&0xFF),((value)&0xFF)

#define DEVICE_ID_OFFSET_VENDOR_ID_ATTR_VAL        18
#define DEVICE_ID_OFFSET_PRODUCT_ID_ATTR_VAL       24
#define DEVICE_ID_OFFSET_VERSION_ATTR_VAL          30
#define DEVICE_ID_OFFSET_VENDOR_ID_SOURCE_ATTR_VAL 41
#define DEVICE_ID_SERVICE_RECORD_SIZE              43

static const u8 device_id_service_record_template[ DEVICE_ID_SERVICE_RECORD_SIZE ] =
{
    /* DataElUint16, ServiceClassIDList(0x0001) */
    0x09, 0x00, 0x01,
        /* DataElSeq 3 bytes */
        0x35, 0x03,
            /* DataElUuid16, PnPInformation(0x1200) */
            0x19, 0x12, 0x00,

    /* DataElUint16, SpecificationID(0x0200) */
    0x09, 0x02, 0x00,
        /* DataElUint16, 'Device Id Specification v1.3'(0x0103) */
        0x09, 0x01, 0x03,

    /* DataElUint16, VendorID(0x0201) */
    0x09, 0x02, 0x01,
        /* DataElUint16, DEVICE_ID_VENDOR_ID */
        0x09, DATA_EL_UINT_16(DEVICE_ID_VENDOR_ID),

    /* DataElUint16, ProductID(0x0202) */
    0x09, 0x02, 0x02,
        /* DataElUint16, DEVICE_ID_PRODUCT_ID */
        0x09, DATA_EL_UINT_16(DEVICE_ID_PRODUCT_ID),

    /* DataElUint16, Version(0x0203) */
    0x09, 0x02, 0x03,
        /* DataElUint16, DEVICE_ID_BCD_VERSION */
        0x09, DATA_EL_UINT_16(DEVICE_ID_BCD_VERSION),

    /* DataElUint16, PrimaryRecord(0x0204) */
    0x09, 0x02, 0x04,
        /* DataElBool, True(0x01) */
        0x28, 0x01,

    /* DataElUint16, VendorIdSource(0x0205) */
    0x09, 0x02, 0x05,
        /* DataElUint16, DEVICE_ID_VENDOR_ID_SOURCE */
        0x09, DATA_EL_UINT_16(DEVICE_ID_VENDOR_ID_SOURCE)
};

#endif  /* INCLUDE_DEVICE_ID */

#ifdef ENABLE_PEER
/* DataElSeq(0x35), Length(0x03), 16-bit UUID(0x19), PnPInformation(0x1200) */
static const u8 device_id_search_pattern[] = {0x35, 0x03, 0x19, 0x12, 0x00};
/* DataElSeq(0x35), Length(0x09), DataElUint16(0x09), VendorIdSource(0x0205), DataElUint16(0x09), VendorID(0x0201), DataElUint16(0x09), ProductID(0x0202) */
static const u8 device_id_attr_list[] = {0x35, 0x09, 0x09, 0x02, 0x01, 0x09, 0x02, 0x02, 0x09, 0x02, 0x05};

bool RequestRemoteDeviceId (Task task, const bdaddr *bd_addr)
{
    if (!theSink.features.PeerUseDeviceId)
    {
        return FALSE;
    }

    ConnectionSdpServiceSearchAttributeRequest(task, bd_addr, 32, sizeof(device_id_search_pattern), device_id_search_pattern, sizeof(device_id_attr_list), device_id_attr_list);
    return TRUE;
}

remote_device CheckRemoteDeviceId (const u8 *attr_data, u16 attr_data_size)
{
    u32 vendorId, productId, vendorIdSrc;

    if ( SdpParseGetArbitrary(attr_data_size, attr_data, 0x0201, &vendorId) &&
         SdpParseGetArbitrary(attr_data_size, attr_data, 0x0202, &productId) &&
         SdpParseGetArbitrary(attr_data_size, attr_data, 0x0205, &vendorIdSrc) )
    {
#if defined DEVICE_ID_PSKEY
        sink_device_id device_id;

        LOGD("Remote Device Id = vid_src(%lx), vid(%lx), pid(%lx)\n", vendorIdSrc, vendorId, productId);

        if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
        {
            LOGD("Local Device Id = vid_src(%x), vid(%x), pid(%x)\n", device_id.vendor_id_source, device_id.vendor_id, device_id.product_id);
            if ( (vendorId == device_id.vendor_id) && (productId == device_id.product_id) && (vendorIdSrc == device_id.vendor_id_source) )
            {
                return remote_device_peer;
            }
        }
#elif defined DEVICE_ID_CONST
        LOGD("Remote Device Id data = vid_src(%xl), vid(%xl), pid(%xl)\n", vendorIdSrc, vendorId, productId);

        if ( (vendorId == DEVICE_ID_VENDOR_ID) && (productId == DEVICE_ID_PRODUCT_ID) && (vendorIdSrc == DEVICE_ID_VENDOR_ID_SOURCE) )
        {
            LOGD("Local Device Id = vid_src(%x), vid(%x), pid(%x)\n", DEVICE_ID_VENDOR_ID_SOURCE, DEVICE_ID_VENDOR_ID, DEVICE_ID_PRODUCT_ID);
            return remote_device_peer;
        }
#endif
    }

    return remote_device_nonpeer;
}
#endif

void RegisterDeviceIdServiceRecord( void )
{
#if defined DEVICE_ID_PSKEY

    sink_device_id device_id;

    if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
    {
        /* Construct service record from template */
        u8 *deviceIdServiceRecord = (u8 *)PanicNull( mallocPanic( sizeof(u8) * DEVICE_ID_SERVICE_RECORD_SIZE ) );
        memmove(deviceIdServiceRecord, device_id_service_record_template, DEVICE_ID_SERVICE_RECORD_SIZE);

        /* Update device specific information */
        deviceIdServiceRecord[DEVICE_ID_OFFSET_VENDOR_ID_ATTR_VAL+0] = (u8)(device_id.vendor_id >> 8);
        deviceIdServiceRecord[DEVICE_ID_OFFSET_VENDOR_ID_ATTR_VAL+1] = (u8)device_id.vendor_id;

        deviceIdServiceRecord[DEVICE_ID_OFFSET_PRODUCT_ID_ATTR_VAL+0] = (u8)(device_id.product_id >> 8);
        deviceIdServiceRecord[DEVICE_ID_OFFSET_PRODUCT_ID_ATTR_VAL+1] = (u8)device_id.product_id;

        deviceIdServiceRecord[DEVICE_ID_OFFSET_VERSION_ATTR_VAL+0] = (u8)(device_id.bcd_version >> 8);
        deviceIdServiceRecord[DEVICE_ID_OFFSET_VERSION_ATTR_VAL+1] = (u8)device_id.bcd_version;

        deviceIdServiceRecord[DEVICE_ID_OFFSET_VENDOR_ID_SOURCE_ATTR_VAL+0] = (u8)(device_id.vendor_id_source >> 8);
        deviceIdServiceRecord[DEVICE_ID_OFFSET_VENDOR_ID_SOURCE_ATTR_VAL+1] = (u8)device_id.vendor_id_source;

        /* Malloc'd block is passed to f/w and unmapped from VM space */
        ConnectionRegisterServiceRecord(&theSink.task, DEVICE_ID_SERVICE_RECORD_SIZE, deviceIdServiceRecord);
    }

#elif defined DEVICE_ID_CONST

    /* Register the Device Id SDP record from constant space */
    ConnectionRegisterServiceRecord(&theSink.task, DEVICE_ID_SERVICE_RECORD_SIZE, device_id_service_record_template);

#endif
}


u16 GetDeviceIdEirDataSize( void )
{
#if defined DEVICE_ID_PSKEY

    sink_device_id device_id;

    /* Device Id dependant on presence of data within PS */
    if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
    {
        return EIR_FULL_SIZE_DEVICE_ID;
    }
    else
    {
        return 0;
    }

#elif defined DEVICE_ID_CONST

    /* Device Id obtained from constant space */
    return EIR_FULL_SIZE_DEVICE_ID;

#else

    /* Device Id not being used, always return zero */
    return 0;

#endif
}


u16 WriteDeviceIdEirData( u8 *p )
{
#if defined DEVICE_ID_PSKEY

    sink_device_id device_id;

    /* Construct EIR record from PS data, if it exists */
    if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
    {
        *p++ = EIR_DATA_SIZE_DEVICE_ID + EIR_TAG_SIZE_DEVICE_ID;    /* Length byte contains size of tag and data only */
        *p++ = EIR_TAG_DEVICE_ID;
        *p++ = (u8)device_id.vendor_id_source;
        *p++ = (u8)(device_id.vendor_id_source>>8);
        *p++ = (u8)device_id.vendor_id;
        *p++ = (u8)(device_id.vendor_id>>8);
        *p++ = (u8)device_id.product_id;
        *p++ = (u8)(device_id.product_id>>8);
        *p++ = (u8)device_id.bcd_version;
        *p++ = (u8)(device_id.bcd_version>>8);

        /* Return EIR record size */
        return EIR_FULL_SIZE_DEVICE_ID;   /* Entire record size - length, tag and data */
    }
    else
    {
        /* No PS data, so EIR record not created */
        return 0;
    }

#elif defined DEVICE_ID_CONST

    /* Construct EIR record from constant data */
    *p++ = EIR_DATA_SIZE_DEVICE_ID + EIR_TAG_SIZE_DEVICE_ID;   /* Length byte contains size of tag and data only */
    *p++ = EIR_TAG_DEVICE_ID;
    *p++ = (u8)DEVICE_ID_VENDOR_ID_SOURCE;
    *p++ = (u8)(DEVICE_ID_VENDOR_ID_SOURCE>>8);
    *p++ = (u8)DEVICE_ID_VENDOR_ID;
    *p++ = (u8)(DEVICE_ID_VENDOR_ID>>8);
    *p++ = (u8)DEVICE_ID_PRODUCT_ID;
    *p++ = (u8)(DEVICE_ID_PRODUCT_ID>>8);
    *p++ = (u8)DEVICE_ID_BCD_VERSION;
    *p++ = (u8)(DEVICE_ID_BCD_VERSION>>8);

    /* Return EIR record size */
    return EIR_FULL_SIZE_DEVICE_ID;    /* Entire record size - length, tag and data */

#else

    /* Device Id not being used, always return zero */
    return 0;

#endif
}


/****************************************************************************
NAME
    CheckEirDeviceIdData

DESCRIPTION
    Helper function to check if expected EIR data is in the inquiry results
*/
#ifdef ENABLE_PEER
bool CheckEirDeviceIdData (u16 size_eir_data, const u8 *eir_data)
{
    while (size_eir_data >= EIR_FULL_SIZE_DEVICE_ID)
    {
        u16 eir_record_size = eir_data[0] + 1;   /* Record size in eir_data[0] does not include length byte, just tag and data size */

        if (eir_data[1] == EIR_TAG_DEVICE_ID)
        {
            u16 vendorIdSource = (eir_data[3]<<8) + eir_data[2];
            u16 vendorId = (eir_data[5]<<8) + eir_data[4];
            u16 productId = (eir_data[7]<<8) + eir_data[6];

            LOGD("EIR Device Id data = vid_src(%x), vid(%x), pid(%x)\n",vendorIdSource,vendorId,productId);

#if defined DEVICE_ID_PSKEY
            {
                sink_device_id device_id;

                if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
                {
                    if ((vendorIdSource==device_id.vendor_id_source) && (vendorId==device_id.vendor_id) && (productId==device_id.product_id))
                    {
                        return TRUE;
                    }
                }
            }
#elif defined DEVICE_ID_CONST
            {
                if ((vendorIdSource==DEVICE_ID_VENDOR_ID_SOURCE) && (vendorId==DEVICE_ID_VENDOR_ID) && (productId==DEVICE_ID_PRODUCT_ID))
                {
                    return TRUE;
                }
            }
#else

            /* Device Id not being used, always return true */
            return TRUE;

#endif
        }

        if (size_eir_data > eir_record_size)
        {
            size_eir_data -= eir_record_size;
            eir_data += eir_record_size;
        }
        else
        {
            size_eir_data = 0;
        }
    }

    /* Device Id data not present or did not match expected values */
    return FALSE;
}

void ValidatePeerUseDeviceIdFeature(void)
{
    /* Check if the Vendor Id is set to 0xFFFF, if yes then unset the theSink.features.PeerUseDeviceId feature bit*/

#if defined DEVICE_ID_PSKEY
    sink_device_id device_id;
    if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
    {
        if((device_id.vendor_id == 0xFFFF) && theSink.features.PeerUseDeviceId )
        {
            /*Reset the theSink.features.PeerUseDeviceId feature bit */
            theSink.features.PeerUseDeviceId = 0;
        }
    }
#elif defined DEVICE_ID_CONST
    if ((DEVICE_ID_VENDOR_ID == 0xFFFF) && theSink.features.PeerUseDeviceId)
    {
        /*Reset the theSink.features.PeerUseDeviceId feature bit */
        theSink.features.PeerUseDeviceId = 0;
    }
#endif

}

#endif


void GetDeviceIdFullVersion(u16 *buffer)
{
#if defined DEVICE_ID_PSKEY

    sink_device_id device_id;

    /* Ensure buffer contents are zero'd */
    memset(buffer, 0, SINK_DEVICE_ID_SW_VERSION_SIZE);

    /* Construct EIR record from PS data, if it exists */
    if (ConfigRetrieve(CONFIG_DEVICE_ID, &device_id, sizeof(device_id)) >= SINK_DEVICE_ID_STRICT_SIZE)
    {
        memmove(buffer, device_id.sw_version, SINK_DEVICE_ID_SW_VERSION_SIZE);
    }

#else

    /* Ensure buffer contents are zero'd */
    memset(buffer, 0, SINK_DEVICE_ID_SW_VERSION_SIZE);

    /* PS key will only contain the s/w version number, which is up to 4 words in length */
    ConfigRetrieve(CONFIG_DEVICE_ID, buffer, SINK_DEVICE_ID_SW_VERSION_SIZE);

#endif
}

