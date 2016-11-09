/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    byte_utils.h
    
DESCRIPTION
    Header file for byte utilities.
*/

#ifndef BYTE_UTILS_H_
#define BYTE_UTILS_H_

#include <csrtypes.h>

u16 ByteUtilsMemCpyToStream(u8 *dst, u8 *src, u16 size);
u16 ByteUtilsMemCpyFromStream(u8 *dst, const u8 *src, u16 size);

u16 ByteUtilsSet1Byte(u8 *dst, u16 byteIndex, u8 val);
u16 ByteUtilsSet2Bytes(u8 *dst, u16 byteIndex, u16 val);
u16 ByteUtilsSet4Bytes(u8 *dst, u16 byteIndex, u32 val);

u8 ByteUtilsGet1ByteFromStream(const u8 *src);
u16 ByteUtilsGet2BytesFromStream(const u8 *src);
u32 ByteUtilsGet4BytesFromStream(const u8 *src);

#endif /* BYTE_UTILS_H_ */
