/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#include "service_private.h"

bool ServiceNextAttribute(Region *r, u16 *id, ServiceDataType *type, Region *value)
{ 
    Region id_region;
    if(ServiceGetValue(r, type, &id_region) && *type == sdtUnsignedInteger && RegionSize(&id_region) == 2)
    {
        *id = serviceUnpack16(id_region.begin);
        return ServiceGetValue(r, type, value);
    }
    return 0;
}
