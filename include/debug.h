/***************************************************************************
Copyright 2004 - 2015 Qualcomm Technologies International, Ltd.
FILE NAME
    sink_debug.h
DESCRIPTION
*/
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#ifndef LOG_TAG
#   define LOG_TAG __func__
#endif

#define DEBUG_PRINT_ENABLED

#ifdef DEBUG_PRINT_ENABLED

#   define DEBUG(tag,fmt, ...)  printf("D/%-16.16s : " fmt, tag, ##__VA_ARGS__)
#   define LOGD(fmt,...)        DEBUG(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#   define DEBUG(x) 
#endif /*DEBUG_PRINT_ENABLED*/
    /* If you want to carry out cVc license key checking in Production test
   Then this needs to be enabled */

#ifdef DEBUG_PEER
#   define PEER_DEBUG(x) DEBUG(x)
#else
#   define PEER_DEBUG(x) 
#endif

#   define NOT_IMPL()   LOGD("Not Impl %s:%d\n", __func__,__LINE__)

#endif /*_SINK_DEBUG_H_*/
