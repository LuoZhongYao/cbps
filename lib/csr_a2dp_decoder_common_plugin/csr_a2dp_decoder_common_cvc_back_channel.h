/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common.h

DESCRIPTION
    
    
NOTES
   
*/
 
#ifndef _CSR_A2DP_DECODER_COMMON_CVC_BACK_CHANNEL_H_
#define _CSR_A2DP_DECODER_COMMON_CVC_BACK_CHANNEL_H_

#define MINUS_45dB         0x0
#define DAC_MUTE           MINUS_45dB

#define BACK_CHANNEL_SAMPLE_RATE 16000 /* fixed at 16k wbs back channel */

/* cVc mode values */
typedef enum
{
    SYSMODE_STANDBY         = 0,
    SYSMODE_LEFT_PASSTHRU   = 1, 
    SYSMODE_RIGHT_PASSTHRU  = 2, 
    SYSMODE_HFK             = 3,
    SYSMODE_LOW_VOLUME      = 4
} CVC_MODES_T ;

/* cVc call state values */
typedef enum
{
    CALLST_CONNECTED    = 1,
    CALLST_CONNECTING    = 2,
    CALLST_MUTE           = 3 
} CVC_CALL_STATES_T ;

void CsrA2dpDecoderPluginDisconnectMic(A2dpPluginConnectParams *codecData);
void csrA2dpDecoderPluginSetLowLatencyMode(AUDIO_MODE_T mode, A2dpPluginTaskdata *task, A2dpPluginModeParams *mode_params, A2DP_MUSIC_PROCESSING_T music_processing);
void csrA2dpDecoderPluginLowLatencyOutputMute(AUDIO_MUTE_STATE_T state);
void csrA2dpDecoderPluginSetLowLatencySoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message);
void CsrA2dpDecoderPluginSetLowLatencyGain(i16 master_gain, i16 tone_gain);
void CvcMicSetGain(audio_instance instance, audio_channel channel, bool digital, T_mic_gain gain);
void CsrA2dpDecoderConnectBackChannel(A2dpPluginConnectParams * codecData, bool use_two_mic_back_channel);

bool isCodecLowLatencyBackChannel(void);

void CsrA2dpDecoderPluginLowLatencyInternalMessage(A2dpPluginTaskdata *task ,u16 id , Message message);
void CsrA2dpDecoderLowLatencyPluginSetEqMode(A2DP_MUSIC_PROCESSING_T music_processing);

AUDIO_OUTPUT_TYPE_T CsrA2dpDecoderPluginIfLowLatencyGetOutputType(void);

#endif


