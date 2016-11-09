/* Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#include <bdaddr.h>

void BdaddrConvertBluestackToVm(bdaddr *out, const BD_ADDR_T *in)
{
    out->lap = (u32)(in->lap);
    out->uap = (u8)(in->uap);
    out->nap = (u16)(in->nap);
}
