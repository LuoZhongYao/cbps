/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */
#ifndef A2DP_SHIM_LAYER_H
#define A2DP_SHIM_LAYER_H


#include <a2dp.h>


typedef enum
{
    A2DP_CODEC_SETTINGS_IND_TEST_EXTRA  = A2DP_MESSAGE_TOP
} A2dpShimMessageId;


typedef struct
{
    A2DP					*a2dp;				
	u32      			rate;				
	a2dp_channel_mode		channel_mode;		
	u8					seid;
    u8                   content_protection;
    u32                  voice_rate;
    u16                  bitpool;
    u16                  format;
    u16                  packet_size;
} A2DP_CODEC_SETTINGS_IND_TEST_EXTRA_T;


void A2dpHandleComplexMessage(Task task, MessageId id, Message message);

void A2dpInitTestExtraAppselect(Task theAppTask, u16 linkloss_timeout);

void A2dpStartKalimbaStreaming(const A2DP* a2dp, uintptr_t media_sink);
void A2dpStopKalimbaStreaming(void);
void A2dpInitTestExtraDefault( Task theAppTask, u8 role, bool enable_mp3 );

void A2dpConnectTestExtra(bdaddr *addr);
void A2dpConnectResponseTestExtra(u16 device_id, bool accept);
void A2dpDisconnectTestExtra(u16 device_id);

void A2dpOpenTestExtra(u16 device_id);
void A2dpOpenResponseTestExtra(u16 device_id, bool accept);
void A2dpCloseTestExtra(u16 device_id, u16 stream_id);
void A2dpMediaStartTestExtra(u16 device_id, u16 stream_id);
void A2dpMediaStartResponseTestExtra(u16 device_id, u16 stream_id, bool accept);
void A2dpMediaSuspendTestExtra(u16 device_id, u16 stream_id);
void A2dpMediaAvSyncDelayResponseTestExtra(u16 device_id, u16 stream_id, u16 delay);

void A2dpReconfigureTestExtra(u16 device_id, u16 stream_id, u16 size_sep_caps, u8 *sep_caps);

void A2dpSendMediaPacketTestExtra(u16 device_id, u16 stream_id);

#endif
