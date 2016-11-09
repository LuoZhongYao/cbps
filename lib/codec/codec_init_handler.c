/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec_init_handler.c
    
DESCRIPTION
	
*/

#include "codec.h"
#include "codec_private.h"
#include "codec_init_handler.h"


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
					  u16 outputGainRange)
{
	MAKE_CODEC_MESSAGE(CODEC_INIT_CFM);
	message->status = status;
	message->inputGainRange = inputGainRange;
	message->outputGainRange = outputGainRange;
    message->codecTask = codecTask;
	MessageSend(clientTask, CODEC_INIT_CFM, message);
}
