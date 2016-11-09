/******************************************************************************
Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
   Part of ADK 4.0
   $Change: 2479944 $  $DateTime: 2016/02/01 13:36:25 $         
   */           
/******************************************************************************/

/******************************************************************************
    MODULE:
        pblock.c

    DESCRIPTION
        This module implements a persistence system called pblock.
        Pblock allows keyed blocks of data to be persisted to/from a PSKey
        Intended usage is for multiple DSP applications running on a chip, each 
        needing an independent block of persistent memory.
  
    Functions:
        void PblockInit(void)
            Initialize the PBlock system. Called once per powercycle
  
        void PblockStore(void)
            Store the data to persistance. Can be called at will.
  
        u16* PblockGet(u16 entry_id, u16* entry_len)
            Retrieve data from a keyed block
  
        void PblockSet(u16 entry_id, u16 entry_len, u16 *data)
            Store data to a keyed block. Entry will be created if not present

    Notes
        Storage format:
            {entry header}
            entryid_entrysize:8:8
            {entry data}
            data0:16
            data1:16
            dataN:16
            {entry header}
            entryid_entrysize:8:8
            {entry data}
            data0:16
            data1:16
            dataN:16
            {max_size}

   */  
/******************************************************************************/

#ifndef PBLOCK_H_
#define PBLOCK_H_
#include <csrtypes.h>

typedef struct
{
    unsigned id:8;
    unsigned size:8;
    u16   data[1];
} pblock_entry;

typedef struct
{
    u16      key;
    unsigned    len:8;
    unsigned    cur_len:8;
    u16*     cache;
} pblock_key;

pblock_key * PblockInit(u16 key, u16 len);
void PblockStore(void);
const pblock_entry* PblockGet(u16 id);
void PblockSet(u16 id, u16 len, u16 *data);

#endif /*PBLOCK_H_*/
