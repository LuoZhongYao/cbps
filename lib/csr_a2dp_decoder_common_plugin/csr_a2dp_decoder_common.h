/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common.h

DESCRIPTION
    
    
NOTES
   
*/
 
#ifndef _CSR_A2DP_DECODER_COMMON_H_
#define _CSR_A2DP_DECODER_COMMON_H_

#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_multi_channel_plugin.h"

/*  The following PS Key can be used to define a non-default maximum clock mismatch between SRC and SNK devices.
    If the PS Key is not set, the default maximum clock mismatch value will be used.
    The default value has been chosen to have a very good THD performance and to avoid audible pitch shifting
    effect even during harsh conditions (big jitters, for example). While the default covers almost all phones
    and other streaming sources by a big margin, some phones could prove to have a larger percentage clock drift.
*/
#define PSKEY_MAX_CLOCK_MISMATCH    0x2258 /* PSKEY_DSP0 */

#define MIXED_MODE_INCREASING_DELAY 42 /* 42 ms optimum delay for increasing volume */
#define MIXED_MODE_DECREASING_DELAY 25 /* 25 ms optimum delay for decreasing volume */


typedef struct
{
    uint16 id;
    uint16 size;
    char   buf[1];
} DSP_LONG_REGISTER_T;

typedef struct sync_Tag
{
    A2dpPluginTaskdata *task;
    Sink media_sink ;
    Sink forwarding_sink ;
    Task codec_task ;
    /*! mono or stereo*/
    AudioPluginFeatures features;
    /* type of audio source, used to determine what port to connect to dsp */
    AUDIO_SINK_T sink_type:8;
    /*! The current mode */
    unsigned mode:8 ;
    unsigned master_routing_mode:2;
    unsigned slave_routing_mode:2;
    unsigned routing_mode_change_pending:1;
    unsigned stream_relay_mode:2;
    unsigned relay_mode_change_pending:1;
    
    unsigned sbc_encoder_bitpool:8;
    unsigned sbc_encoder_format:8;
    
    unsigned sbc_encoder_params_pending:1;
    unsigned external_volume_enabled:1;
    unsigned device_trims_pending:1;
    unsigned dsp_ports_connected:1;
    unsigned input_audio_port_mute_active:1;
	unsigned :1;
    unsigned packet_size:10;                /* Used to configure RTP transform when forwarding undecoded audio frames */

    /* Additional mode parameters */
    uint16 mode_params;
    /* digital volume structure including trim gains */    
    AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T volume;
    uint16 params;
    uint32 rate;                /* Codec sample rate (input rate to DSP) */
    uint16 dsp_resample_rate;   /* Output sample rate (required output rate from DSP, divided by DSP_RESAMPLING_RATE_COEFFICIENT ready to send in Kalimba message) */
    Task app_task;
    AUDIO_MUTE_STATE_T mute_state[audio_mute_group_max];
}DECODER_t ;


/*plugin functions*/
void csrA2dpDecoderEnableExternalVolume (bool enabled);
void csrA2dpDecoderSetTwsRoutingMode (uint16 master_routing, uint16 slave_routing);
void csrA2dpDecoderSetSbcEncoderParams (uint8 bitpool, uint8 format);
void csrA2dpDecoderSetTWSDeviceTrims (int16 device_trim_master, int16 device_trim_slave);
void csrA2dpDecoderSetStreamRelayMode (uint16 mode);
void CsrA2dpDecoderPluginConnect( A2dpPluginTaskdata *task, 
                                  Sink audio_sink , 
                                  AUDIO_SINK_T sink_type,
                                  Task codec_task , 
                                  uint16 volume , 
                                  uint32 rate , 
                                  AudioPluginFeatures features,
                                  AUDIO_MODE_T mode , 
                                  const void * params, 
                                  Task app_task );
void CsrA2dpDecoderPluginDisconnect( A2dpPluginTaskdata *task ) ;
void CsrA2dpDecoderPluginSetVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *volumeDsp) ;
void CsrA2dpDecoderPluginResetVolume(void);
void CsrA2dpDecoderPluginSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message);
void csrA2dpDecoderPluginOutputMute(audio_plugin_mch_group_t group, AUDIO_MUTE_STATE_T state);
void CsrA2dpDecoderPluginSetMode( AUDIO_MODE_T mode , A2dpPluginTaskdata *task , const void * params ) ;
void CsrA2dpDecoderPluginPlayTone ( A2dpPluginTaskdata *task, ringtone_note * tone , Task codec_task , uint16 tone_volume);
void CsrA2dpDecoderPluginStopTone ( void ) ;
void CsrA2dpDecoderPluginToneComplete ( void ) ;
void CsrA2dpDecoderPluginInternalMessage( A2dpPluginTaskdata *task ,uint16 id , Message message ) ;
bool CsrA2dpDecoderPluginGetLatency (A2dpPluginTaskdata *audio_plugin, bool *estimated, uint16 *latency);
DECODER_t * CsrA2dpDecoderGetDecoderData(void);
void CsrA2dpDecoderPluginSetEqMode(uint16 operating_mode, A2DP_MUSIC_PROCESSING_T music_processing, A2dpPluginModeParams *mode_params);
void CsrA2dpDecoderPluginUpdateEnhancements(A2dpPluginModeParams *mode_params);
void csrA2dpDecoderPluginMicMute(AUDIO_MUTE_STATE_T mute);
void csrA2dpDecoderStartTransformCheckScms(Transform rtp_transform, uint8 content_protection);
void MusicConnectAudio (A2dpPluginTaskdata *task);
void MusicConnectOutputSinks(void);
void CsrA2dpDecoderPluginSetHardwareLevels(AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG_T * message);
void CsrA2dpDecoderPluginStartDisconnect(TaskData * task);
void CsrA2dpDecoderPluginSetLevels(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T * VolumeMsg, bool ForceSetVolume);
void CsrA2dpDecoderPluginAllowVolChanges(void);
void CsrA2dpDecoderPluginSubCheckForConnectionFailure(void);
void CsrA2dpDecoderPluginSetAudioLatency (A2dpPluginTaskdata *audio_plugin, uint16 latency);
uint32 CsrA2DPGetDecoderSampleRate(void);
uint32 CsrA2DPGetDecoderSubwooferSampleRate(void);
void CsrA2dpDecoderPluginSetInputAudioMute(const AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG_T *mute_message);
void csrA2dpDecoderSetTwsCompatibilityMode(peer_buffer_level buffer_level_required);




#endif


