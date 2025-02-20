/****************************************************************************
Copyright (c) 2012 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_subwoofer.h

DESCRIPTION
    Contains the internal plugin functions to handle audio routing for the 
    CSR subwoofer application
*/

#ifndef _CSR_SUBWOOFER_H_
#define _CSR_SUBWOOFER_H_

/* Subwoofer plugin includes */
#include "csr_subwoofer_plugin.h"

/* Firmware includes */
#include <sink.h>
#include <message.h>

/* The Kalimba input ports used by the subwoofer DSP application */
#define DSP_INPUT_PORT_ESCO     (0)
#define DSP_INPUT_PORT_L2CAP    (1)
#define DSP_INPUT_PORT_ADC      (2)

/* The Kalimba output ports used by the subwoofer DSP application */
#define DSP_OUTPUT_PORT_DAC     (0)
#define DSP_OUTPUT_PORT_I2S     (1)

/* Structure used to store the plugin data whilst it's in use */
typedef struct subwooferPluginData
{
    Sink    codec_sink;             /* Stores the codec sink in use */
    Source  audio_source;           /* Stores the input audio source in use */
    
    csrSubwooferInput   input;      /* Keeps track of the input in use (ADC/eSCO/L2CAP) */
    csrSubwooferOutput  output;     /* Keeps track of the output in use (DAC/I2S) */
    
    u16  swat_system_volume_db;  /* Soundbar system volume */
    u16  swat_trim_gain_db;      /* Trim gain from soundbar */
    u16  adc_volume_index;       /* Volume when ADC is connected */
    u16  sample_rate;            /* Sample rate used in L2CAP mode */
    u16  adc_sample_rate;        /* Sample rate for the ADC */
    u16  dsp_set_sample_rate;    /* sample rate that the dsp has been set to */
    
    Task    app_task;               /* Store the message handler for the application */
    Task    codec_task;             /* Store the codec task */

} subwooferPluginData;


/* Message structure for messages sent from the Kalimba to the plugin */
typedef struct
{
    u16 id;
    u16 a;
    u16 b;
    u16 c;
    u16 d;
} DSP_REGISTER_T;


/****************************************************************************
NAME
    CsrSubwooferPluginConnect

PARAMETERS
    audio_sink  The sink that contains the audio to plug into the DSP
    codec_task  The codec task (initialised by the application)
    app_task    The applications task
    params      Parameters required to plug the correct DSP ports

DESCRIPTION
    This function connects audio from the SOURCE to the SINK, where
        SOURCE is either:
            0 - ADC Input
            1 - ESCO Input (SWAT low latency)
            2 - L2CAP Input (SWAT standard latency)
        SINK is either:
            0 - DAC Output
            1 - I2S Output
*/                                     
void CsrSubwooferPluginConnect(Sink audio_sink, Task codec_task, Task app_task, subwooferPluginConnectParams * params);


/****************************************************************************
NAME
    CsrSubwooferPluginDisconnect

DESCRIPTION
    This function disconnects audio from the connected SOURCE to the 
    connected SINK
*/
void CsrSubwooferPluginDisconnect(void);


/****************************************************************************
NAME
    CsrSubwooferPluginSetVolume

DESCRIPTION
    This function sends the subwoofer volume level to the DSP application
*/
void CsrSubwooferPluginSetVolume(u16 volume);


/****************************************************************************
NAME
    CsrSubwooferPluginSetMode

DESCRIPTION
    This function is used to set the DSP mode
*/
void CsrSubwooferPluginSetMode(subwooferPluginModeParams * params);


/****************************************************************************
NAME
    CsrSubwooferPluginSetSampleRate

DESCRIPTION
    This function sends the sample rate to the DSP application
*/
void CsrSubwooferPluginSetSampleRate(u16 sample_rate);


/****************************************************************************
NAME
    handleInternalPluginMessage

DESCRIPTION
    Message handler for the internal plugin messages and messages sent from 
    the Kalimba application
*/
void handleInternalPluginMessage(Task task, MessageId id, Message message);

#endif /* _CSR_SUBWOOFER_H_ */
