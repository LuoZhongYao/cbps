/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#include "region.h"

u32 RegionReadUnsigned(const Region *r)
{
    u32 v = 0;
    const u8 *p;
    for(p = r->begin; p != r->end; ++p)
        v = (v<<8) | *p;
    return v;
}
