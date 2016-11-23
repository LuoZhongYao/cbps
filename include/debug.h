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

#   define PUTS(level, tag, fmt, ...)      printf(level "/%-16.16s : " fmt, tag, ##__VA_ARGS__)

#else

#   define PUTS(...)

#endif /*DEBUG_PRINT_ENABLED*/
    /* If you want to carry out cVc license key checking in Production test
   Then this needs to be enabled */

#define DEBUG(tag,fmt, ...)      PUTS("D", tag, fmt, ##__VA_ARGS__)
#define INFO(tag,fmt, ...)       PUTS("I", tag, fmt, ##__VA_ARGS__)
#define WARNING(tag,fmt, ...)    PUTS("W", tag, fmt, ##__VA_ARGS__)
#define ERROR(tag, fmt, ...)     PUTS("E", tag, fmt, ##__VA_ARGS__

#define LOGD(fmt,...)        DEBUG(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt,...)        INFO(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt,...)        WARNING(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt,...)        ERROR(LOG_TAG, fmt, ##__VA_ARGS__)
#define NOT_IMPL()           LOGW("Not Impl %s:%d\n", __func__,__LINE__)

#ifdef DEBUG_PEER
#   define PEER_DEBUG(x) DEBUG(x)
#else
#   define PEER_DEBUG(x) 
#endif


#endif /*_SINK_DEBUG_H_*/
