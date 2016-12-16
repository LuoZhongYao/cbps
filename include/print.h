/*******************************************************************************
Copyright (c) 2013 - 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

/* 
    Library allowing user to add debug printfs to their code and be able to
    enable / disable them as required using a single switch.
*/

/*!
 @file print.h
 @brief Debug print functions.

  This library allows applications to contain debug printfs in their code and
  to able to enable / disable them as required using a single switch:

  #define DEBUG_PRINT_ENABLED

  PRINT and CPRINT take the same arguments as printf and cprintf.
*/

#include <debug.h>

#define PRINT(x) LOGD x
