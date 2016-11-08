/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_csr_internal_config_handler.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_config_handler.h"
#include "codec_csr_internal_config_handler.h"

#include <stdlib.h>

/****************************************************************************
NAME	
	handleCsrInternalCodecConfigureReq

DESCRIPTION
	Function to handle internal configure request message, for the CSR internal
	codec.
*/
void handleCsrInternalCodecConfigureReq(CsrInternalCodecTaskData *codec, const CODEC_INTERNAL_CONFIG_REQ_T *req)
{
    free(req->config);
	sendCodecConfigureCfm(codec->clientTask, codec_success);
}
