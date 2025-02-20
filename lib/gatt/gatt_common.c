/*************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    gatt_common.c

DESCRIPTION
    Functions common to multiple modules.

NOTES
*/

#include "gatt_private.h"
#include <string.h>

#if (GATT_FEATURES & (GATT_PRIMARY_DISCOVERY | \
                      GATT_RELATIONSHIP_DISCOVERY))
static const u32 att_base_uuid[] = {
        0x00001000,
        0x80000080,
        0x5f9b34fb,
    };

/*************************************************************************
NAME    
    gatt_get_att_uuid
    
DESCRIPTION
    This function is used to extract UUID and UUID type from ATT little
    endian data stream.
    
RETURNS
    
*/
void gatt_get_att_uuid(gatt_uuid_t *uuid,
                       gatt_uuid_type_t *uuid_type,
                       u16 size_value,
                       const u8 *value)
{
    u16 i;
    
    switch (size_value)
    {
        case 2: /* 16-bit UUID */
        case 4: /* 32-bit UUID */
            uuid[0] = (((u16)value[1]) << 8) | value[0];
            *uuid_type = gatt_uuid16;
            if ( size_value == 4 )
            	/* 32-bit UUID */
            {
                uuid[0] |= (((u32)value[3]) << 24) |
                        (((u32)value[2]) << 16);
                *uuid_type = gatt_uuid32;
            }
            uuid[1] = att_base_uuid[0];
            uuid[2] = att_base_uuid[1];
            uuid[3] = att_base_uuid[2];
            break;

        case 16:
            for (i = 0; i < 4; i++)
            {
                uuid[3 - i] = value[0] | (value[1] << 8) |
                    ((u32)value[2] << 16) |
                    ((u32)value[3] << 24);
                value += 4;
            }
            if (!memcmp(uuid + 1, att_base_uuid, 3 * sizeof(u32)))
                /* Base UUID is same as Bluetooth base UUID */
            {
                /* we got 16 bytes of data but the UUID is still either 16-bit
                 * or 32-bit attribute UUID */
                *uuid_type = gatt_uuid32;
                if (!(uuid[0] & 0xffff0000))
                    /* If the higher 16-bits are 0 */
                {
                    /* Its a 16-bit UUID */
                    *uuid_type = gatt_uuid16;
                }
            }
            else
            {
                *uuid_type = gatt_uuid128;
            }
            break;
            
        default:
            memset(uuid, 0, GATT_UUID_SIZE);
            *uuid_type = gatt_uuid_none;            
    }
    
}
#endif

#if (GATT_FEATURES & GATT_PRIMARY_DISCOVERY)
/*************************************************************************
NAME    
    gatt_get_sdp_uuid
    
DESCRIPTION
    This function is used to extract UUID and UUID type from SDP big
    endian data stream.

RETURNS
    
*/
void gatt_get_sdp_uuid(gatt_uuid_t *uuid,
                       gatt_uuid_type_t *uuid_type,
                       Region *val)
{
    u16 i;

    memmove(uuid + 1, att_base_uuid, 3 * sizeof(u32));

    for (i = 0; val->begin < val->end; i++)
    {
        uuid[i] = RegionReadUnsigned(val);
        val->begin += 4; /* jump to next u32 */
    }
    
    if (!memcmp(uuid + 1, att_base_uuid, 3 * sizeof(u32)))
        /* Base UUID is same as Bluetooth base UUID */
    {
        /* we got 16 bytes of data but the UUID is still either 16-bit
         * or 32-bit attribute UUID */
        *uuid_type = gatt_uuid32;
        if (!(uuid[0] & 0xffff0000))
            /* If the higher 16-bits are 0 */
        {
            /* Its a 16-bit UUID */
            *uuid_type = gatt_uuid16;
        }
    }
    else
    {
        *uuid_type = gatt_uuid128;
    }
}
#endif

#if (GATT_FEATURES & GATT_CHARACTERISTIC_DISCOVERY)
/*************************************************************************
NAME    
    gatt_read8
    
DESCRIPTION
    This function is used to read 8-bit value form ATT little
    endian data stream.
    
RETURNS
    
*/
u8 gatt_read8(u8 **data)
{
    u8 rc;

    rc = (*data)[0];
    (*data)++;
    
    return rc;
}
#endif

#if (GATT_FEATURES & (GATT_RELATIONSHIP_DISCOVERY |     \
                      GATT_CHARACTERISTIC_DISCOVERY))
/*************************************************************************
NAME    
    gatt_read16
    
DESCRIPTION
    This function is used to read 16-bit value form ATT little
    endian data stream.
    
RETURNS
    
*/
u16 gatt_read16(u8 **data)
{
    u16 rc;

    rc = ((*data)[1] << 8) | (*data)[0];
    *data += 2;
    
    return rc;
}
#endif
