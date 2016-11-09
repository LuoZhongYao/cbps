/*
Copyright (c) 2005-2015  Qualcomm Technologies International, Ltd.
*/

/* This implementation is derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm 
*/

/* 
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it is
identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is" without
express or implied warranty of any kind.
These notices must be retained in any copies of any part of this
documentation and/or software.
*/

#include "md5.h"

#include <string.h>

#define min(a,b) ((a)<(b)?(a):(b))

/* Unpack an array of u32's into a little-endian array of u8's. */

static void encode(u8 *output, const u32 *input, u16 len)
{
    u16 j;
    u32 t = 0;

    for (j = 0; j < len; ++j)
    {            
        if((j & 3) == 0) t = *input++;
        output[j] = (u8) (t & 0xFF);        
        t >>= 8;
    }
}

/* 
   Or an array of u8's into the specified offset of little-endian u32's
   Assumes that the u32 array has previously been zeroed
*/

static void fill(u32 word[16], u16 index, const u8 *byte, u16 bytes)
{
    u16 i;
    for(i = 0; i < bytes; ++i, ++index)
        word[index >> 2] |= ((u32)(byte[i] & 0xFF)) << (8 * (index & 3));
}

/* Basic MD5 transform of a single 512-bit block */

static void transform(MD5_CTX *context)
{
    const static u32 k[64] =
        {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
            0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
            0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
            0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
            0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
            0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
            0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
            0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
        };
    const static u8 r[4][4] =
        {
            {  7, 12, 17, 22 },
            {  5,  9, 14, 20 },
            {  4, 11, 16, 23 },
            {  6, 10, 15, 21 }
        };
    u16 i;
    u32 a = context->state[0];
    u32 b = context->state[1];
    u32 c = context->state[2];
    u32 d = context->state[3];
  
    for(i = 0; i < 64; ++i)
    {
        u32 f;
        u16 g;
        u16 shift = r[i / 16][i & 3];
        if(i < 16)
        {
            g = i;      
            f = d ^ (b & (c ^ d));
        }
        else if(i < 32)
        {
            g = 5*i + 1;
            f = c ^ (d & (b ^ c));
        }
        else if(i < 48)
        {
            g = 3*i + 5;
            f = b ^ (c ^ d);
        }
        else 
        {
            g = 7*i;
            f = c ^ (b | ~d);
        }      
        f += a + k[i] + context->buffer[g & 15];
        f = (f << shift) | (f >> (32-shift)); /* Rotate left by 'shift' */
        a = d; 
        d = c;
        c = b;
        b += f;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;

    /* Clear input buffer again as required by next fill */
    memset(context->buffer, 0, sizeof(context->buffer));
}

/*
   MD5 initialization. Begins an MD5 operation, writing a new context. 
*/

void MD5Init(MD5_CTX *context)
{
    const static u32 seed[4] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    memset(context, 0, sizeof(*context));
    memmove(context->state, seed, sizeof(seed));
}

/* 
   MD5 block update operation. 

   Continues an MD5 message-digest operation, processing another
   message block, and updating the context.
*/

void MD5Update(MD5_CTX *context, const u8 *input, u16 len)
{
    /* Compute number of bytes mod 64 */
    u16 i, n;
    u16 index = context->bytes & 0x3F;

    context->bytes += len;
    for(i = 0; i < len; i += n)
    {
        n = min(64 - index, len - i);
        /* Append bytes into buffer inside context */
        fill(context->buffer, index, &input[i], n);
        /* If buffer is full, process it */
        if(index+n == 64)
        {
            transform(context);
            index = 0;
        }
    }
}

/* 
   MD5 finalization.

   Ends an MD5 message-digest operation, extracting the digest.
*/

void MD5Final(u8 digest[16], MD5_CTX *context)
{
    const u8 pad = 0x80;
    u16 index = (u16)(context->bytes & 0x3f);

    fill(context->buffer, index++, &pad, 1);
    if(index > 56) transform(context);

    context->buffer[14] = context->bytes << 3;
    context->buffer[15] = context->bytes >> 29;
    transform(context);
  
    encode (digest, context->state, 16);
}
