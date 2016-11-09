/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sink_uitils.h

DESCRIPTION
    Contains utility functions used by the sink application
*/

#ifndef _SINK_UTILS_H_
#define _SINK_UTILS_H_

#include <csrtypes.h>


/*******************************************************************************
NAME
    bitCounter16

DESCRIPTION
    Function to count the number of set bits in a u16 bitmask
*/
u16 bitCounter16(u16 to_count);


/*******************************************************************************
NAME    
    bitCounter32
    
DESCRIPTION
    Function to count the number of set bits in a 32bit mask
*/
u16 bitCounter32(u32 to_count);


#endif /* _SINK_UTILS_H_ */
