/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    pbapc_util.h
    
DESCRIPTION
 Helper header file

*/

#ifndef PBAP_UTIL_H_
#define PBAP_UTIL_H_

/* length of APP Params */
#define APP_PARAM_HDR_LEN   2
#define BYTE_APP_PARAM_LEN  3
#define WORD_APP_PARAM_LEN  4
#define LONG_LONG_WORD_APP_PARAM_LEN 10

#include <csrtypes.h>

enum
{
    pbapc_app_order = 0x01,
    pbapc_app_search_value,
    pbapc_app_search_attr,
    pbapc_app_max_list_count,
    pbapc_app_start_offset,
    pbapc_app_filter,
    pbapc_app_format,
    pbapc_app_phonebook_size,
    pbapc_app_new_missed_calls
};

void pbapcFrame2ByteAppParams( u8* ptr, u8 param, u16 value );
void pbapcFrameByteAppParams( u8* ptr, u8 param, u8 value );
void pbapcFrame4ByteAppParams( u8* ptr, u8 param, u32 value );
u16 pbapcFramevCardListAppParams( u8 order ,
                                     const u8* srchVal,
                                     u16 srchValLen,
                                     u8 srchAttr,
                                     u8* listData );
u16 pbapcFrameListAppParams( u16 maxListCount,
                              u16 startOffset,
                              u8* listData );
u16 pbapcFrameFilterFormat( u32  filterHi,
                               u32  filterLo,
                               u8   format,
                               u8* listData );
const u8 *pbapcGetPbNameFromID(u8 book, u16 *len);
const u8* pbapcGetSimName( u16 *len );
u16 pbapcGetPbPathFromID( bool  sim,  u8 book, u8* path); 
const u8 *pbapcGetPhonebookMimeType(u16 *len);
const u8 *pbapcGetvCardListingMimeType(u16 *len);
const u8 *pbapcGetvCardMimeType(u16 *len);
 
#endif /* PBAP_COMMON_H_ */
