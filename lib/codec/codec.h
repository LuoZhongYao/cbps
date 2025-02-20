/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    codec.h
    
DESCRIPTION 

    Header file for the codec library.  This library implements the
    functionality to be able to configure a stereo codec for use. Can define if
    the stereo inputs or outputs of the codec needs to be used, and the sample
    rates to be used for the ADCs and DACs.
	
	The library exposes a functional downstream API and an upstream message
	based API.
*/
/*!
    @file   codec.h  

    @brief Header file for the codec library.  This library implements the
    functionality to be able to configure a stereo codec for use. Can define if
    the stereo inputs or outputs of the codec needs to be used, and the sample
    rates to be used for the ADCs and DACs.
 
    The library exposes a functional downstream API and an upstream message
    based API.
*/
#ifndef CODEC_H_
#define CODEC_H_

#include <stream.h>
#include <source.h>
#include <sink.h>
#include <codec_.h>
#include <message_.h>
#include <library.h>

#include "audio_plugin_if.h"

/*!
    @brief The codec structures.
*/
typedef struct __CsrInternalCodecTaskData CsrInternalCodecTaskData;

typedef struct
{
    i16 dsp_db_scaled;
    u8 dac_gain;
}hybrid_gains_t;

typedef struct __volume_group_config_t
{
	i16 no_of_steps;         /* number of steps of volume change permitted */
	i16 volume_knee_value_1;  /* volume point at which curve of dB conversion changes */  
	i16 volume_knee_value_2;  /* volume point at which curve of dB conversion changes */
	i16 dB_knee_value_1;      /* dB value for point at which curve of dB conversion changes */
	i16 dB_knee_value_2;      /* dB value for point at which curve of dB conversion changes */
	i16 dB_max;               /* dB value at maximum volume level */
	i16 dB_min;               /* dB value at minimum volume level */
} volume_group_config_t;

typedef struct
{
	volume_group_config_t group_config[multi_channel_group_all];

    i16 device_trim_master;   /* TWS master device trim gain */
    i16 device_trim_slave;    /* TWS slave device trim gain */
    i16 device_trim_change;   /* TWS device trim step change */
    i16 device_trim_min;      /* TWS device trim minimum gain */
    i16 device_trim_max;      /* TWS device trim maximum gain */

} dsp_volume_user_config;

#define CODEC_STEPS 15          /* number of DAC steps */
#define DB_TO_DAC   180         /* DAC steps of 3 dB * scaling factor of dsp volume control which is 60 */
#define DB_DSP_SCALING_FACTOR 60/* DSP dB values are multiplied by 60 */ 
#define MIN_CODEC_GAIN_DB   -45
#define MAXIMUM_DIGITAL_VOLUME_0DB 0
#define MINIMUM_DIGITAL_VOLUME_80dB (-80 * DB_DSP_SCALING_FACTOR)
#define DSP_VOICE_PROMPTS_LEVEL_ADJUSTMENT (-6 * DB_DSP_SCALING_FACTOR) /* voice prompts played through the dsp require 6 dB of attenuation */
#define DIGITAL_VOLUME_MUTE (-120 * DB_DSP_SCALING_FACTOR) /* mute is -120dB */

/*!
    @brief Defines used to indicate Codec Gain Ranges
*/

#define CODEC_INPUT_GAIN_RANGE      22
#define CODEC_OUTPUT_GAIN_RANGE     22

/*!
    @brief Defines used to configure outputs in codec_config_params struct. 
   
    On some codecs you can feed the input directly to the output without any
    digital signal processing.
*/
#define OUTPUT_NONE			0x0
#define OUTPUT_DAC			0x1
#define OUTPUT_MICIN		0x2
#define OUTPUT_LINEIN		0x4

/*!
    @brief The upstream codec messages 
*/
typedef enum
{
	CODEC_INIT_CFM 		= 	CODEC_MESSAGE_BASE,
    
	CODEC_CONFIGURE_CFM ,
    
    CODEC_MESSAGE_TOP
} CodecMessageId;

/*! 
	@brief The codecs channel being referred to, left, right or both.
*/
typedef enum
{
	/*! The left channel of the codec.*/
	left_ch,						
	/*! The right channel of the codec.*/
	right_ch,						
	/*! The left and right channel of the codec. */
	left_and_right_ch				
} codec_channel;

/*!
    @brief The status codes returned to the client application to indicate the
    status of a requested operation.
*/
typedef enum
{
	/*! The requested codec operation was a success. */ 
	codec_success = (0),           
	/*! The requested codec operation was a failure. */ 
	codec_fail,                    
	/*! The requested operation supplied invalid sample rates for this
	  codec. */ 
	codec_invalid_sample_rates,    
	/*! The requested operation supplied an invalid configuration. */ 
	codec_invalid_configuration,   
	/*! The requested operation was not supported for this codec. */ 
	codec_not_supported            
} codec_status_code;

/*!
    @brief The possible sample rates that the ADCs and DACs can be configured
    to.
*/
typedef enum
{
	sample8kHz = (0),
			
	sample11_025kHz,
	sample16kHz,
	sample22_25kHz,
	sample24kHz,
	
	sample32kHz,
	sample44_1kHz,
	sample48kHz,
	sample88_2kHz,
	sample96kHz,
	/* Leave sampleNotUsed as the last enum item */
	sampleNotUsed
} sample_freq;

/*!
    @brief Type of input to the codec ADCs that will be used.
*/
typedef enum
{
	/*! The input to the codec is microphone. */ 
	mic_input,						
	/*! The input to the codec is line. */ 
	line_input,						
	/*! No input to the codec. */ 
	no_input        /* Leave no_input as the last enum item */
} input_type;

/*!
    @brief The configuration used to set up the codec. 
    
    Define type of inputs and outputs, and the sample rates to use for the ADCs
    and DACs.
*/
typedef struct
{
	/*! Type of input to use with ADCs. */
	input_type inputs;				
	/*! Can output one or more of DACS (0x1), mic in (0x2), line in (0x4). */
	u16 outputs;					
	/*! The sample rate to use for the ADC. */
	sample_freq adc_sample_rate;	
	/*! The sample rate to use for the DAC. */
	sample_freq dac_sample_rate;	
} codec_config_params;

/*!
    @brief This message returns the result of a call to CodecInitXXX.
	
    NOTE: inputGainRange and outputGainRange are set by the defines CODEC_INPUT_GAIN_RANGE and
    CODEC_OUTPUT_GAIN_RANGE. 
*/
typedef struct
{
	codec_status_code status;      /*!< The current codec status. */
	u16 inputGainRange;         /*!< The max input gain for the codec. */
	u16 outputGainRange;        /*!< The max output gain for the codec. */
	Task codecTask;                /*!< The codec task. */
} CODEC_INIT_CFM_T;

/*!
    @brief This message returns the result of a call to CodecConfigure.
*/
typedef struct
{
	codec_status_code status;		/*!< The current codec status. */
} CODEC_CONFIGURE_CFM_T;
	
    
struct __CsrInternalCodecTaskData
{
	TaskData		       task;
	Task			       clientTask;
};

/*!
    @brief Initialise the CSR Internal Codec.
   
    @param codec Pointer to CsrInternalCodecTaskData structure
    Caller is responsible for allocation of this struct
    @param appTask The current application task.
   
    CODEC_INIT_CFM message will be received by the application. 
*/
void CodecInitCsrInternal(CsrInternalCodecTaskData* codec, Task appTask);

/*!
    @brief Configure the Codec with the supplied parameters.
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.

    @param config The configuration parameters to setup the codec with.
   
    CODEC_CONFIGURE_CFM message will be received by the application. 
*/
void CodecConfigure(Task codecTask, const codec_config_params *config);

/*!
    @brief Set Codec Input Gain, left channel. 
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.

    @param volume The gain level (volume) to set the input channel to.

    @param channel The channel to use.
*/
void CodecSetInputGain(Task codecTask, u16 volume, codec_channel channel);


/*!
    @brief Update the codec input gain immediately. 
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.

    @param volume The gain level (volume) to set the input channel to.

    @param channel The channel to use.
*/
void CodecSetInputGainNow(Task codecTask, u16 volume, codec_channel channel);


/*!
    @brief Set Codec Output Gain. 
 
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.

    @param volume The gain level (volume) to set the output left channel to.

    @param channel The channel to use.
*/
void CodecSetOutputGain(Task codecTask, u16 volume, codec_channel channel);


/*!
    @brief Update the codec output gain immediately. 
 
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.

    @param volume The gain level (volume) to set the output left channel to.

    @param channel The channel to use.
*/
void CodecSetOutputGainNow(Task codecTask, u16 volume, codec_channel channel);


/*!
    @brief Enable the codec. 
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.
*/
void CodecEnable(Task codecTask);

/*!
    @brief Disable the codec. 
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.
*/
void CodecDisable(Task codecTask);

/*!
    @brief Power down the codec. 
   
    @param codecTask The codec task that was returned with the CODEC_INIT_CFM
    message.
*/
void CodecPowerDown(Task codecTask);

/****************************************************************************
NAME
 VolumeConvertDACGainToDB

DESCRIPTION
 Converts dac gain to dB value

RETURNS
 dB value

*/
i16 VolumeConvertDACGainToDB(u16 DAC_Gain);

/****************************************************************************
NAME
 VolumeConvertStepsToDB

DESCRIPTION
 Converts decimal step values into dB values for DSP volume control scheme

RETURNS
 dB value

*/
i16 VolumeConvertStepsToDB(i16 Volume, volume_group_config_t * volMappingConfig);

/****************************************************************************
NAME
 codecCalcHybridValues

DESCRIPTION
 Converts decimal step values into dB values for DSP volume control scheme

RETURNS
 dB value

*/
void CodecCalcHybridValues(hybrid_gains_t * gains, i16 volume);

/****************************************************************************
NAME
 CodecSetOutputGainLater

DESCRIPTION
 delayed setting of the DAC gains

RETURNS
 none

*/
void CodecSetOutputGainLater(Task codecTask, u16 volume, codec_channel channel, u16 delay);

#endif /* CODEC_H_ */



