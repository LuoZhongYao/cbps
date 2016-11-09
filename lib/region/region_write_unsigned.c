/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#include "region.h"

void RegionWriteUnsigned(const Region *r, u32 value)
{
    u8 *p = (u8 *) (r->end);
    while(p != r->begin)
    {
        *--p = (u8) (value & 0xFF);
        value >>= 8;
    }
}

