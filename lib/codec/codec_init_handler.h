/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_init_handler.h
    
DESCRIPTION
*/

#ifndef CODEC_INIT_HANDLER_H_
#define CODEC_INIT_HANDLER_H_


/****************************************************************************
NAME	
	sendInitCfmToApp

DESCRIPTION
	Send an initialisation confirmation message back to the client application.
*/
void sendInitCfmToApp(Task codecTask, 
					  Task clientTask, 
					  codec_status_code status, 
					  u16 inputGainRange, 
					  u16 outputGainRange);
	

#endif /* CODEC_INIT_HANDLER_H */
