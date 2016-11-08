/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_config_handler.h
    
DESCRIPTION
*/

#ifndef CODEC_CONFIG_HANDLER_H_
#define CODEC_CONFIG_HANDLER_H_


/****************************************************************************
NAME	
	sendCodecConfigureCfm

DESCRIPTION
	Send a CODEC_CONFIG_CFM message back to the client application with
	a status code.
*/
void sendCodecConfigureCfm(Task clientTask, codec_status_code status);
		

#endif /* CODEC_CONFIG_HANDLER_H */
