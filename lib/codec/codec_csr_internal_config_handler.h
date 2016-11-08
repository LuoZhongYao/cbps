/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_csr_internal_config_handler.h
    
DESCRIPTION
*/

#ifndef CODEC_CSR_INTERNAL_CONFIG_HANDLER_H_
#define CODEC_CSR_INTERNAL_CONFIG_HANDLER_H_


/****************************************************************************
NAME	
	handleCsrInternalCodecConfigureReq

DESCRIPTION
	Function to handle internal configure request message, for the CSR internal
	codec.
*/
void handleCsrInternalCodecConfigureReq(CsrInternalCodecTaskData *codec, const CODEC_INTERNAL_CONFIG_REQ_T *req);
	

#endif /* CODEC_CSR_INTERNAL_CONFIG_HANDLER_H */

