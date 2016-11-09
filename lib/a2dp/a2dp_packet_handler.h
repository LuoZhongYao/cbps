/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_packet_handler.h
    
DESCRIPTION
    
	
*/

#ifndef A2DP_PACKET_HANDLER_H_
#define A2DP_PACKET_HANDLER_H_

#include "a2dp_private.h"


/****************************************************************************
NAME	
	a2dpGrabSink

DESCRIPTION
	Allocates space in the Sink for writing data.

RETURNS
	NULL on failure.
*/
u8 *a2dpGrabSink(Sink sink, u16 size);


/****************************************************************************
NAME	
	a2dpSendPacket

DESCRIPTION
	Flushes an AVDTP packet already written into the passed Sink and
    performs any required fragmentation.

RETURNS
	bool - TRUE on success, FALSE on failure.
*/
bool a2dpSendPacket(remote_device *device, u8 signalling_header, u8 signal_id, u16 payload_size);

void a2dpSendAccept (remote_device *device, avdtp_signal_id signal_id, u16 payload_size);
void a2dpSendReject (remote_device *device, avdtp_signal_id signal_id, u16 error_code);
void a2dpSendGeneralReject (remote_device *device);
bool a2dpSendCommand(remote_device *device, u8 signal_id, const u8* payload_data, u16 payload_size);

void a2dpHandleSignalPacket(remote_device *device);

#endif /* A2DP_PACKET_HANDLER_H_ */
