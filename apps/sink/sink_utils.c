/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_utils.c

DESCRIPTION
    Contains utility functions used by the sink application
*/

#include "sink_utils.h"

/*******************************************************************************
NAME
    bitCounter16

DESCRIPTION
    Function to count the number of set bits in a u16 bitmask
*/
u16 bitCounter16(u16 to_count)
{
    u16 counter = 0;
    counter = to_count - ((to_count >> 1) & 0x5555);
    counter = ((counter >> 2) & 0x3333) + (counter & 0x3333);
    counter = ((counter >> 4) + counter) & 0x0F0F;
    counter = ((counter >> 8) + counter) & 0x00FF;

    return counter;
}


/******************************************************************************/
u16 bitCounter32(u32 to_count)
{
    to_count = to_count - ((to_count >> 1) & 0x55555555);
    to_count = (to_count & 0x33333333) + ((to_count >> 2) & 0x33333333);
    return (((to_count + (to_count >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
