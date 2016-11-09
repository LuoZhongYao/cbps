/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_voice_prompts.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_SIMPLE_TESXT_TO_SPEECH_H_
#define _CSR_SIMPLE_TESXT_TO_SPEECH_H_

#define OUTPUT_RATE_48K   48000

void CsrVoicePromptsPluginInit(u16 no_prompts, u16 no_languages);

typedef enum
{
    kalimba_idle,
    kalimba_loaded,
    kalimba_ready
} kalimba_state;

void CsrVoicePromptPluginPlayDsp(kalimba_state state);

void CsrVoicePromptsPluginPlayPhrase(u16 id , u16 language, Task codec_task , u16 prompt_volume , AudioPluginFeatures features);
void CsrVoicePromptsPluginStopPhrase ( void ) ;
void CsrVoicePromptsPluginPlayTone ( TaskData *task, ringtone_note * tone, Task codec_task, u16 tone_volume, AudioPluginFeatures features);
void CsrVoicePromptsPluginSetVolume(i16 prompt_volume, bool using_tone_port);
void CsrVoicePromptsPluginHandleStreamDisconnect(void);

#endif


