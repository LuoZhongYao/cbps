/*
 * Copyright Cambridge Silicon Radio Limited 2001-2015
 */

/*!
	@mainpage VM and Native Reference Guide

	This section documents all the functions and types that are available to 
	an on-chip VM or Native application. Use the links above to navigate the documentation.
	 
*/

/*!
	@file csrtypes.h
	@brief Contains definitions of commonly required types.
*/

#ifndef __TYPES_H

#define __TYPES_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

/* ANSI types */

/* typedef unsigned int size_t; */

/* XAP types */

#define __unused __attribute__((__unused__))

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

/* Types for PAN */

#ifndef TRUE
#define TRUE           ((bool)1)
#endif
#ifndef FALSE
#define FALSE          ((bool)0)
#endif

#endif
