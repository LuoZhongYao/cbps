/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#ifndef SERVICE_PRIVATE_H_
#define SERVICE_PRIVATE_H_

#include "service.h"

static u16 __inline__ serviceUnpack16(const u8 *s)
{ return (((u16)s[0])<<8) | (u16)s[1]; }

static u32 __inline__ serviceUnpack32(const u8 *s)
{
    u32 r = s[0];
    r <<= 8; r |= s[1];
    r <<= 8; r |= s[2];
    r <<= 8; r |= s[3];
    return r;
}

#endif /* SERVICE_PRIVATE_H_ */
