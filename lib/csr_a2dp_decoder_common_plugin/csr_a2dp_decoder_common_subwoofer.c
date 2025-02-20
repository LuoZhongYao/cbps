/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_subwoofer.c
DESCRIPTION
    subwoofer specific functions
NOTES
*/

#include <audio.h>
#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <print.h>
#include <file.h>
#include <stream.h> /*for the ringtone_note*/
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <message.h>
#include <ps.h>
#include <transform.h>
#include <string.h>
#include <pio_common.h>

#include "csr_i2s_audio_plugin.h"
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_a2dp_decoder_common_plugin.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_common_subwoofer.h"
#include "csr_a2dp_decoder_common_cvc_back_channel.h"

/****************************************************************************
DESCRIPTION
    connect the subwoofer if a subwoofer link has been established
*/
void CsrA2dpDecoderPluginConnectSubwoofer(A2dpPluginConnectParams *codecData)
{     
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    
    PRINT(("DECODER:Try to connect sub\n"));
 
    /* ensure the decoder is loaded */
    if(codecData)
    {
        /* if the sub woofer is available at connection time? */
        if(codecData->sub_woofer_type != AUDIO_SUB_WOOFER_NONE)
        {
            /* determine the sub link type */
            switch(codecData->sub_woofer_type)
            {
                /* SUB using esco as its bt link, connect to dsp port 2 */
                case AUDIO_SUB_WOOFER_ESCO:
                    /* ensure the correct ype of link is requested */
                    if(DECODER->sink_type != AUDIO_SINK_AV)
                    {
                        PRINT(("DECODER: connect esco sub\n"));
                        /* connect SUB output from dsp, source port 2, to esco sink  */
                        StreamConnect(StreamKalimbaSource(DSP_ESCO_SUB_PORT),codecData->sub_sink);
                        /* update port connection status */                    
                        codecData->sub_connection_port = DSP_ESCO_SUB_PORT;
                        /* set volume levels to desired level */
                        SubConnectedNowUnmuteVolume(DECODER);
                    }
                    else
                        PRINT(("DECODER: connect esco sub - ***wrong type***\n"));                        
                break;
    
                /* SUB using l2cap as its bt link, connect to dsp port 3 */
                case AUDIO_SUB_WOOFER_L2CAP:
                    /* ensure the correct ype of link is requested */
                    if(DECODER->sink_type == AUDIO_SINK_AV)
                    {
                        PRINT(("DECODER: connect l2cap sub\n"));
                        /* connect SUB output from dsp, source port 3, to l2cap sink  */
                        StreamConnect(StreamKalimbaSource(DSP_L2CAP_SUB_PORT),codecData->sub_sink);
                        /* update port connection status */                                        
                        codecData->sub_connection_port = DSP_L2CAP_SUB_PORT;
                        /* set volume levels to desired level */
                        SubConnectedNowUnmuteVolume(DECODER);
                    }
                    else
                        PRINT(("DECODER: connect l2cap sub - ***wrong type***\n"));                        
                break;              
    
                default:
                break;
            }
        }
    }
}

/****************************************************************************
DESCRIPTION
    disconnect the subwoofer if currently connected
*/
void CsrA2dpDecoderPluginDisconnectSubwoofer(void)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    
    /* ensure the decoder is loaded */
    if(DECODER)
    {
        A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;

        /* if the sub woofer is connected then disconnect it */
        if(codecData->sub_connection_port != DSP_SUB_PORT_NOT_CONNECTED)
        {
            PRINT(("DECODER: CsrA2dpDecoderPluginDisconnect disconnect sub\n"));
    
            /* disconnect kalimba port, this causes dsp app to switch clock source internally */
            StreamDisconnect(StreamKalimbaSource(codecData->sub_connection_port), codecData->sub_sink );
    
            /* update connected ports state */
            codecData->sub_connection_port = DSP_SUB_PORT_NOT_CONNECTED;
        }
    }
}

/****************************************************************************
DESCRIPTION
    configure the sub woofer and connect its audio if present
*/
void CsrA2dpDecoderPluginSetSubWoofer(AUDIO_SUB_TYPE_T sub_type, Sink sub_sink)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    
    /* ensure the decoder is loaded */
    if(DECODER)
    {
        A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;

        /* update the decoder params with the new sub woofer status */
        codecData->sub_woofer_type = sub_type;
        codecData->sub_sink = sub_sink;

        /* determine sub woofer type and connect (or disconnect if gone away) the sink
           to the appropriate dsp port */
        switch(sub_type)
        {
            /* SUB no longer available, disconnect port if still connected */
            case AUDIO_SUB_WOOFER_NONE:
                PRINT(("DECODER: Disconnect woofer\n" ));
                /* if source is still connected then disconnect it */
                if(codecData->sub_connection_port != DSP_SUB_PORT_NOT_CONNECTED)
                {
                    /* determine if the subwoofer has connected after the decoder was loaded,
                       configured and unmuted */
                    if(codecData->sub_is_available == FALSE)
                    {
                        /* mute the outputs whilst the subwoofer port is connected and
                           decoder restarted */
                        csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_ENABLE);
                    }

                    PRINT(("DECODER: Disconnect kalimba port %x\n", codecData->sub_connection_port));
                    StreamDisconnect(StreamKalimbaSource(codecData->sub_connection_port), 0);                 
                    /* update connected ports state */
                    codecData->sub_connection_port = DSP_SUB_PORT_NOT_CONNECTED;
                    /* set volume levels to desired level */
                    SubConnectedNowUnmuteVolume(DECODER);
                    
                    /* determine if the subwoofer has connected after the decoder was loaded,
                       configured and unmuted */
                    if(codecData->sub_is_available == FALSE)
                    {
                        /* unmute the outputs again */
                        csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_DISABLE);
                    }
                }
            break;

            /* SUB using esco as its bt link, connect to dsp port 2 */
            case AUDIO_SUB_WOOFER_ESCO:
                PRINT(("DECODER: connect esco dsp port 2, sub_sink = %p\n", sub_sink ));
                /* ensure the correct ype of link is requested */
                if(DECODER->sink_type != AUDIO_SINK_AV)
                {               
                    /* connect SUB output from dsp, source port 2, to esco sink  */
                    if(codecData->sub_connection_port != DSP_ESCO_SUB_PORT)
                    {
                        /* determine if the subwoofer has connected after the decoder was loaded,
                           configured and unmuted */
                        if(codecData->sub_is_available == FALSE)
                        {
                            /* mute the outputs whilst the subwoofer port is connected and
                               decoder restarted */
                            csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_ENABLE);
                        }
                        
                        /* if not already connected */
                        if(codecData->sub_connection_port == DSP_L2CAP_SUB_PORT)
                        {
                            /* wrong port connected, disconnect it first */
                            StreamDisconnect(StreamKalimbaSource(codecData->sub_connection_port), 0);
                            PRINT(("DECODER: Disconnect kalimba port %x\n", codecData->sub_connection_port));
                        }
                        /* ensure sink is valid */
                        if(SinkIsValid(sub_sink))
                        {
                            /* connect esco (2) port */
                            if(StreamConnect(StreamKalimbaSource(DSP_ESCO_SUB_PORT),sub_sink))
                            {
                                /* update connected ports state */
                                codecData->sub_connection_port = DSP_ESCO_SUB_PORT;
                                PRINT(("DECODER: Connect kalimba port %x\n", codecData->sub_connection_port));
                                /* set volume levels to desired level */
                                SubConnectedNowUnmuteVolume(DECODER);
                            }
                            else
                                PRINT(("DECODER: connect esco dsp port 2 FAILED\n" ));
                        }
                        else
                        {
                            PRINT(("DECODER: connect esco dsp port 2, sub_sink = %p NOT VALID\n", sub_sink ));
                        }

                        /* determine if the subwoofer has connected after the decoder was loaded,
                           configured and unmuted */
                        if(codecData->sub_is_available == FALSE)
                        {
                            /* unmute the outputs again */
                            csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_DISABLE);
                        }

                    }
                }
                else
                    PRINT(("DECODER: ESCO - ****wrong link type****\n"));                    
            break;

            /* SUB using l2cap as its bt link, connect to dsp port 3 */
            case AUDIO_SUB_WOOFER_L2CAP:
                PRINT(("DECODER: connect l2cap dsp port 3, sub_sink = %p\n", sub_sink ));
                /* ensure the correct ype of link is requested */
                if(DECODER->sink_type == AUDIO_SINK_AV)
                {
                    /* connect SUB output from dsp, source port 2, to esco sink  */
                    if(codecData->sub_connection_port != DSP_L2CAP_SUB_PORT)
                    {
                        /* determine if the subwoofer has connected after the decoder was loaded,
                           configured and unmuted */
                        if(codecData->sub_is_available == FALSE)
                        {
                            /* mute the outputs whilst the subwoofer port is connected and
                               decoder restarted */
                            csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_ENABLE);
                        }

                        /* if not already connected */
                        if(codecData->sub_connection_port == DSP_ESCO_SUB_PORT)
                        {
                            /* wrong port connected, disconnect it first */
                            StreamDisconnect(StreamKalimbaSource(codecData->sub_connection_port), 0);
                            PRINT(("DECODER: Disconnect kalimba port %x\n", codecData->sub_connection_port));
                        }
    
                        /* ensure sink is valid */
                        if(SinkIsValid(sub_sink))
                        {
                            /* connect l2cap (3) port */
                            if(StreamConnect(StreamKalimbaSource(DSP_L2CAP_SUB_PORT),sub_sink))
                            {
                                /* update connected ports state */
                                codecData->sub_connection_port = DSP_L2CAP_SUB_PORT;
                                PRINT(("DECODER: Connect kalimba port %x\n", codecData->sub_connection_port));
                                /* set volume levels to desired level */
                                SubConnectedNowUnmuteVolume(DECODER);
                            }
                            else
                                PRINT(("DECODER: connect l2cap dsp port 3 FAILED\n" ));
                        }
                        else
                        {
                            PRINT(("DECODER: connect l2cap dsp port 3, sub_sink = %p NOT VALID\n", sub_sink ));
                        }
                        
                        /* determine if the subwoofer has connected after the decoder was loaded,
                           configured and unmuted */
                        if(codecData->sub_is_available == FALSE)
                        {
                            /* unmute the outputs again */
                            csrA2dpDecoderPluginOutputMute(multi_channel_group_all, AUDIO_MUTE_DISABLE);
                        }
                    }
                }
                else
                    PRINT(("DECODER: L2CAP - ****wrong link type****\n"));                                        
            break;
        }
    }
    else
        PRINT(("DECODER: CsrA2dpDecoderPluginSetSubWoofer ERROR NO DECODER\n" ));
}

/****************************************************************************
DESCRIPTION
    unmute the volume by message to allow a full dsp buffer to have been collected
    which will ensure a smooth ramping of the volume level
*/
void SubConnectedNowUnmuteVolume(DECODER_t * DECODER)
{
    A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;
    
    /* Ensure the volume is set before applying it, it may not yet have become available from the VM app */
    if(DECODER->volume.main.master != DIGITAL_VOLUME_MUTE || DECODER->volume.aux.master != DIGITAL_VOLUME_MUTE)
    {
        /* send the volume message to unmute the audio after a preset delay to allow
           a full dsp buffer to make use of the soft unmute function */
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG ) ;
    
        /* create volume message contents */
        memmove(message, &DECODER->volume, sizeof(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T));
        
        /* schedule in the unmute in 100ms*/
        MessageSendLater((TaskData *)DECODER->task, AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG, message , ALLOW_FULL_DSP_BUFFER_DELAY_FOR_SOFT_MUTE) ;
    }
    else
    {
        PRINT(("DECODER: SubConnectedNowUnmuteVolume - vol not set\n" ));
    }
    /* release the lock on changing volume and playing tones */
    codecData->delay_volume_message_sending = FALSE;
    
    PRINT(("DECODER: SubConnectedNowUnmuteVolume\n" ));
}
