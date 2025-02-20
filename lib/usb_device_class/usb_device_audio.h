/*******************************************************************************
Copyright (c) 2010 - 2016 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#ifndef USB_DEVICE_CLASS_REMOVE_AUDIO

#ifndef _USB_DEVICE_AUDIO_H
#define _USB_DEVICE_AUDIO_H


#define B_DESCRIPTOR_TYPE_CS_INTERFACE 0x24
#define B_DESCRIPTOR_SUB_TYPE_FORMAT   0x02
#define B_DESCRIPTOR_FORMAT_TYPE_I     0x01

#define B_INTERFACE_CLASS_AUDIO 0x01
#define B_INTERFACE_SUB_CLASS_AUDIOCONTROL 0x01
#define B_INTERFACE_SUB_CLASS_AUDIOSTREAMING 0x02
#define PR_PROTOCOL_UNDEFINED 0x00

#define SUPPORTED_FREQUENCIES 6
#define SAMPLE_RATE_48K ((u32) 48000)
#define SAMPLE_RATE_44K1 ((u32) 44100)
#define SAMPLE_RATE_32K ((u32) 32000)
#define SAMPLE_RATE_22K05 ((u32) 22050)
#define SAMPLE_RATE_16K ((u32) 16000)
#define SAMPLE_RATE_8K ((u32) 8000)
#define SAMPLE_RATE ((u32) 48000)

/*! Audio Device Class Request Types */
#define SET       0x00
#define GET       0x80

/*! Audio Device Class Request Codes */
#define CUR             0x01
#define MIN             0x02
#define MAX             0x03
#define RES             0x04
#define MEM             0x05

#define SET_CUR (SET | CUR)
#define SET_MIN (SET | MIN)
#define SET_MAX (SET | MAX)
#define SET_RES (SET | RES)
#define SET_MEM (SET | MEM)
#define GET_CUR (GET | CUR)
#define GET_MIN (GET | MIN)
#define GET_MAX (GET | MAX)
#define GET_RES (GET | RES)
#define GET_MEM (GET | MEM)

/*! Interface Sub-Types */
#define HEADER          0x01
#define INPUT_TERMINAL  0x02
#define OUTPUT_TERMINAL 0x03
#define MIXER_UNIT      0x04
#define SELECTOR_UNIT   0x05
#define FEATURE_UNIT    0x06
#define PROCESSING_UNIT 0x07
#define EXTENSION_UNIT  0x08

/*! Endpoint Control Selectors */
#define SAMPLING_FREQ_CONTROL (0x01)

/*! Feature Unit Control Selectors */
#define FU_CONTROL_UNDEFINED        0x00
#define MUTE_CONTROL                0x01
#define VOLUME_CONTROL              0x02
#define BASS_CONTROL                0x03
#define MID_CONTROL                 0x04
#define TREBLE_CONTROL              0x05
#define GRAPHIC_EQUALIZER_CONTROL   0x06
#define AUTOMATIC_GAIN_CONTROL      0x07
#define DELAY_CONTROL               0x08
#define BASS_BOOST_CONTROL          0x09
#define LOUDNESS_CONTROL            0x0A

/*! Helpers */
#define REQ_CS(req)         (req.wValue >> 8)
#define REQ_CN(req)         (req.wValue & 0xFF)
#define REQ_CODE(req)       (req.bRequest & 0x07)
#define REQ_UNIT(req)       (req.wIndex >> 8)
#define REQ_INTERFACE(req)  (req.wIndex & 0xFF)
#define REQ_IS_GET(req)     (req.bRequest & GET)

/*! Unit/Terminal IDs */
#define SPEAKER_IT  0x01
#define SPEAKER_FU  0x02
#define SPEAKER_OT  0x03
#define MIC_IT      0x04
#define MIC_FU      0x05
#define MIC_OT      0x06

/*! Audio Values */
#define CHANNEL_MASTER 0x00
#define CHANNEL_LEFT   0x01
#define CHANNEL_RIGHT  0x02

/*! 
    Audio Gain Limits - We support analogue gain from -45dB to 0dB. 
    Digital gain goes up to +21.5 but distorts audio so don't report it. 
    Firmware settings for analogue gain are:
    0   1     2   3     4   5     6   7     8   9   10   11 12 12 14 15
    Which correspond to gains (in dB) of:
    -45 -41.5 -39 -35.5 -33 -29.5 -27 -23.5 -21 -18 -15 -12 -9 -6 -3 0
    We report resolution of 3dB so we're always within .5dB of the truth:
    -45 -42   -39 -36   -33 -30   -27 -24   -21 -18 -15 -12 -9 -6 -3 0
*/
#define DEFAULT_SPEAKER_VOLUME_MAX              0x0000 /* +00.00 */
#define DEFAULT_SPEAKER_VOLUME_MIN              0xD200 /* -45.00 */
#define DEFAULT_SPEAKER_VOLUME_RESOLUTION       0x0300 /* +03.00 */
#define DEFAULT_SPEAKER_VOLUME_DEFAULT          0xD200 /* -45.00 */

#define DEFAULT_MICROPHONE_VOLUME_MAX           0x0000 /* +00.00 */
#define DEFAULT_MICROPHONE_VOLUME_MIN           0xD200 /* -45.00 */
#define DEFAULT_MICROPHONE_VOLUME_RESOLUTION    0x0300 /* +03.00 */
#define DEFAULT_MICROPHONE_VOLUME_DEFAULT       0x0000 /* +00.00 */

/*! Convert from gain to f/w gain setting */
#define VOL_SETTING(x) ((x/3)+15)

typedef struct
{
    u8 length;
    u8 type;
    u8 sub_type;
} usb_descriptor_header;

typedef struct
{
    usb_descriptor_header header;
    u8 type;
    u8 num_channels;
    u8 subframe_size;
    u8 resolution;
    u8 num_freq;
    u8 freq[SUPPORTED_FREQUENCIES*3];             
} usb_format_descriptor;


bool usbConfigureAudioDescriptors(usb_device_class_config config, const usb_device_class_audio_config* params);

bool usbConfigureAudioVolume(usb_device_class_config config, const usb_device_class_audio_volume_config* params);

bool usbEnumerateAudio(u16 usb_device_class);
        
Sink usbAudioMicSink(void);

Source usbAudioSpeakerSource(void);

void usbAudioGetLevels(u16* levels);

UsbInterface usbAudioGetInterfaceId(usb_device_class_get_value interface);

u32 usbAudioGetSpeakerSampleFreq(void);
u32 usbAudioGetMicSampleFreq(void);


#endif /* _USB_DEVICE_AUDIO_H */

#endif /* !USB_DEVICE_CLASS_REMOVE_AUDIO */
