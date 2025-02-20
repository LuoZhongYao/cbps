/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_plugin.c
DESCRIPTION
    Interface file for an audio_plugin
NOTES
*/
#ifndef SCO_DSP_BOUNDARIES
#define SCO_DSP_BOUNDARIES
#endif

#include <audio.h>
#include <codec.h>
#include <stdlib.h>
#include <print.h>
#include <stream.h> /*for the ringtone_note*/
#include <string.h> /*for memcpy*/

#include "audio_plugin_if.h" /*the messaging interface*/
#include "csr_cvc_common_plugin.h"
#include "csr_cvc_common.h"
#include "csr_cvc_common_if.h"
	/*the task message handler*/
static void message_handler (Task task, MessageId id, Message message) ;

	/*the local message handling functions*/
static void handleAudioMessage ( Task task , MessageId id, Message message ) 	;
static void handleInternalMessage ( Task task , MessageId id, Message message ) 	;
	
	/*the plugin task*/
const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_plugin = {{message_handler},CVSD_CVC_1_MIC_HEADSET, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_wbs_cvc_1mic_headset_plugin = {{message_handler},WBS_CVC_1_MIC_HEADSET, LINK_ENCODING_SBC, 0, AUDIO_CODEC_CVSD, 1};
const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_bex_plugin = {{message_handler},CVSD_CVC_1_MIC_HEADSET_BEX, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 1};

const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_plugin = {{message_handler},CVSD_CVC_2_MIC_HEADSET, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_wbs_cvc_2mic_headset_plugin = {{message_handler},WBS_CVC_2_MIC_HEADSET, LINK_ENCODING_SBC, 1, AUDIO_CODEC_CVSD, 1};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_bex_plugin = {{message_handler},CVSD_CVC_2_MIC_HEADSET_BEX, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 1};

/* Handsfree variants */
const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_plugin = {{message_handler},CVSD_CVC_1_MIC_HANDSFREE, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_wbs_cvc_1mic_handsfree_plugin = {{message_handler},WBS_CVC_1_MIC_HANDSFREE, LINK_ENCODING_SBC, 0, AUDIO_CODEC_CVSD, 1};
const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_bex_plugin = {{message_handler},CVSD_CVC_1_MIC_HANDSFREE_BEX, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 1};

const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_plugin = {{message_handler},CVSD_CVC_2_MIC_HANDSFREE, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_wbs_cvc_2mic_handsfree_plugin = {{message_handler},WBS_CVC_2_MIC_HANDSFREE, LINK_ENCODING_SBC, 1, AUDIO_CODEC_CVSD, 1};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_bex_plugin = {{message_handler},CVSD_CVC_2_MIC_HANDSFREE_BEX, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 1};

const CvcPluginTaskdata csr_cvsd_no_dsp_plugin = {{message_handler},CVSD_NO_DSP, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 0};

const CvcPluginTaskdata csr_cvsd_cvc_1mic_asr_plugin = {{message_handler},CVSD_CVC_1_MIC_HEADSET_ASR, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_asr_plugin = {{message_handler},CVSD_CVC_2_MIC_HEADSET_ASR, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 0};

const CvcPluginTaskdata csr_cvsd_cvc_1mic_hf_asr_plugin = {{message_handler},CVSD_CVC_1_MIC_HANDSFREE_ASR, LINK_ENCODING_CVSD, 0, AUDIO_CODEC_CVSD, 0};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_hf_asr_plugin = {{message_handler},CVSD_CVC_2_MIC_HANDSFREE_ASR, LINK_ENCODING_CVSD, 1, AUDIO_CODEC_CVSD, 0};

/****************************************************************************
DESCRIPTION
	The main task message handler
*/
static void message_handler ( Task task, MessageId id, Message message ) 
{
	if ( (id >= AUDIO_DOWNSTREAM_MESSAGE_BASE ) && (id < AUDIO_DOWNSTREAM_MESSAGE_TOP) )
	{
		handleAudioMessage (task , id, message ) ;
	}
	else
	{
		handleInternalMessage (task , id , message ) ;
	}
}	

/****************************************************************************
DESCRIPTION

	messages from the audio library are received here. 
	and converted into function calls to be implemented in the 
	plugin module
*/ 
static void handleAudioMessage ( Task task , MessageId id, Message message ) 	
{
    bool audio_busy = (bool)IsAudioBusy();
    
	switch (id)
	{
		case (AUDIO_PLUGIN_CONNECT_MSG ):
		{
			AUDIO_PLUGIN_CONNECT_MSG_T * connect_message = (AUDIO_PLUGIN_CONNECT_MSG_T *)message ;
	
			if (audio_busy)
			{ 		/*Queue the connect message until the audio task is available*/
				MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_CONNECT_MSG ) ; 
				
				message->audio_sink = connect_message->audio_sink ;
				message->sink_type  = connect_message->sink_type ;
				message->codec_task = connect_message->codec_task ;
				message->volume     = connect_message->volume ;
				message->rate       = connect_message->rate ;
				message->mode 		= connect_message->mode ;
				message->features   = connect_message->features ;
				message->params     = connect_message->params ;
				message->power      = connect_message->power ;
				message->params     = connect_message->params;
                message->app_task   = connect_message->app_task;
			    MessageSendConditionally ( task, AUDIO_PLUGIN_CONNECT_MSG , message , (const u16 *)AudioBusyPtr() ) ;
				PRINT(("CVC: audio connect requeued\n"));				
			} 
			else
			{
                /* Get the audio source */
                Source audio_source = StreamSourceFromSink(connect_message->audio_sink);
                CvcPluginTaskdata* cvc_task = (CvcPluginTaskdata*)task;
                hfp_common_plugin_params_t* params   = (hfp_common_plugin_params_t*)connect_message->params;
                
                if(params)
                {
                    /* Separate source for USB */
                    if(connect_message->sink_type == AUDIO_SINK_USB)
                        connect_message->audio_sink = params->usb_params.usb_sink;
                }

                PRINT(("CVC: audio connect actioned\n"));				
                
                /*connect the audio*/
				CsrCvcPluginConnect(  cvc_task,
                                      connect_message,
                                      audio_source);
			}			
		}	
		break ;
		
		case (AUDIO_PLUGIN_DISCONNECT_MSG ):
		{
			if (audio_busy)
			{
   				PRINT(("CVC: audio disconnect requeued\n"));				
				MessageSendConditionally ( task, AUDIO_PLUGIN_DISCONNECT_MSG , 0 ,(const u16 *)AudioBusyPtr() ) ;
    		}
			else
			{		
				PRINT(("CVC: audio disconnect actioned\n"));				
				CsrCvcPluginDisconnect((CvcPluginTaskdata*)task) ;
			}
		}	
		break ;
		
		case (AUDIO_PLUGIN_SET_MODE_MSG ):
		{
            AUDIO_PLUGIN_SET_MODE_MSG_T * mode_message = (AUDIO_PLUGIN_SET_MODE_MSG_T *)message ;			
			
            if (audio_busy || (GetCurrentDspStatus() && (GetCurrentDspStatus() != DSP_RUNNING)))
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_SET_MODE_MSG) ;
                message->mode   = mode_message->mode ;
                message->params = mode_message->params ;
        
        		MessageSendConditionally ( task, AUDIO_PLUGIN_SET_MODE_MSG , message ,(const u16 *)AudioBusyPtr()) ;
    	    }
            else
            {
				CsrCvcPluginSetMode((CvcPluginTaskdata*)task, mode_message->mode , mode_message->params) ;
            }
		}
		break ;
        
        /* Message from VM application via audio lib to configure the mute state of the microphone / outputs */
        case (AUDIO_PLUGIN_SET_SOFT_MUTE_MSG):
        {
            AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* mute_message = (AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T*)message;
            
            if (audio_busy)
            {
                MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_SOFT_MUTE_MSG );
                memcpy(message, mute_message, sizeof(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T));
                
                MessageSendConditionally(task, AUDIO_PLUGIN_SET_SOFT_MUTE_MSG, message, (const u16*)AudioBusyPtr());
            }
            else
            {
                CsrCvcPluginSetSoftMute(mute_message);
            }
        }
        break ;
		
		case (AUDIO_PLUGIN_SET_VOLUME_MSG ): 
		{
			AUDIO_PLUGIN_SET_VOLUME_MSG_T * volume_message = (AUDIO_PLUGIN_SET_VOLUME_MSG_T *)message ;			
			
			if (audio_busy)
			{
			     MAKE_AUDIO_MESSAGE (AUDIO_PLUGIN_SET_VOLUME_MSG ) ;
			     message->volume = volume_message->volume ;
                 message->tone_volume = volume_message->tone_volume;
			     
		         MessageSendConditionally ( task, AUDIO_PLUGIN_SET_VOLUME_MSG , message ,(const u16 *)AudioBusyPtr() ) ;    	
            }
            else
            {
                CsrCvcPluginSetVolume (volume_message->volume ) ;
            }			
		}		
		break ;
		
   		case (AUDIO_PLUGIN_RESET_VOLUME_MSG ): 
		{
			if (audio_busy)
			{
		         MessageSendConditionally ( task, AUDIO_PLUGIN_RESET_VOLUME_MSG , 0 ,(const u16 *)AudioBusyPtr() ) ;    	
            }
            else
            {
                CsrCvcPluginResetVolume () ;
            }			
		}		
		break ;

		case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
		{
			AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message = (AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message ;
			
			if (audio_busy || IsTonePlaying() || IsVpPlaying()) 
			{	
				if ( tone_message->can_queue) /*then re-queue the tone*/
				{				
					MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_TONE_MSG ) ; 
					
					message->tone        = tone_message->tone       ;
					message->can_queue   = tone_message->can_queue  ;
					message->codec_task  = tone_message->codec_task ;
					message->tone_volume = tone_message->tone_volume;
					message->features    = tone_message->features   ;
	
					PRINT(("TONE:Q\n"));
					
					MessageSendConditionally ( task , AUDIO_PLUGIN_PLAY_TONE_MSG, message ,(const u16 *)AudioBusyPtr() ) ;			
				}
			}
			else
			{
				PRINT(("TONE CVC:start\n"));				
                SetAudioBusy((TaskData*) task);    
                SetTonePlaying(TRUE);
                CsrCvcPluginPlayTone ((CvcPluginTaskdata*)task, tone_message->tone, tone_message->tone_volume) ;
			}
							     
		}
		break ;
		
		case (AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG ):
		{
			if (audio_busy)
			{
   				PRINT(("CVC: stop tone\n"));				
				CsrCvcPluginStopTone((CvcPluginTaskdata*)task) ;
			}
		}
		break ;	
        
        case (AUDIO_PLUGIN_MIC_SWITCH_MSG ):
#ifdef CVC_ALL		
        {
            if (audio_busy)
            {
                MessageSendConditionally ( task, AUDIO_PLUGIN_MIC_SWITCH_MSG , 0 ,(const u16 *)AudioBusyPtr()) ;
            }
            else
            {
                CsrCvcPluginMicSwitch() ;
            }
        }
#endif		
        break ;

        case (AUDIO_PLUGIN_OUTPUT_SWITCH_MSG):
#ifdef CVC_ALL
        {
            if (audio_busy)
            {
                MessageSendConditionally ( task, AUDIO_PLUGIN_OUTPUT_SWITCH_MSG , 0 ,(const u16 *)AudioBusyPtr()) ;
            }
            else
            {
                CsrCvcPluginOutputSwitch() ;
            }
        }
#endif
        break;
		
		case (AUDIO_PLUGIN_SET_POWER_MSG ):
		{
			AUDIO_PLUGIN_SET_POWER_MSG_T * power_message = (AUDIO_PLUGIN_SET_POWER_MSG_T *)message ;
			
			CsrCvcPluginSetPower((CvcPluginTaskdata*)task, power_message->power) ;
		}
		break ;
        
        /* start or restart the ASR engine */
		case (AUDIO_PLUGIN_START_ASR ):
		{
            AUDIO_PLUGIN_START_ASR_T * asr_message = (AUDIO_PLUGIN_START_ASR_T *)message ;			
			
            if (audio_busy || (GetCurrentDspStatus() && (GetCurrentDspStatus() != DSP_RUNNING)))
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_START_ASR) ;
                message->mode   = asr_message->mode ;
        
        		MessageSendConditionally ( task, AUDIO_PLUGIN_START_ASR , message ,(const u16 *)AudioBusyPtr()) ;
    	    }
            else
            {
				CsrCvcPluginSetAsr((CvcPluginTaskdata*)task, asr_message->mode) ;
            }
		}
		break ;

        
        
        
		default:
		{
		}
		break ;
	}
}

/****************************************************************************
DESCRIPTION
	Internal messages to the task are handled here
*/ 
static void handleInternalMessage ( Task task , MessageId id, Message message ) 	
{
	switch (id)
	{
        case MESSAGE_STREAM_DISCONNECT: /*a tone has completed*/
        {
            PRINT(("CVC: Tone End\n"));
            CsrCvcPluginToneComplete((CvcPluginTaskdata*)task) ;            
        }    
		break ;
		
        case MESSAGE_FORCE_TONE_COMPLETE:
        {
            CsrCvcPluginToneForceCompleteNoDsp((CvcPluginTaskdata*)task);
        }
        break;
		
		default:
		{
				/*route the cvc messages to the relavent handler*/
			CsrCvcPluginInternalMessage((CvcPluginTaskdata*)task , id , message ) ;
		}
		break ;
	}	
}
