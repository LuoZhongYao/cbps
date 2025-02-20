/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_init.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_csr_internal_message_handler.h"
#include <stdlib.h>
#include <panic.h>

/****************************************************************************
NAME	
	CodecInitCsrInternal

DESCRIPTION
   Initialise the CSR internal Codec. 
   
   CODEC_INIT_CFM message will be received by the application.	
*/
void CodecInitCsrInternal(CsrInternalCodecTaskData* codec, Task appTask)
{
	codec->task.handler = csrInternalMessageHandler;
	codec->clientTask = appTask;
	MessageSend(&codec->task, CODEC_INTERNAL_INIT_REQ, 0);
}


