/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_csr_internal_init_handler.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_init_handler.h"
#include "codec_csr_internal_init_handler.h"


/****************************************************************************
NAME	
	handleCsrInternalCodecInitReq

DESCRIPTION
	Function to handle internal init request message, for the CSR internal
	codec.
*/
void handleCsrInternalCodecInitReq(CsrInternalCodecTaskData *codec)
{
	sendInitCfmToApp(&codec->task, codec->clientTask, codec_success, CODEC_INPUT_GAIN_RANGE, CODEC_OUTPUT_GAIN_RANGE);	
}
