/*
 * Copyright 2015 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.0
 */

#ifndef __UTIL_H

#define __UTIL_H

#include <csrtypes.h>
/*! @file util.h @brief Utility routines
**
**
These routines perform a number of frequently required tasks. They will execute at a greater speed than
similar routines written in VM application code.
*/


/*! 
  @brief Compare two blocks of memory of extent 'size', as u16's. 

  @param a First memory block to compare.
  @param b Second memory block to compare.
  @param size Size of memory blocks to compare.
  @return > 0 if 'a' is lexicographically greater than 'b',
          < 0 if 'a' is lexicographically less than 'b',
            0 if 'a' and 'b' have identical contents.
*/
int UtilCompare(const u16 *a, const u16 *b, size_t size);

/*!
  @brief Compares two memory blocks.

  @param mask The bitmask to use.
  @param value The value to look for.
  @param data_start The memory location of the start of the table.
  @param offset The offset into each table entry that the search will be performed at.
  @param size The size of each table entry.
  @param count The number of entries in the table.

  Conceptually we have a table in memory starting at 'data_start', with 'count' entries where each entry is 'size'.
  UtilFind searches for 'value', at 'offset' from the start of each entry, using bitmask 'mask'.

  @return VM address of the table entry containing 'value' if found, else 0.
*/
const u16 *UtilFind(u16 mask, u16 value, const u16 *data_start, u16 offset, u16 size, u16 count);

/*! 
  @brief Converts a string into a number. 

  @param start The start of the string to convert.
  @param end The 'end' of the string to convert.
  @param result Will contain the resulting number.
  @return A pointer into the string (the first character after
  the number) unless no number was found, in which case 0 is returned.

  Note that the string converted here will not include the character pointed to by 'end'.
  That is, to convert the string "123" you would need to call 'UtilGetNumber(start, start+3, &result)'.

  The number is expected to be an unsigned decimal in the range 0 to 2^16-1.
*/
const u8 *UtilGetNumber(const u8 *start, const u8 *end, u16 *result);

/*!
  @brief Compute a u16 hash value for the memory at 'data', of extent 'size'
  u16's, starting with the given 'seed'.

  @param data The start of the memory block to hash. 
  @param size The size of the memory block to hash.
  @param seed The seed value to use for the hash.
*/
u16 UtilHash(const u16 *data, u16 size, u16 seed);

/*!
  @brief Returns a 16-bit random number. 

  Uses a numerical approach, but the state is shared with the BlueCore firmware 
  which also makes calls into this function so predictability will be low.
*/
u16 UtilRandom(void);

/*!
  @brief Exchanges the high and low bytes of 'size' words at 'data'.

  @param data The memory location to begin swapping from.
  @param size The number of swaps to perform.
*/
void UtilSwap(u16 *data, u16 size);

#endif
