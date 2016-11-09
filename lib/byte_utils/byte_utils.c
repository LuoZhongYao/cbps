/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    byte_utils.c
    
DESCRIPTION
    Utility functions to deal with different byte sizes on 
    XAP and the rest of the world.
*/

#include "byte_utils.h"

u16 ByteUtilsMemCpyToStream(u8 *dst, u8 *src, u16 size)
{
    u16 i;

    for(i = 0; i < size; ++i)
    {
        if(i%2)
        {
            dst[i] = src[i/2];
        }
        else
        {
            dst[i] = src[i/2] >> 8;
        }
    }

    return size;
}

u16 ByteUtilsMemCpyFromStream(u8 *dst, const u8 *src, u16 size)
{
    u16 i;

    for(i = 0; i < size; ++i)
    {
        ByteUtilsSet1Byte(dst, i, src[i]);
    }

    return size;
}

u16 ByteUtilsSet1Byte(u8 *dst, u16 byteIndex, u8 val)
{
    u16 *ptr2Byte = (u16 *)dst;

    if(byteIndex%2)
    {
        ptr2Byte[byteIndex/2] |= val;
    }
    else
    {
        ptr2Byte[byteIndex/2] = val << 8;
    }

    return 1;
}

u16 ByteUtilsSet2Bytes(u8 *dst, u16 byteIndex, u16 val)
{
    u16 *ptr2Byte = (u16 *)dst;

    if(byteIndex%2)
    {
        ptr2Byte[byteIndex/2] |= val >> 8;
        ptr2Byte[byteIndex/2+1] = val << 8;
    }
    else
    {
        ptr2Byte[byteIndex/2] = val;
    }

    return 2;
}

u16 ByteUtilsSet4Bytes(u8 *dst, u16 byteIndex, u32 val)
{
    byteIndex += ByteUtilsSet2Bytes(dst, byteIndex, val >> 16);
    ByteUtilsSet2Bytes(dst, byteIndex, val);

    return 4;
}

u8 ByteUtilsGet1ByteFromStream(const u8 *src)
{
    return src[0];
}

u16 ByteUtilsGet2BytesFromStream(const u8 *src)
{
    u16 val = 0;

    val = src[1];
    val |= (u16)src[0] << 8;

    return val;
}

u32 ByteUtilsGet4BytesFromStream(const u8 *src)
{
    u32 val = 0;

    val = ((u32)src[3] & 0xff);
    val |= ((u32)src[2] & 0xff) << 8;
    val |= ((u32)src[1] & 0xff) << 16;
    val |= ((u32)src[0] & 0xff) << 24;

    return val;
}


/*u16 ByteUtilsGet1Byte(u8 *src, u16 byteIndex, u8 *val)
{
    u16 *ptr2Byte = (u16 *)src;

    *val = ptr2Byte[byteIndex/2];


    return 1;
}

u16 ByteUtilsGet2Bytes(u8 *src, u16 byteIndex, u16 *val)
{
    u16 *ptr2Byte = (u16 *)src;

    if(byteIndex%2)
    {
        *val = ptr2Byte[byteIndex/2] << 8;
        *val |= ptr2Byte[byteIndex/2 + 1] >> 8;
    }
    else
    {
        *val = ptr2Byte[byteIndex/2];
    }

    return 2;
}

u16 ByteUtilsGet4Bytes(u8 *src, u16 byteIndex, u32 *val)
{
    u16 msb, lsb;

    byteIndex += ByteUtilsGet2Bytes(src, byteIndex, &msb);
    ByteUtilsGet2Bytes(src, byteIndex, &lsb);

    *val = (u32)msb << 16;
    *val |= lsb;

    return 4;
}*/
