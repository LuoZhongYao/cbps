/****************************************************************************
Copyright (c) 2013 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    audio_plugin_if.h
    
DESCRIPTION
	
*/

/*!
@file   cs_i2s_audio_plugin.h
@brief  Header file for the audio plugin interface.

    The parameters / enums here define the message interface used for the 
    i2s audio plugin
    
     
*/


#ifndef _CSR_I2S_AUDIO_PLUGIN_H_
#define _CSR_I2S_AUDIO_PLUGIN_H_

#include <library.h>
#include <power.h>
#include <stream.h>

typedef enum
{
    i2s_plugin_none_use_pskey = 0,
    i2s_plugin_ssm2518,
    i2s_plugin_future_expansion
    
}i2s_plugin_type;

typedef enum
{
    i2s_out_1_left = 0,
    i2s_out_1_right,
    i2s_out_2_left,
    i2s_out_2_right
} i2s_out_t;

/* PSKEY_USR35 I2S pskey configuration*/
typedef struct
{
    /* 0: pskey config, 1: SSM2518 device */
    unsigned plugin_type:8;
    /* master or slave operation */
    unsigned master_operation:8;
    /* left justified or i2s data */
    unsigned left_or_right_justified:4;
    /* justified delay by 1 bit */
    unsigned justified_bit_delay:4;   
    /* bits per sample */
    unsigned bits_per_sample:8;
    /* length of i2s configuration data pskey */
    unsigned i2s_configuration_command_pskey_length:8;
    /* number of individual i2c initialistion commands sent to i2s device */
    unsigned number_of_initialisation_cmds:8;   
    /* number of individual i2c volume commands sent to i2s device */
    unsigned number_of_volume_cmds:8;   
    /* offset in data of start of first volume command */
    unsigned volume_cmds_offset:8;
    /* number of individual i2c shutdown commands sent to an i2s device */
    unsigned number_of_shutdown_cmds:8;   
    /* offset in data of start of first shutdown command */
    unsigned shutdown_cmds_offset:8;
    /* specifies the number of bits of the volume setting in the i2c command, e.g. 8 or 16 */
    unsigned volume_no_of_bits:16;
    /* specifies the volume range maximum, volume sent from sink will be scaled to this range */
    unsigned volume_range_max:16;
    /* specifies the volume range minimum, volume sent from sink will be scaled to this range */
    unsigned volume_range_min:16;
    /* specifies whether to resample music rates for i2s output 0 = no resampling */
    unsigned music_resampling_frequency:16;
    /* specifies whether to resample voice rates for i2s output 0 = no resampling */
    unsigned voice_resampling_frequency:16;
    /* i2s master clock frequency scaling factor */
    unsigned master_clock_scaling_factor:16;
} i2s_init_config_t;

#define PACKET_LENGTH 0
#define PACKET_VOLUME_OFFSET 1
#define PACKET_I2C_ADDR 2
#define PACKET_DATA 3

#define I2S_MINIMUM_SUPPORTED_OUTPUT_SAMPLE_RATE    44100
#define I2S_FALLBACK_OUTPUT_SAMPLE_RATE             48000
#define I2S_NO_RESAMPLE                             0

/* PSKEY_USR36 I2S configuration data */
typedef struct
{ 
    /* data takes the form of:
       {
           u8 packet_length;
           u8 volume_level_offset_within_packet;   ***if applicable***
           u8 packet_data[packet_length];
       }
    */  
    u8 data[1];
} i2s_data_config_t;

/* configuration in pskey format */
typedef struct
{   
    /* plugin type and lenght of data definitions */
    i2s_init_config_t i2s_init_config;
    /* i2s pskey configuration data */
    i2s_data_config_t i2s_data_config;
    /* don't add anything here, it will get overwritten */    
}I2SConfiguration;
    
    
/****************************************************************************
DESCRIPTION: CsrI2SInitialisePlugin :

    This function gets a pointer to the application malloc'd slot
    containing the i2s configuration data

PARAMETERS:
    
    pointer to malloc'd slot
    
RETURNS:
    none
*/
void CsrI2SInitialisePlugin(I2SConfiguration * config);


/****************************************************************************
DESCRIPTION: CsrI2SAudioOutputConnectConfigureSink :

    This function configures the I2S interface ready for a connection.

PARAMETERS:
    
    Sink sink   - The Sink to configure
    u32 rate - Sample rate of the data coming from the DSP

RETURNS:
    Nothing.
*/
void CsrI2SAudioOutputConnectConfigureSink(Sink sink, u32 rate);


/****************************************************************************
DESCRIPTION: CsrI2SAudioOutputConnect :

    This function configures the I2S interface and connects the audio streams 
    from the dsp to I2S external hardware.

PARAMETERS:
    
    u32 rate - sample rate of data coming from dsp
    bool   stereo - indicates whether to connect left or left and right channels
    Source dsp_left_port - audio stream from dsp for the left channel audio
    Source dsp_right_port - audio stream from dsp for the left channel audio

RETURNS:
    Sink sink to the left channel
*/
Sink CsrI2SAudioOutputConnect(u32 rate, bool stereo, Source left_port, Source right_port);

/****************************************************************************
DESCRIPTION::

    This function disconnects the audio streams from the dsp to I2S external hardware.

PARAMETERS:
    
    bool   stereo - indicates whether to connect left or left and right channels

RETURNS:
    none
*/
void CsrI2SAudioOutputDisconnect(bool stereo);

/****************************************************************************
DESCRIPTION: CsrI2SAudioOutputSetVolume :

    This function sets the volume level of the I2S external hardware if supported
    by the device being used.

PARAMETERS:
    
    bool   stereo - indicates whether to connect left or left and right channels

RETURNS:
    none
*/
void CsrI2SAudioOutputSetVolume(bool stereo, i16 left_volume, i16 right_volume, bool volume_in_dB);

/****************************************************************************
DESCRIPTION: CsrI2SAudioOutputConnectAdpcm :

    This function configures the I2S interface and connects the audio streams 
    from the dsp to I2S external hardware.

PARAMETERS:
    
    u32 rate - sample rate of data coming from dsp
    bool   stereo - indicates whether to connect left or left and right channels
    Source dsp_left_port - audio stream from dsp for the left channel audio
    

RETURNS:
    sink
*/
Sink CsrI2SAudioOutputConnectAdpcm(u32 rate, bool stereo, Source left_port);

/****************************************************************************
DESCRIPTION: CsrInitialiseI2SDevice :

    This function configures the I2S device 

PARAMETERS:
    
    u32 sample_rate - sample rate of data coming from dsp

RETURNS:
    none
*/    
void CsrInitialiseI2SDevice(u32 sample_rate);

/****************************************************************************
DESCRIPTION: CsrShutdownI2SDevice :

    This function shuts down the I2S device 

PARAMETERS:
    
    none
    
RETURNS:
    none
*/    
void CsrShutdownI2SDevice(void);

/******************************************************************************
DESCRIPTION: CsrSetVolumeI2SChannel

    This function sets the volume of a single I2S channel on a specific I2S
    device via the I2C interface. The volume can either be passed in as a value
    in 1/60th's of dB (with range -7200 to 0), or as an absolute value in the 
    same format used by the CODEC plugin for compatibility (range 0x0 - 0xf).

PARAMETERS:
    i2s_out_t channel   The I2S device and channel to set the volume of.
    i16 volume        The volume level required, in dB/60 or CODEC_STEPS.
    bool volume_in_dB   Set to TRUE if volume passed in dB, FALSE otherwise.

RETURNS:
    Whether volume was successfully changed for the requested device channel.
*/
bool CsrSetVolumeI2SChannel(i2s_out_t channel, i16 vol, bool volume_in_dB);

/******************************************************************************
DESCRIPTION: CsrSetVolumeI2SDevice

    This function sets the primary I2S device volume via the I2C interface, for
    use in stereo mode.

PARAMETERS:
    i16 left_vol    - Volume level for primary left channel.
    i16 right_vol   - Volume level for primary left channel.
    bool volume_in_dB - Whether the volume is passed in in dB or CODEC_STEPS.

RETURNS:
    none
*/
void CsrSetVolumeI2SDevice(i16 left_vol, i16 right_vol, bool volume_in_dB);

/****************************************************************************
DESCRIPTION: CsrI2SMusicResamplingFrequency 

    This function returns the current resampling frequency for music apps,

PARAMETERS:
    
    none

RETURNS:
    frequency or 0 indicating no resampling required
*/    
u16 CsrI2SMusicResamplingFrequency(void);

/****************************************************************************
DESCRIPTION: CsrI2SVoiceResamplingFrequency 

    This function returns the current resampling frequency for voice apps,

PARAMETERS:
    
    none

RETURNS:
    frequency or 0 indicating no resampling required
*/    
u16 CsrI2SVoiceResamplingFrequency(void);

/****************************************************************************
DESCRIPTION: CsrI2SAudioInputConnect 

    This function configures and connects the I2S to the dsp input ports 

PARAMETERS:
    
    none

RETURNS:
    none
*/    
void CsrI2SAudioInputConnect(u32 rate, bool stereo, Sink left_port, Sink right_port );

/****************************************************************************
DESCRIPTION: CsrI2SMasterIsEnabled 

    This function returns the I2S operation mode 

PARAMETERS:
    
    none

RETURNS:
    
    TRUE : Operation mode is I2S Master
    FALSE : Operation mode is I2S Slave 
    
*/    
bool CsrI2SMasterIsEnabled(void);

u16 CsrI2SGetOutputResamplingFrequencyForI2s(const u16 requested_rate);
        
#endif
