/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_audio_prompts.h
    
DESCRIPTION
    header file which defines the interface between the audio (voice) prompts and the application
    
*/  

#ifndef SINK_AUDIO_PROMPTS_H
#define SINK_AUDIO_PROMPTS_H
#include "sink_debug.h"


#define AUDIO_PROMPT_NOT_DEFINED (0xFF)



/****************************************************************************

*/
void AudioPromptConfigure( u8 size_index );

/****************************************************************************

*/
void AudioPromptPlay(Task plugin, u16 id, bool can_queue, bool override);

/****************************************************************************

*/
bool AudioPromptPlayEvent( sinkEvents_t event );

/****************************************************************************
NAME 
    AudioPromptPlayNumString
DESCRIPTION
    Play a numeric string using the Audio Prompt plugin
RETURNS    
*/
void AudioPromptPlayNumString(u16 size_num_string, u8* num_string);

/****************************************************************************
NAME 
    AudioPromptPlayNumber
DESCRIPTION
    Play a u32 using the audio prompt plugin
RETURNS    
*/
void AudioPromptPlayNumber(u32 number);

/* **************************************************************************
   */


bool AudioPromptPlayCallerNumber( const u16 size_number, const u8* number );

/****************************************************************************
NAME    
    AudioPromptPlayCallerName
    
DESCRIPTION
  	function to play caller name
    
RETURNS
    
*/
bool AudioPromptPlayCallerName( const u16 size_name, const u8* name );
   
/****************************************************************************
NAME    
    AudioPromptCancelNumString
    
DESCRIPTION
  	function to cancel any pending number string messages.
    
RETURNS
    
*/
void AudioPromptCancelNumString( void );

/****************************************************************************
NAME    
    AudioPromptSelectLanguage
    
DESCRIPTION
  	Move to next language
    
RETURNS
    
*/
void AudioPromptSelectLanguage( void );

#endif

