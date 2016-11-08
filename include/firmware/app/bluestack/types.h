/*!

        Copyright Cambridge Silicon Radio Limited 2001-2015

\file   types.h

\brief This file contains basic type definitions.

\mainpage   Bluestack
*/

#ifndef BLUESTACK__TYPES_H
#define BLUESTACK__TYPES_H


#ifdef BLUESTACK_TYPES_INT_TYPE_FILE
#define BLUESTACK_TYPES_DONE
#include BLUESTACK_TYPES_INT_TYPE_FILE
#endif /* BLUESTACK_TYPES_INT_TYPE_FILE */

#ifdef BLUESTACK_TYPES_INT_TYPE_DEFS
#define BLUESTACK_TYPES_DONE
BLUESTACK_TYPES_INT_TYPE_DEFS
#endif /* BLUESTACK_TYPES_INT_TYPE_DEFS */

#ifndef BLUESTACK_TYPES_DONE

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;

#ifndef DONT_HAVE_LIMITS_H
#include <limits.h>
#endif /* ndef DONT_HAVE_LIMITS_H */

#if defined(USHRT_MAX) && (USHRT_MAX >= 0xFFFFFFFFUL)
typedef signed short int int32_t;
typedef unsigned short int uint32_t;
#elif defined(UINT_MAX) && (UINT_MAX >= 0xFFFFFFFFUL)
typedef signed int int32_t;
typedef unsigned int uint32_t;
#else
typedef signed long int int32_t;
typedef unsigned long int uint32_t;
#endif

typedef uint8_t bool_t;
typedef uint32_t uint24_t;

#endif /* ndef BLUESTACK_TYPES_DONE */
#undef BLUESTACK_TYPES_DONE

typedef uint16_t phandle_t;
#define INVALID_PHANDLE ((phandle_t)0xFFFF)

/*!
    Type used with InquiryGetPriority() and InquirySetPriority().
*/
typedef enum
{
    inquiry_normal_priority, /*!< Set inquiry priority to normal */
    inquiry_low_priority     /*!< Set inquiry priority to lower than that of ACL data */
} InquiryPriority;

#endif /* ndef BLUESTACK__TYPES_H */
