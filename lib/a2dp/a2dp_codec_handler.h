/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_sep_handler.h
    
DESCRIPTION
	
*/

#ifndef A2DP_CODEC_HANDLER_H_
#define A2DP_CODEC_HANDLER_H_

#include "a2dp_private.h"


/****************************************************************************
NAME	
	processCodecInfo

DESCRIPTION
	Process the codec capabilities selected.
	
*/
void processCodecInfo(bool accept, u16 size_codec_service_caps, const u8 *codec_service_caps);


/****************************************************************************
NAME	
	a2dpHandleSelectingCodecSettings

DESCRIPTION
	Select the correct capabilities depending on which codec is selected.
		
*/
bool a2dpSelectOptimalCodecSettings(remote_device *device, u8 *remote_codec);


/****************************************************************************
NAME	
	a2dpSendCodecAudioParams

DESCRIPTION
	Choose configured codec parameterss and send them to the application.
		
*/
a2dp_codec_settings * a2dpGetCodecAudioParams (remote_device *device);


#endif /* A2DP_CODEC_HANDLER_H_ */
