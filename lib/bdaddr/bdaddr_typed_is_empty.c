/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#include <bdaddr.h>

bool BdaddrTypedIsEmpty(const typed_bdaddr *in)
{
    return  in->type == TYPED_BDADDR_INVALID &&
            BdaddrIsZero(&in->addr);
}
