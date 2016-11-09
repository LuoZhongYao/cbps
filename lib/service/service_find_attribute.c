/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#include "service.h"

bool ServiceFindAttribute(Region *r, u16 id, ServiceDataType *type, Region *out)
{
    u16 found;
    while(ServiceNextAttribute(r, &found, type, out))
        if(found == id)
            return 1;
    return 0;
}

