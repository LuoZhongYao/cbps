/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_usb.c

DESCRIPTION
    Application level implementation of USB features

NOTES
    - Conditional on ENABLE_USB define
    - Ensure USB host interface and VM control of USB enabled in project/PS
*/

#include <string.h>

#ifdef ENABLE_USB

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_powermanager.h"
#include "sink_usb.h"
#include "sink_configmanager.h"
#include "sink_audio.h"
#include "sink_a2dp.h"
#include "sink_avrcp.h"
#include "sink_statemanager.h"
#include "sink_pio.h"
#include "sink_tones.h"
#include "sink_display.h"
#include "sink_audio_routing.h"
#include "sink_volume.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include "sink_config.h"

#include <usb_device_class.h>
#include <power.h>
#include <panic.h>
#include <print.h>
#include <usb.h>
#include <file.h>
#include <stream.h>
#include <source.h>
#include <boot.h>
#include <charger.h>

#ifdef ENABLE_USB_AUDIO

#include "sink_usb_descriptors.h"

#include <csr_a2dp_decoder_common_plugin.h>
/* Reduced to 20ms to minimize delay before USB audio is heard */
#define USB_AUDIO_DISCONNECT_DELAY (20)

/* Interface Descriptors for mono 8kHz. Do Not Modify. */
static const u8 interface_descriptor_control_mic_and_speaker[] =
{
    /* Class Specific Header */
    0x0A,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x01,         /* bDescriptorSubType = HEADER */
    0x00, 0x01,   /* bcdADC = Audio Device Class v1.00 */
    0x0A + 0x0c + 0x0b + 0x09 + 0x0c + 0x0b + 0x09, /* wTotalLength LSB */
    0x00,         /* wTotalLength MSB */
    0x02,         /* bInCollection = 2 AudioStreaming interfaces */
    0x01,         /* baInterfaceNr(1) - AS#1 id */
    0x02,         /* baInterfaceNr(2) - AS#2 id */

    /* Microphone IT */
    0x0c,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = INPUT_TERMINAL */
    MIC_IT,       /* bTerminalID */
    0x03, 0x02,   /* wTerminalType = Personal Microphone */
    0x00,         /* bAssocTerminal = none */
    0x01,         /* bNrChannels = 1 */
    0x00, 0x00,   /* wChannelConfig = mono */
    0x00,         /* iChannelName = no string */
    0x00,         /* iTerminal = same as USB product string */

    /* Microphone Features */
    0x0b,           /*bLength*/
    0x24,           /*bDescriptorType = CS_INTERFACE */
    0x06,           /*bDescriptorSubType = FEATURE_UNIT*/
    MIC_FU,         /*bUnitId*/
    MIC_IT,         /*bSourceId - Microphone IT*/
    0x02,           /*bControlSize = 2 bytes per control*/
    0x01, 0x00,     /*bmaControls[0] = 0001 (Mute on Master Channel)*/
    0x00, 0x00,     /*bmaControls[1] = 0000 (No gain control)*/
    0x00,           /*iFeature = same as USB product string*/

    /* Microphone OT */
    0x09,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x03,         /* bDescriptorSubType = OUTPUT_TERMINAL */
    MIC_OT,       /* bTerminalID */
    0x01, 0x01,   /* wTerminalType = USB streaming */
    0x00,         /* bAssocTerminal = none */
    MIC_FU,       /* bSourceID - Microphone Features */
    0x00,         /* iTerminal = same as USB product string */

    /* Speaker IT */
    0x0c,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = INPUT_TERMINAL */
    SPEAKER_IT,   /* bTerminalID */
    0x01, 0x01,   /* wTerminalType = USB streaming */
    0x00,         /* bAssocTerminal = none */
    0x01,         /* bNrChannels = 1 */
    0x00, 0x00,   /* wChannelConfig = mono */
    0x00,         /* iChannelName = no string */
    0x00,         /* iTerminal = same as USB product string */

    /* Speaker Features */
    0x0b,           /*bLength*/
    0x24,           /*bDescriptorType = CS_INTERFACE */
    0x06,           /*bDescriptorSubType = FEATURE_UNIT*/
    SPEAKER_FU,     /*bUnitId*/
    SPEAKER_IT,     /*bSourceId - Speaker IT*/
    0x02,           /*bControlSize = 2 bytes per control*/
    0x01, 0x00,     /*bmaControls[0] = 0001 (Mute on Master Channel)*/
    0x02, 0x00,     /*bmaControls[1] = 0002 (Vol on Left Front)*/
    0x00,           /*iFeature = same as USB product string*/

    /* Speaker OT */
    0x09,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x03,         /* bDescriptorSubType = OUTPUT_TERMINAL */
    SPEAKER_OT,   /* bTerminalID */
    0x01, 0x03,   /* wTerminalType = Speaker */
    0x00,         /* bAssocTerminal = none */
    SPEAKER_FU,   /* bSourceID - Speaker Features*/
    0x00,         /* iTerminal = same as USB product string */
};

static const u8 interface_descriptor_streaming_mic[] =
{
    /* Class Specific AS interface descriptor */
    0x07,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    MIC_OT,       /* bTerminalLink = Microphone OT */
    0x00,         /* bDelay */
    0x01, 0x00,   /* wFormatTag = PCM */

    /* Type 1 format type descriptor */
    0x08 + 0x03,  /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = FORMAT_TYPE */
    0x01,         /* bFormatType = FORMAT_TYPE_I */
    0x01,         /* bNumberOfChannels */
    0x02,         /* bSubframeSize = 2 bytes */
    0x10,         /* bBitsResolution */
    0x01,         /* bSampleFreqType = 1 discrete sampling freq */
    0xFF & (SAMPLE_RATE_CVC),       /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC >> 8),  /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC >> 16), /* tSampleFreq */

    /* Class specific AS isochronous audio data endpoint descriptor */
    0x07,         /* bLength */
    0x25,         /* bDescriptorType = CS_ENDPOINT */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    0x00,         /* bmAttributes = none */
    0x02,         /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00     /* wLockDelay */
};

static const u8 interface_descriptor_streaming_speaker[] =
{
    /* Class Specific AS interface descriptor */
    0x07,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    SPEAKER_IT,   /* bTerminalLink = Speaker IT */
    0x00,         /* bDelay */
    0x01, 0x00,   /* wFormatTag = PCM */

    /* Type 1 format type descriptor */
    0x08 + 0x03,/* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = FORMAT_TYPE */
    0x01,         /* bFormatType = FORMAT_TYPE_I */
    0x01,         /* bNumberOfChannels */
    0x02,         /* bSubframeSize = 2 bytes */
    0x10,         /* bBitsResolution */
    0x01,         /* bSampleFreqType = 1 discrete sampling freq */
    0xFF & (SAMPLE_RATE_CVC),       /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC >> 8),  /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC >> 16), /* tSampleFreq */

    /* Class specific AS isochronous audio data endpoint descriptor */
    0x07,         /* bLength */
    0x25,         /* bDescriptorType = CS_ENDPOINT */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    0x81,         /* bmAttributes = MaxPacketsOnly and SamplingFrequency control */
    0x02,         /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00    /* wLockDelay */
};

static const u8 audio_endpoint_user_data[] =
{
    0, /* bRefresh */
    0  /* bSyncAddress */
};


/*  Streaming Isochronous Endpoint. Maximum packet size 16 (mono at 8khz) */
static const EndPointInfo epinfo_streaming_speaker[] =
{
    {
        end_point_iso_in, /* address */
        end_point_attr_iso, /* attributes */
        16, /* max packet size */
        1, /* poll_interval */
        audio_endpoint_user_data, /* data to be appended */
        sizeof(audio_endpoint_user_data) /* length of data appended */
    }
};


/* Streaming Isochronous Endpoint. Maximum packet size 16 (mono at 8khz) */
static const EndPointInfo epinfo_streaming_mic[] =
{
    {
        end_point_iso_out, /* address */
        end_point_attr_iso, /* attributes */
        16, /* max packet size */
        1, /* poll_interval */
        audio_endpoint_user_data, /* data to be appended */
        sizeof(audio_endpoint_user_data), /* length of data appended */
    }
};

static const usb_device_class_audio_config usb_cvc_config =
{
    {interface_descriptor_control_mic_and_speaker,
    sizeof(interface_descriptor_control_mic_and_speaker),
    NULL},
    {interface_descriptor_streaming_mic,
    sizeof(interface_descriptor_streaming_mic),
    epinfo_streaming_mic},
    {interface_descriptor_streaming_speaker,
    sizeof(interface_descriptor_streaming_speaker),
    epinfo_streaming_speaker}
};

static const u8 interface_descriptor_streaming_mic_wb[] =
{
    /* Class Specific AS interface descriptor */
    0x07,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    MIC_OT,       /* bTerminalLink = Microphone OT */
    0x00,         /* bDelay */
    0x01, 0x00,   /* wFormatTag = PCM */

    /* Type 1 format type descriptor */
    0x08 + 0x03,  /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = FORMAT_TYPE */
    0x01,         /* bFormatType = FORMAT_TYPE_I */
    0x01,         /* bNumberOfChannels */
    0x02,         /* bSubframeSize = 2 bytes */
    0x10,         /* bBitsResolution */
    0x01,         /* bSampleFreqType = 1 discrete sampling freq */
    0xFF & (SAMPLE_RATE_CVC_WB),       /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC_WB >> 8),  /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC_WB >> 16), /* tSampleFreq */

    /* Class specific AS isochronous audio data endpoint descriptor */
    0x07,         /* bLength */
    0x25,         /* bDescriptorType = CS_ENDPOINT */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    0x00,         /* bmAttributes = none */
    0x02,         /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00     /* wLockDelay */
};

static const u8 interface_descriptor_streaming_speaker_wb[] =
{
    /* Class Specific AS interface descriptor */
    0x07,         /* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    SPEAKER_IT,   /* bTerminalLink = Speaker IT */
    0x00,         /* bDelay */
    0x01, 0x00,   /* wFormatTag = PCM */

    /* Type 1 format type descriptor */
    0x08 + 0x03,/* bLength */
    0x24,         /* bDescriptorType = CS_INTERFACE */
    0x02,         /* bDescriptorSubType = FORMAT_TYPE */
    0x01,         /* bFormatType = FORMAT_TYPE_I */
    0x01,         /* bNumberOfChannels */
    0x02,         /* bSubframeSize = 2 bytes */
    0x10,         /* bBitsResolution */
    0x01,         /* bSampleFreqType = 1 discrete sampling freq */
    0xFF & (SAMPLE_RATE_CVC_WB),       /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC_WB >> 8),  /* tSampleFreq */
    0xFF & (SAMPLE_RATE_CVC_WB >> 16), /* tSampleFreq */

    /* Class specific AS isochronous audio data endpoint descriptor */
    0x07,         /* bLength */
    0x25,         /* bDescriptorType = CS_ENDPOINT */
    0x01,         /* bDescriptorSubType = AS_GENERAL */
    0x81,         /* bmAttributes = MaxPacketsOnly and SamplingFrequency control */
    0x02,         /* bLockDelayUnits = Decoded PCM samples */
    0x00, 0x00    /* wLockDelay */
};

/*  Streaming Isochronous Endpoint. Maximum packet size 16 (mono at 8khz) */
static const EndPointInfo epinfo_streaming_speaker_wb[] =
{
    {
        end_point_iso_in, /* address */
        end_point_attr_iso, /* attributes */
        32, /* max packet size */
        1, /* poll_interval */
        audio_endpoint_user_data, /* data to be appended */
        sizeof(audio_endpoint_user_data) /* length of data appended */
    }
};


/* Streaming Isochronous Endpoint. Maximum packet size 16 (mono at 8khz) */
static const EndPointInfo epinfo_streaming_mic_wb[] =
{
    {
        end_point_iso_out, /* address */
        end_point_attr_iso, /* attributes */
        32, /* max packet size */
        1, /* poll_interval */
        audio_endpoint_user_data, /* data to be appended */
        sizeof(audio_endpoint_user_data), /* length of data appended */
    }
};

static const usb_device_class_audio_config usb_cvc_wb_config =
{
    {interface_descriptor_control_mic_and_speaker,
    sizeof(interface_descriptor_control_mic_and_speaker),
    NULL},
    {interface_descriptor_streaming_mic_wb,
    sizeof(interface_descriptor_streaming_mic_wb),
    epinfo_streaming_mic_wb},
    {interface_descriptor_streaming_speaker_wb,
    sizeof(interface_descriptor_streaming_speaker_wb),
    epinfo_streaming_speaker_wb}
};

static const usb_device_class_audio_volume_config usb_stereo_audio_volume =
{
    SPEAKER_VOLUME_MIN,
    SPEAKER_VOLUME_MAX,
    SPEAKER_VOLUME_RESOLUTION,
    SPEAKER_VOLUME_DEFAULT,
    MICROPHONE_VOLUME_MIN,
    MICROPHONE_VOLUME_MAX,
    MICROPHONE_VOLUME_RESOLUTION,
    MICROPHONE_VOLUME_DEFAULT
};

static const usb_plugin_info usb_plugins[] =
{
    {SAMPLE_RATE_STEREO, NULL                , usb_plugin_stereo},
    {SAMPLE_RATE_CVC,    &usb_cvc_config     , usb_plugin_mono_nb},
    {SAMPLE_RATE_CVC_WB, &usb_cvc_wb_config  , usb_plugin_mono_wb}
};


#endif /* ENABLE_USB_AUDIO */

#ifdef DEBUG_USB
#else
#endif

typedef struct
{
    FILE_INDEX  index;
    u32      size;
} usb_file_info;

#define USB_NAME_SIZE 8
#define USB_EXT_SIZE  3

typedef struct
{
    char name[USB_NAME_SIZE];
    char ext[USB_EXT_SIZE];
} usb_file_name;

typedef struct
{
    char  name[USB_NAME_SIZE + USB_EXT_SIZE + 1];
    u8 size;
} usb_file_name_info;

#define USB_CLASS_ENABLED(x)        ((bool)(theSink.usb->config.device_class & (x)))
#define USB_CLASS_DISABLE(x)        theSink.usb->config.device_class &= ~(x)
#define USB_CLASS_ENABLE(x)         theSink.usb->config.device_class |= (x)

#ifdef HAVE_VBAT_SEL
#define usbDeadBatteryAtBoot()      (ChargerGetBatteryStatusAtBoot() == CHARGER_BATTERY_DEAD)
#else
#define usbDeadBatteryAtBoot()      (TRUE) /* Assume dead battery until initial VBAT reading taken */
#endif
#define usbDeadBatteryProvision()   (theSink.usb->dead_battery && !theSink.usb->enumerated && !theSink.usb->deconfigured)

const char root_name[] = "usb_root";
const char fat_name[]  = "usb_fat";

/* Define some easier to read values... */
#define USB_PLAY_PAUSE  USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_PLAY_PAUSE
#define USB_STOP        USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_STOP
#define USB_FWD         USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_NEXT_TRACK
#define USB_BCK         USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_PREVIOUS_TRACK
#define USB_VOL_UP      USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_VOL_UP
#define USB_VOL_DN      USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_VOL_DOWN
#define USB_MUTE        USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_MUTE

static void sinkUsbReadConfig(void);
static void sinkUsbVolumeEvent(sinkEvents_t event);

#ifdef ENABLE_USB_AUDIO
static void usbAudioSetVolume(void);
#else
#define usbAudioSetVolume() ((void)(0))
#endif

#ifdef ENABLE_USB_AUDIO
static void usbAudioSetSpeakerSampleRate(u16 sample_rate);
#else
#define usbAudioSetSpeakerSampleRate(x) ((void)0)
#endif

#ifdef ENABLE_USB_AUDIO
static void usbAudioSetMicSampleRate(u16 sample_rate);
#else
#define usbAudioSetMicSampleRate(x) ((void)0)
#endif

#ifdef ENABLE_USB_AUDIO
static void usbSendHidEvent(usb_device_class_event event);
#else
#define usbSendHidEvent(x) ((void)(0))
#endif

#define usbPlayPause()  usbSendHidEvent(USB_PLAY_PAUSE)
#define usbStop()       usbSendHidEvent(USB_STOP)
#define usbFwd()        usbSendHidEvent(USB_FWD)
#define usbBck()        usbSendHidEvent(USB_BCK)
#define usbVolUp()      usbSendHidEvent(USB_VOL_UP)
#define usbVolDn()      usbSendHidEvent(USB_VOL_DN)
#define usbMute()       usbSendHidEvent(USB_MUTE)

#ifdef ENABLE_USB_AUDIO
#define sinkUsbVolumeIsAtMinimum()  (theSink.usb->out_l_vol == (i16)(i8)((i16)SPEAKER_VOLUME_MIN >> 8))
#else
#define sinkUsbVolumeIsAtMinimum()  (FALSE)
#endif

#ifdef ENABLE_USB_AUDIO
#define sinkUsbVolumeIsAtMaximum()  (theSink.usb->out_l_vol == (i16)(i8)((i16)SPEAKER_VOLUME_MAX >> 8))
#else
#define sinkUsbVolumeIsAtMaximum()  (FALSE)
#endif

/*************************************************************************
NAME
    usbSetAudioSuspendState

DESCRIPTION
    Sets the audio suspend state for USB

RETURNS
    None

**************************************************************************/
static void usbSetAudioSuspendState (usb_audio_suspend_state state)
{
    if (state == usb_audio_suspend_none)
    {   /* Returning to the unsuspended state */
        theSink.usb->audio_suspend_state = state;
#ifdef ENABLE_PEER
        peerClaimRelay(TRUE);
#endif
    }
    else
    {   /* Check if we are already suspended before updating state */
        if (theSink.usb->audio_suspend_state == usb_audio_suspend_none)
        {
            theSink.usb->audio_suspend_state = state;
#ifdef ENABLE_PEER
            peerClaimRelay(FALSE);
#endif
        }
    }

    LOGD("USB: Setting audio suspend_state = %d\n",theSink.usb->audio_suspend_state);
}


/****************************************************************************
NAME
    usbUpdateChargeCurrent

DESCRIPTION
    Set the charge current based on USB state

RETURNS
    void
*/
void usbUpdateChargeCurrent(void)
{
    /* Don't change anything if battery charging disabled */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        powerManagerUpdateChargeCurrent();
}


/****************************************************************************
NAME
    usbSetLowPowerMode

DESCRIPTION
    If delay is non zero queue a message to reset into low powered mode. If
    delay is zero do nothing.

RETURNS
    void
*/
static void usbSetLowPowerMode(u8 delay)
{
    /* Only queue low power mode if not enumerated and attached to normal host/hub */
    if(!theSink.usb->enumerated && delay && (UsbAttachedStatus() == HOST_OR_HUB))
    {
        LOGD("USB: Queue low power in %d sec\n", delay);
        MessageSendLater(&theSink.task, EventUsrUsbLowPowerMode, 0, D_SEC(delay));
    }
}


/****************************************************************************
NAME
    usbSetBootMode

DESCRIPTION
    Set the boot mode to default or low power

RETURNS
    void
*/
void usbSetBootMode(u8 bootmode)
{
    /* Don't change anything if battery charging disabled */
    if(!USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        return;

    if(BootGetMode() != bootmode)
    {
        LOGD("USB: Set Mode %d\n", bootmode);
        BootSetMode(bootmode);
    }
}


/****************************************************************************
NAME
    handleUsbMessage

DESCRIPTION
    Handle firmware USB messages

RETURNS
    void
*/
void handleUsbMessage(Task task, MessageId id, Message message)
{
    LOGD("USB: ");
    switch (id)
    {
        case MESSAGE_USB_ATTACHED:
        {
            LOGD("MESSAGE_USB_ATTACHED\n");
            usbUpdateChargeCurrent();
            audioHandleRouting(audio_source_none);
            usbSetLowPowerMode(theSink.usb->config.attach_timeout);
            if(theSink.usb->dead_battery)
                MessageSendLater(&theSink.task, EventSysUsbDeadBatteryTimeout, 0, D_MIN(45));

#if defined(ENABLE_PEER) && defined(ENABLE_USB_AUDIO)
            {
                /*If the USB has connected then notify this to the peer device */
                sinkAvrcpUpdatePeerWiredSourceConnected(USB_AUDIO);
            }
#endif

            break;
        }
        case MESSAGE_USB_DETACHED:
        {
            LOGD("MESSAGE_USB_DETACHED\n");
            theSink.usb->enumerated = FALSE;
            theSink.usb->suspended  = FALSE;
            theSink.usb->deconfigured = FALSE;
            usbUpdateChargeCurrent();
            usbSetAudioSuspendState(usb_audio_suspend_remote);
#ifdef ENABLE_PEER
            PEER_UPDATE_REQUIRED_RELAY_STATE("USB DETACHED");
#endif
            audioHandleRouting(audio_source_none);
            MessageCancelAll(&theSink.task, EventUsrUsbLowPowerMode);
            MessageCancelAll(&theSink.task, EventSysUsbDeadBatteryTimeout);
            break;
        }
        case MESSAGE_USB_ENUMERATED:
        {
            LOGD("MESSAGE_USB_ENUMERATED\n");
            if(!theSink.usb->enumerated)
            {
                theSink.usb->enumerated = TRUE;
                usbUpdateChargeCurrent();
                MessageCancelAll(&theSink.task, EventUsrUsbLowPowerMode);
                MessageCancelAll(&theSink.task, EventSysUsbDeadBatteryTimeout);
            }
            break;
        }
        case MESSAGE_USB_SUSPENDED:
        {
            MessageUsbSuspended* ind = (MessageUsbSuspended*)message;
            LOGD("MESSAGE_USB_SUSPENDED - %s\n", (ind->has_suspended ? "Suspend" : "Resume"));
            if(ind->has_suspended != theSink.usb->suspended)
            {
                theSink.usb->suspended = ind->has_suspended;
                usbUpdateChargeCurrent();
            }

            if (ind->has_suspended)
            {
                usbSetAudioSuspendState(usb_audio_suspend_remote);
#ifdef ENABLE_PEER
                PEER_UPDATE_REQUIRED_RELAY_STATE("USB SUSPENDED");
#endif
            }
            break;
        }
        case MESSAGE_USB_DECONFIGURED:
        {
            LOGD("MESSAGE_USB_DECONFIGURED\n");
            if(theSink.usb->enumerated)
            {
                theSink.usb->enumerated = FALSE;
                theSink.usb->deconfigured  = TRUE;
                usbUpdateChargeCurrent();
                usbSetLowPowerMode(theSink.usb->config.deconfigured_timeout);
            }
            break;
        }
        case MESSAGE_USB_ALT_INTERFACE:
        {
            u16 interface_id;
            MessageUsbAltInterface* ind = (MessageUsbAltInterface*)message;

            LOGD("MESSAGE_USB_ALT_INTERFACE %d %d\n", ind->interface, ind->altsetting);
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_MIC_INTERFACE_ID, &interface_id);
            if(interface_id == ind->interface)
            {
                theSink.usb->mic_active = (ind->altsetting ? TRUE : FALSE);
                LOGD("USB: Mic ID %d active %d\n", interface_id, theSink.usb->mic_active);
            }
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_SPEAKER_INTERFACE_ID, &interface_id);
            if(interface_id == ind->interface)
            {
                theSink.usb->spkr_active = (ind->altsetting ? TRUE : FALSE);
                LOGD("USB: Speaker ID %d active %d\n", interface_id, theSink.usb->spkr_active);

                if (theSink.usb->spkr_active)
                {
                    usbSetAudioSuspendState(usb_audio_suspend_none);
#ifdef ENABLE_PEER
                    PEER_UPDATE_REQUIRED_RELAY_STATE("USB SPEAKER ACTIVE");
#endif
                }
                else
                {
                    usbSetAudioSuspendState(usb_audio_suspend_remote);
#ifdef ENABLE_PEER
                    PEER_UPDATE_REQUIRED_RELAY_STATE("USB SPEAKER INACTIVE");
#endif
                }
            }
#ifdef ENABLE_USB_AUDIO
            /* check for changes in required audio routing */
            LOGD("USB: MESSAGE_USB_ALT_INTERFACE checkAudioRouting\n");
            MessageCancelFirst(&theSink.task, EventSysCheckAudioRouting);
            MessageSendLater(&theSink.task, EventSysCheckAudioRouting, 0, USB_AUDIO_DISCONNECT_DELAY);
#endif
            break;
        }
        case USB_DEVICE_CLASS_MSG_AUDIO_LEVELS_IND:
        {
            LOGD("USB_DEVICE_CLASS_MSG_AUDIO_LEVELS_IND\n");
            usbAudioSetVolume();
            break;
        }

        /* update of speaker sample rate from usb host */
        case USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND:
        {
            LOGD("USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND: %ld\n",(u32)((USB_DEVICE_CLASS_SAMPLE_RATE_T *)message)->sample_rate );
            usbAudioSetSpeakerSampleRate(((USB_DEVICE_CLASS_SAMPLE_RATE_T *)message)->sample_rate);
        }
        break;

        /* update of mic sample rate from usb host */
        case USB_DEVICE_CLASS_MSG_MIC_SAMPLE_RATE_IND:
        {
            LOGD("USB_DEVICE_CLASS_MSG_MICR_SAMPLE_RATE_IND: %ld\n",(u32)((USB_DEVICE_CLASS_SAMPLE_RATE_T *)message)->sample_rate );
            usbAudioSetMicSampleRate(((USB_DEVICE_CLASS_SAMPLE_RATE_T *)message)->sample_rate);
        }
        break;

        default:
        {
            LOGD("Unhandled USB message 0x%x\n", id);
            break;
        }
    }
}


/****************************************************************************
NAME
    usbFileInfo

DESCRIPTION
    Get file info (index and size) for a given file name.

RETURNS
    void
*/
static void usbFileInfo(const char* name, u8 size_name, usb_file_info* info)
{
    Source source;
    info->index = FileFind(FILE_ROOT, name, size_name);
    source = StreamFileSource(info->index);
    info->size = SourceSize(source);
    SourceClose(source);
}


/****************************************************************************
NAME
    usbFileName

DESCRIPTION
    Get file name from USB root

RETURNS
    void
*/
static void usbFileName(usb_file_info* root, usb_file_name_info* result)
{
    Source source = StreamFileSource(root->index);
    usb_file_name* file = (usb_file_name*)SourceMap(source);

    result->size = 0;

    if(file)
    {
        memmove(result->name, file->name, USB_NAME_SIZE);
        for(result->size = 0; result->size < USB_NAME_SIZE; result->size++)
            if(file->name[result->size] == ' ')
                break;
        *(result->name + result->size) = '.';
        result->size++;
        memmove(result->name + result->size, file->ext, USB_EXT_SIZE);
        result->size += USB_EXT_SIZE;
        SourceClose(source);
    }
#ifdef DEBUG_USB
    {
    u8 count;
    LOGD("USB: File Name ");
    for(count = 0; count < result->size; count++)
        LOGD("%c", result->name[count]);
    LOGD("\n");
    }
#endif
}


/****************************************************************************
NAME
    usbTimeCriticalInit

DESCRIPTION
    Initialise USB. This function is time critical and must be called from
    _init. This will fail if either Host Interface is not set to USB or
    VM control of USB is FALSE in PS. It may also fail if Transport in the
    project properties is not set to USB VM.

RETURNS
    void
*/
void usbTimeCriticalInit(void)
{
#ifdef ENABLE_USB_AUDIO
    const usb_plugin_info* plugin;
#endif
    usb_device_class_status status;
    usb_file_info root;
    usb_file_info file;
    usb_file_name_info file_name;

    LOGD("USB: Time Critical\n");

    /* maloc the memory for the USB configuration */
    theSink.usb = mallocPanic( sizeof(usb_info) );
    memset(theSink.usb, 0, sizeof (usb_info));
    LOGD("INIT: Malloc size usb: [%d]\n",sizeof(usb_info));

    /* Default to not configured or suspended */
    theSink.usb->ready = FALSE;
    theSink.usb->enumerated = FALSE;
    theSink.usb->suspended  = FALSE;
    theSink.usb->vbus_okay  = TRUE;
    theSink.usb->deconfigured = FALSE;
    theSink.usb->audio_suspend_state = usb_audio_suspend_none;

    /* Check if we booted with dead battery */
    usbSetVbatDead(usbDeadBatteryAtBoot());

    /* Get USB configuration */
    sinkUsbReadConfig();

    /* Abort if no device classes supported */
    if(!theSink.usb->config.device_class)
        return;

    usbFileInfo(root_name, sizeof(root_name)-1, &root);
    usbFileName(&root, &file_name);
    usbFileInfo(file_name.name, file_name.size, &file);

    /* If we can't find the help file don't enumerate mass storage */
    if(file.index == FILE_NONE || root.index == FILE_NONE)
        USB_CLASS_DISABLE(USB_DEVICE_CLASS_TYPE_MASS_STORAGE);

#ifdef ENABLE_USB_AUDIO
    plugin = &usb_plugins[theSink.usb->config.plugin_type];
    LOGD("USB: Audio Plugin %d\n", theSink.usb->config.plugin_index);

    status = UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_AUDIO_INTERFACE_DESCRIPTORS, 0, 0, (const u8*)(plugin->usb_descriptors));
    LOGD("USB: interface descriptors = %x\n",status);
    /* configure usb audio volume levels/steps */
    status = UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_AUDIO_VOLUMES, 0, 0, (const u8*)(&usb_stereo_audio_volume));
    LOGD("USB: volume descriptors = %x\n",status);
#else
    /* If audio not supported don't enumerate as mic or speaker */
    USB_CLASS_DISABLE(USB_DEVICE_CLASS_AUDIO);
#endif

    LOGD("USB: Endpoint Setup [0x%04X] - ", theSink.usb->config.device_class);
    /* Attempt to enumerate - abort if failed */
    status = UsbDeviceClassEnumerate(&theSink.task, theSink.usb->config.device_class);

    if(status != usb_device_class_status_success)
    {
        LOGD("Error %X\n", status);
        return;
    }

    LOGD("Success\n");
    /* Configure mass storage device */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_MASS_STORAGE))
    {
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_DATA_AREA, file.index, file.size, 0);
        usbFileInfo(fat_name, sizeof(fat_name)-1, &file);
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_TABLE, file.index, file.size, 0);
        UsbDeviceClassConfigure(USB_DEVICE_CLASS_CONFIG_MASS_STORAGE_FAT_ROOT_DIR, root.index, root.size, 0);
    }
}


/****************************************************************************
NAME
    usbInit

DESCRIPTION
    Initialisation done once the main loop is up and running. Determines USB
    attach status etc.

RETURNS
    void
*/
void usbInit(void)
{
    LOGD("USB: Init\n");

    /* Abort if no device classes supported */
    if(!theSink.usb->config.device_class)
        return;
    /* If battery charging enabled set the charge current */
    usbUpdateChargeCurrent();
#ifdef ENABLE_USB_AUDIO
    /* Pass NULL USB mic Sink until the plugin handles USB mic */
    theSink.a2dp_link_data->a2dp_audio_connect_params.usb_params = NULL;
#endif
    /* Schedule reset to low power mode if attached */
    usbSetLowPowerMode(theSink.usb->config.attach_timeout);
    /* Check for audio */
    theSink.usb->ready = TRUE;
    audioHandleRouting(audio_source_none);
    stateManagerAmpPowerControl(POWER_UP);
}


/****************************************************************************
NAME
    usbSetVbusLevel

DESCRIPTION
    Set whether VBUS is above or below threshold

RETURNS
    void
*/
void usbSetVbusLevel(voltage_reading vbus)
{
    LOGD("USB: VBUS %dmV [%d]\n", vbus.voltage, vbus.level);
    theSink.usb->vbus_okay = vbus.level;
}


/****************************************************************************
NAME
    usbSetDeadBattery

DESCRIPTION
    Set whether VBAT is below the dead battery threshold

RETURNS
    void
*/
void usbSetVbatDead(bool dead)
{
    LOGD("USB: VBAT %s\n", dead ? "Dead" : "Okay");
    theSink.usb->dead_battery = dead;
    if(!dead) MessageCancelAll(&theSink.task, EventSysUsbDeadBatteryTimeout);
}


/****************************************************************************
NAME
    usbGetChargeCurrent

DESCRIPTION
    Get USB charger limits

RETURNS
    void
*/
sink_charge_current* usbGetChargeCurrent(void)
{
    /* USB charging not enabled - no limits */
    if(!USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
        return NULL;

    LOGD("USB: Status ");

    /* Set charge current */
    switch(UsbAttachedStatus())
    {
        case HOST_OR_HUB:
            LOGD("Host/Hub ");
            if(theSink.usb->suspended)
            {
                LOGD("Suspended (Battery %s)\n", usbDeadBatteryProvision() ? "Dead" : "Okay");
                if(usbDeadBatteryProvision())
                    return &theSink.usb->config.i_susp_db;
                else
                    return &theSink.usb->config.i_susp;
            }
            else if(powerManagerIsChargerFullCurrent())
            {
                LOGD("%sEnumerated (Chg Full)\n", theSink.usb->enumerated ? "" : "Not ");
                if(!theSink.usb->enumerated)
                    return &theSink.usb->config.i_att;
                else
                    return &theSink.usb->config.i_conn;
            }
            else
            {
                LOGD("%sEnumerated (Chg Partial)\n", theSink.usb->enumerated ? "" : "Not ");
                if(!theSink.usb->enumerated)
                    return &theSink.usb->config.i_att_trickle;
                else
                    return &theSink.usb->config.i_conn_trickle;
            }
#ifdef HAVE_FULL_USB_CHARGER_DETECTION
        case DEDICATED_CHARGER:
            LOGD("Dedicated Charger Port%s\n", theSink.usb->vbus_okay ? "" : " Limited");
            if(theSink.usb->vbus_okay)
                return &theSink.usb->config.i_dchg;
            else
                return &theSink.usb->config.i_lim;

        case HOST_OR_HUB_CHARGER:
        case CHARGING_PORT:
            LOGD("Charger Port%s\n", theSink.usb->vbus_okay ? "" : " Limited");
            if(theSink.usb->vbus_okay)
                return &theSink.usb->config.i_chg;
            else
                return &theSink.usb->config.i_lim;
#endif
        case DETACHED:
        default:
            LOGD("Detached\n");
            if(powerManagerIsChargerConnected())
                return &theSink.usb->config.i_disc;
            else
                return NULL;
    }
}


/****************************************************************************
NAME
    usbIsAttached

DESCRIPTION
    Determine if USB is attached

RETURNS
    TRUE if USB is attached, FALSE otherwise
*/
bool usbIsAttached(void)
{
    /* If not detached return TRUE */
    return (UsbAttachedStatus() != DETACHED);
}


#ifdef ENABLE_USB_AUDIO
/****************************************************************************
NAME
    usbSendHidEvent

DESCRIPTION
    Send HID event over USB

RETURNS
    void
*/
static void usbSendHidEvent(usb_device_class_event event)
{
    LOGD("USB: HID Event 0x%X\n", event);
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL))
        UsbDeviceClassSendEvent(event);
}

/****************************************************************************
NAME
    usbAudioIsAttached

DESCRIPTION
    Determine if USB Audio is attached

RETURNS
    TRUE if USB Audio is attached, FALSE otherwise
*/
bool usbAudioIsAttached(void)
{
    USB_DEBUG(("usbAudioIsAttached: att=%u rdy=%u alw=%u mic=%u spk=%u sus=%u\n",
               usbIsAttached(),
               theSink.usb->ready,
               theSink.usb->config.audio_always_on,
               theSink.usb->mic_active,
               theSink.usb->spkr_active,
               theSink.usb->audio_suspend_state));

    /* If USB detached or not ready - no audio */
    if (!usbIsAttached() || !theSink.usb->ready)
    {
        return FALSE;
    }

    /* If USB attached and always on - audio */
    if (theSink.usb->config.audio_always_on)
    {
        return TRUE;
    }

    /* If mic and speaker both inactive - no audio */
    if (!theSink.usb->mic_active && !theSink.usb->spkr_active)
    {
        return FALSE;
    }

    /* USB speaker can be held active even though we have suspended */
    if (theSink.usb->spkr_active && theSink.usb->audio_suspend_state != usb_audio_suspend_none)
    {
        return FALSE;
    }

    /* Mic or speaker active - audio */
    return TRUE;
}


/****************************************************************************
NAME
    usbGetVolume

DESCRIPTION
    Extract USB volume setting from USB lib levels

RETURNS
    Volume to pass to csr_usb_audio_plugin
*/
static i16 usbGetVolume(AUDIO_MODE_T* mode, volume_direction* dir)
{
    i16 result;
    bool mic_muted;
    bool spk_muted = FALSE;

    /* Get vol settings from USB lib */
    usb_device_class_audio_levels levels;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_LEVELS, (u16*)&levels);

    LOGD("USB: RAW Gain L %X R %X\n", levels.out_l_vol, levels.out_r_vol);

    /* convert to signed 16 */
    levels.out_l_vol = (i16)(i8)(levels.out_l_vol>>8);
    levels.out_r_vol = (i16)(i8)(levels.out_r_vol>>8);

    LOGD("USB: Limited Gain L %X R %X\n", levels.out_l_vol, levels.out_r_vol);
    LOGD("USB: Mute M %X S %X\n", levels.in_mute, levels.out_mute);

    if(theSink.usb->config.plugin_type == usb_plugin_stereo)
    {
        /* Pack result */
        result = (sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps - ((levels.out_l_vol * sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps)/-60));
        /* limit check */
        if(result >= sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps)
            result = (sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps - 1);
        if(result < 0)
            result = 0;

        /* check for mute state */
        if(levels.out_mute)
        {
            /* set to mute */
            result = VOLUME_A2DP_MIN_LEVEL;
        }

        LOGD("USB: Stereo Gain dB %d\n", result);
        displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * result)/sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps);
#ifdef ENABLE_SUBWOOFER
        updateSwatVolume(result);
#endif
    }

    else
    {
        /* Use HFP gain mappings */
        /* convert from USB dB to hfp volume steps */
        result = (VOL_NUM_VOL_SETTINGS - ((levels.out_l_vol * VOL_NUM_VOL_SETTINGS)/-60));
        /* limit check */
        if(result >= VOL_NUM_VOL_SETTINGS)
            result = (VOL_NUM_VOL_SETTINGS - 1);
        if(result < 0)
            result = 0;

        displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * levels.out_l_vol)/sinkVolumeGetGroupConfig(multi_channel_group_main).no_of_steps);
#ifdef ENABLE_SUBWOOFER
        updateSwatVolume(levels.out_l_vol);
#endif
    }

    /* Mute if muted by host or not supported */
    mic_muted = levels.in_mute  || !USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_AUDIO_MICROPHONE);
    spk_muted = spk_muted || levels.out_mute || !USB_CLASS_ENABLED(USB_DEVICE_CLASS_TYPE_AUDIO_SPEAKER);

    if(mode)
    {
        if(mic_muted && spk_muted)
            *mode = AUDIO_MODE_STANDBY;
        else
            *mode = AUDIO_MODE_CONNECTED;
    }

    VolumeUpdateMuteStatusMicrophone(mic_muted);
    VolumeUpdateMuteStatusAllOutputs(spk_muted);
    /* Compare with previously stored volume level and set dir as appropriate.
       Only compare to out_l_vol as out_r_vol is ignored in setting result.
       */
    if (dir)
    {
        if(theSink.usb->out_l_vol < levels.out_l_vol)
            *dir = increase_volume;
        else if (theSink.usb->out_l_vol > levels.out_l_vol)
            *dir = decrease_volume;
        else
            *dir = same_volume;
    }

    /* Store usb audio levels for use later */
    theSink.usb->out_l_vol = levels.out_l_vol;
    theSink.usb->out_mute = levels.out_mute;

    return result;
}


/****************************************************************************
NAME
    usbAudioSinkMatch

DESCRIPTION
    Compare sink to the USB audio sink

RETURNS
    TRUE if sink matches USB audio sink, otherwise FALSE
*/
bool usbAudioSinkMatch(Sink sink)
{
    Source usb_source = NULL;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (u16*)&usb_source);

    LOGD("USB: usbAudioSinkMatch sink %x = %x, enabled = %x\n", (u16)sink , (u16)StreamSinkFromSource(usb_source), USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) );

    return (USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) && sink && (sink == StreamSinkFromSource(usb_source)));
}


/****************************************************************************
NAME
    usbAudioGetPluginInfo

DESCRIPTION
    Get USB plugin info for current config

RETURNS
    TRUE if successful, otherwise FALSE
*/
static const usb_plugin_info* usbAudioGetPluginInfo(Task* task, usb_plugin_type type, u8 index)
{
    switch(type)
    {
        case usb_plugin_stereo:
            *task = getA2dpPlugin(SBC_SEID);

            audioControlLowPowerCodecs (FALSE) ;
        break;
        case usb_plugin_mono_nb:
            *task = audioHfpGetPlugin(hfp_wbs_codec_mask_cvsd, index);

            audioControlLowPowerCodecs (TRUE) ;
        break;
        case usb_plugin_mono_wb:
            *task = audioHfpGetPlugin(hfp_wbs_codec_mask_msbc, index);

             audioControlLowPowerCodecs (TRUE) ;
        break;
        default:
            *task = NULL;
        break;
    }

    return &usb_plugins[type];
}

#if defined ENABLE_PEER && defined PEER_TWS
/****************************************************************************
NAME
 usbCheckDeviceTrimVol

DESCRIPTION
 check whether USB streaming is ongoing and if currently active and routing audio to the device, adjust the volume up or down
 as appropriate.

RETURNS
 bool   TRUE if volume adjusted, FALSE if no streams found

*/
bool usbCheckDeviceTrimVol(volume_direction dir, tws_device_type tws_device)
{
    if(!usbAudioSinkMatch(theSink.routed_audio))
    {
        return FALSE;
    }

    VolumeModifyAndUpdateTWSDeviceTrim(dir, tws_device);

    return TRUE;
}

#endif

/****************************************************************************
NAME
    usbAudioRoute

DESCRIPTION
    Connect USB audio stream

RETURNS
    void
*/
void usbAudioRoute(void)
{
    Sink sink;
    Source source;
    u16 sampleFreq;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (u16*)(&source));
    /* Note: UsbDeviceClassGetValue uses u16 which limits max value of sample frequency to 64k (uint 16 has range 0->65536) */
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_SPEAKER_SAMPLE_FREQ, &sampleFreq);
    sink = StreamSinkFromSource(source);

    LOGD("USB: Audio - sample rate = %u ",sampleFreq);
    /* Check Audio configured (sink will be NULL if VM USB not enabled) */
    if(USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) && sink)
    {
        LOGD("Configured ");
        if(usbAudioIsAttached())
        {
            LOGD("Attached\n");
            if(theSink.routed_audio != sink)
            {
                Task plugin;
                AUDIO_MODE_T mode;
                const usb_plugin_info* plugin_info = usbAudioGetPluginInfo(&plugin, theSink.usb->config.plugin_type, theSink.usb->config.plugin_index);

                /* get current volume level in steps from usb interface,
                 * main group only */
                theSink.volume_levels->usb_volume.main_volume = usbGetVolume(&mode, NULL);

                /* update currently routed source */
                theSink.routed_audio = sink;
                UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SINK, (u16*)(&theSink.hfp_plugin_params.usb_params.usb_sink));
                /* Make sure we're using correct parameters for USB */
                theSink.a2dp_link_data->a2dp_audio_connect_params.mode_params = &theSink.a2dp_link_data->a2dp_audio_mode_params;

                /* Allow USB volume changes to generate max and min events */
                MessageSend ( &theSink.task , EventSysAllowUSBVolEvents , 0 );

                LOGD("USB: Connect 0x%X 0x%X", (u16)sink, (u16)(theSink.hfp_plugin_params.usb_params.usb_sink));

#ifdef ENABLE_SUBWOOFER
                /* set the sub woofer link type prior to passing to audio connect */
                theSink.a2dp_link_data->a2dp_audio_connect_params.sub_woofer_type  = AUDIO_SUB_WOOFER_NONE;
                theSink.a2dp_link_data->a2dp_audio_connect_params.sub_sink  = NULL;
#endif

                /* use a2dp connect parameters */
                /* sample frequency is not fixed so read this from usb library */
                if(plugin_info->plugin_type == usb_plugin_stereo)
                {
#if defined ENABLE_PEER && defined PEER_TWS
                    AudioPluginFeatures PluginFeatures = theSink.conf2->audio_routing_data.PluginFeatures;
                    PluginFeatures.audio_input_routing = AUDIO_ROUTE_INTERNAL_AND_RELAY;

                    switch (sampleFreq)
                    {
                    case 16000:
                        theSink.a2dp_link_data->a2dp_audio_connect_params.bitpool = 53;
                        theSink.a2dp_link_data->a2dp_audio_connect_params.format = 0x3F;    /* 16Khz, 16 blocks, Joint Stereo, SNR, 8 subbands */
                        break;
                    case 32000:
                        theSink.a2dp_link_data->a2dp_audio_connect_params.bitpool = 53;
                        theSink.a2dp_link_data->a2dp_audio_connect_params.format = 0x7F;    /* 32Khz, 16 blocks, Joint Stereo, SNR, 8 subbands */
                        break;
                    case 44100:
                        theSink.a2dp_link_data->a2dp_audio_connect_params.bitpool = 53;
                        theSink.a2dp_link_data->a2dp_audio_connect_params.format = 0xBF;    /* 44.1Khz, 16 blocks, Joint Stereo, SNR, 8 subbands */
                        break;
                    case 48000:
                        theSink.a2dp_link_data->a2dp_audio_connect_params.bitpool = 51;
                        theSink.a2dp_link_data->a2dp_audio_connect_params.format = 0xFF;    /* 48Khz, 16 blocks, Joint Stereo, SNR, 8 subbands */
                        break;
                    default:
                        /* Unsupported rate */
                        break;
                    }

                    LOGD("USB: AUDIO_ROUTE_INTERNAL_AND_RELAY\n");
                    AudioConnect(plugin,
                                 sink,
                                 AUDIO_SINK_USB,
                                 theSink.codec_task,
                                 TonesGetToneVolumeInDb(multi_channel_group_main),
                                 sampleFreq,
                                 PluginFeatures,
                                 mode,
                                 AUDIO_ROUTE_INTERNAL,
                                 powerManagerGetLBIPM(),
                                 &theSink.a2dp_link_data->a2dp_audio_connect_params,
                                 &theSink.task);
#else

                    AudioConnect(plugin,
                                 sink,
                                 AUDIO_SINK_USB,
                                 theSink.codec_task,
                                 TonesGetToneVolumeInDb(multi_channel_group_main),
                                 sampleFreq,
                                 theSink.conf2->audio_routing_data.PluginFeatures,
                                 mode,
                                 AUDIO_ROUTE_INTERNAL,
                                 powerManagerGetLBIPM(),
                                 &theSink.a2dp_link_data->a2dp_audio_connect_params,
                                 &theSink.task);
#endif
                    /* Use group volume interface for stereo volume */
                    VolumeSetupInitialMutesAndVolumes(&theSink.volume_levels->usb_volume);
                }
                /* all other plugins use cvc connect parameters */
                else
                {
                    AudioConnect(plugin,
                                 sink,
                                 AUDIO_SINK_USB,
                                 theSink.codec_task,
                                 TonesGetToneVolumeInDb(multi_channel_group_main),
                                 sampleFreq,
                                 theSink.conf2->audio_routing_data.PluginFeatures,
                                 mode,
                                 AUDIO_ROUTE_INTERNAL,
                                 powerManagerGetLBIPM(),
                                 &theSink.hfp_plugin_params,
                                 &theSink.task);

                    /* Use volume interface for CVC volume */
                    AudioSetVolume(sinkVolumeGetCvcVol(theSink.volume_levels->usb_volume.main_volume),
                                   TonesGetToneVolume(FALSE),
                                   theSink.codec_task);
                }


#ifdef ENABLE_PEER
                /* Update the mute state. Mutes the audio output if the peer link is changing state */
                peerUpdateMuteState();
#endif

#ifdef ENABLE_SUBWOOFER
                /* set subwoofer volume level */
                updateSwatVolume(theSink.volume_levels->usb_volume.main_volume);
#endif
            }
        }
    }
    LOGD("\n");
}

/*************************************************************************
NAME
    usbAudioSuspend

DESCRIPTION
    Issue HID consumer control command to attempt to pause current USB stream

RETURNS
    None

**************************************************************************/
static void usbAudioSuspend (void)
{
    /* If speaker is in use or not marked as suspended then pause */
    if (theSink.usb->spkr_active || (theSink.usb->audio_suspend_state == usb_audio_suspend_none))
    {
        usbSetAudioSuspendState(usb_audio_suspend_local);
        UsbDeviceClassSendEvent(USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_PAUSE);

        /* Ensure audio routing is kicked as we may not always get a notification that streaming has ceased via USB */
        MessageSend(&theSink.task, EventSysCheckAudioRouting, 0);
    }
}

/*************************************************************************
NAME
    usbAudioResume

DESCRIPTION
    Issue HID consumer control command to attempt to resume current USB stream

RETURNS
    None

**************************************************************************/
void usbAudioResume (void)
{
    /* If speaker is not use or marked as suspended then resume */
    if (!theSink.usb->spkr_active || (theSink.usb->audio_suspend_state != usb_audio_suspend_none))
    {
        usbSetAudioSuspendState(usb_audio_suspend_none);
        UsbDeviceClassSendEvent(USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_PLAY);

        /* Ensure audio routing is kicked as we may not always get a notification that streaming has started via USB */
        MessageSend(&theSink.task, EventSysCheckAudioRouting, 0);
    }
}

/****************************************************************************
NAME
    usbAudioDisconnect

DESCRIPTION
    Disconnect USB audio stream

RETURNS
    void
*/
void usbAudioDisconnect(void)
{
    if(usbAudioSinkMatch(theSink.routed_audio))
    {
        LOGD("USB: Disconnect 0x%X\n", (u16)theSink.routed_audio);
        AudioDisconnect();
        theSink.routed_audio = 0;

        /* If speaker is in use then pause */
        if (theSink.usb->config.pause_when_switching_source)
        {
        	/* This option was added to address B-164414
         	 * If the device was paused already this command is redundant, but it will however force the
        	 * vm to send a resume command when switching back to USB. This will restart a USB audio stream
        	 * that was previously paused.
        	 * Also, when using USB audio not all sources will accept the pause command e.g streaming services.
        	 */
        	usbAudioSuspend();
        }

#ifdef ENABLE_PEER
        {
            /*If the USB has disconnected then notify this to the peer device */
            sinkAvrcpUpdatePeerSourceDisconnected(USB_AUDIO);
        }
#endif
    }
}


/****************************************************************************
NAME
    usbAudioSetVolume

DESCRIPTION
    Set USB audio volume

RETURNS
    void
*/
static void usbAudioSetVolume(void)
{
    if(usbAudioSinkMatch(theSink.routed_audio) && USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO))
    {
        AUDIO_MODE_T mode;
        volume_direction direction;

        /* Store old volume to check if we need to change mode */
        i16 oldVolume = theSink.volume_levels->usb_volume.main_volume;

        /* get usb volume in steps */
        theSink.volume_levels->usb_volume.main_volume = usbGetVolume(&mode, &direction);

        if(theSink.usb->vol_event_allowed)
        {
            if((direction == decrease_volume) && sinkUsbVolumeIsAtMinimum())
            {
                sinkUsbVolumeEvent(EventSysVolumeMin);
            }
            else if((direction == increase_volume) && sinkUsbVolumeIsAtMaximum())
            {
                sinkUsbVolumeEvent(EventSysVolumeMax);
            }
        }

        if(theSink.usb->config.plugin_type == usb_plugin_stereo)
        {
            /* Use group volume interface for stereo volume */
            VolumeSetNewMainVolume(&theSink.volume_levels->usb_volume, oldVolume);
        }
        else
        {
            /* Use volume interface for CVC volume */
            AudioSetVolume(sinkVolumeGetCvcVol(theSink.volume_levels->usb_volume.main_volume) ,
                           TonesGetToneVolume(FALSE),
                           theSink.codec_task);
        }
    }
}


/****************************************************************************
NAME
    usbAudioGetMode

DESCRIPTION
    Get the current USB audio mode if USB in use

RETURNS
    void
*/
void usbAudioGetMode(AUDIO_MODE_T* mode)
{
    if(usbAudioSinkMatch(theSink.routed_audio) && (theSink.usb->config.plugin_type == usb_plugin_stereo))
    {
        (void)usbGetVolume(mode, NULL);
    }
}

/****************************************************************************
NAME
    usbGetAudioSink

DESCRIPTION
    check USB state and return sink if available

RETURNS
   sink if available, otherwise 0
*/
Sink usbGetAudioSink(void)
{
    Source usb_source = NULL;
    Sink sink = NULL;
    UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE, (u16*)&usb_source);

    /* if the usb lead is attached and the speaker is active, try to obtain the audio sink */
    if((theSink.usb->config.audio_always_on)||((usbAudioIsAttached())&&(theSink.usb->spkr_active)))
    {
        /* attempt to obtain USB audio sink */
        sink = StreamSinkFromSource(usb_source);
        LOGD("USB: usbGetAudioSink sink %x, enabled = %x\n", (u16)sink , USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO) );
    }
    /* USB not attached */
    else
        LOGD("USB: usbGetAudioSink sink %x, enabled = %x, speaker active = %x\n", (u16)sink , USB_CLASS_ENABLED(USB_DEVICE_CLASS_AUDIO), theSink.usb->spkr_active );

    return sink;
}

/****************************************************************************
NAME
    usbAudioSetSpeakerSampleRate

DESCRIPTION
    check if the sample rate has change, whether usb audio is currently streaming
    , if so a restart of the sbc_decoder is necessary to run at the new indicated rate

RETURNS
    none
*/
static void usbAudioSetSpeakerSampleRate(u16 sample_rate)
{
    /* determine if the USB audio is currently being streamed by the DSP and the dsp
       is in the running state */
    if((theSink.rundata->routed_audio_source == audio_source_USB)&&
       (GetAudioPlugin() == (TaskData *)&csr_sbc_decoder_plugin)&&
       (GetCurrentDspStatus() == DSP_RUNNING))
    {
        u32 current_sampling_rate = AudioGetA2DPSampleRate();

        LOGD("USB: Set Spk Sample Rate - DSP running - now: %ld new: %ld\n", current_sampling_rate, (u32)sample_rate);

        /* dsp is loaded and running the usb decoder, determine if the new sample
           rate is different to that currently being used by the decoder */
        if(current_sampling_rate)
        {
            if(sample_rate != current_sampling_rate)
            {
                LOGD("USB: Set Spk Sample Rate - DSP running - *** needs update *** - AudioDisconnect\n");

                /* the sample rate is different to that currently being used, it is necessary
                   to restart the decoder to use the correct audio sample rate */
                AudioDisconnect();

                LOGD("USB: Set Spk Sample Rate - DSP running - *** needs update *** - Switch To USB\n");
                /* audio no longer routed */
                theSink.rundata->routed_audio_source = audio_source_none;
                theSink.routed_audio = 0;
                MessageCancelAll (&theSink.task, EventSysCheckAudioRouting);
                /* re-route USB audio */
                MessageSend( &theSink.task , EventSysCheckAudioRouting , 0);
            }
        }
        else
        {
            /*Current sampling rate is set to 0, which means that the audio decoder plugin has disconnected */
            /* reschedule the message to occur after the dsp has completed loading */
            MAKE_USB_DEVICE_CLASS_MESSAGE(USB_DEVICE_CLASS_SAMPLE_RATE);
            message->sample_rate = sample_rate;
            MessageCancelAll (&theSink.task, USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND);
            MessageSendLater(&theSink.task, USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND, message, 100);
            LOGD("USB: Set Spk Sample Rate - DSP Loading - Reschedule\n");
        }
    }
    /* check if dsp is in the process of being loaded but not currently running
       in which case it needs to be restarted with the new sample rate */
    else if((theSink.rundata->routed_audio_source == audio_source_USB)&&
            (GetAudioPlugin() == (TaskData *)&csr_sbc_decoder_plugin)&&
            ((GetCurrentDspStatus() == DSP_LOADING)||(GetCurrentDspStatus() == DSP_LOADED_IDLE)))
    {
        /* reschedule the message to occur after the dsp has completed loading */
        MAKE_USB_DEVICE_CLASS_MESSAGE(USB_DEVICE_CLASS_SAMPLE_RATE);
        message->sample_rate = sample_rate;
        MessageCancelAll (&theSink.task, USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND);
        MessageSendLater(&theSink.task, USB_DEVICE_CLASS_MSG_SPEAKER_SAMPLE_RATE_IND, message, 100);
        LOGD("USB: Set Spk Sample Rate - DSP Loading - Reschedule\n");
    }
    else
    {
        LOGD("USB: Set Spk Sample Rate - DSP NOT running\n");
    }

}

/****************************************************************************
NAME
    usbAudioSetMicSampleRate

DESCRIPTION
    check if the sample rate has change, whether usb audio is currently streaming
    , if so a restart of the sbc_decoder may be necessary to run at the new indicated rate

RETURNS
    none
*/
static void usbAudioSetMicSampleRate(u16 sample_rate)
{

}

bool usbIfCurrentSinkIsUsb(void)
{
    if(usbAudioSinkMatch(theSink.routed_audio))
{
        return TRUE;
        }
    return FALSE;
    }

bool usbIfUsbSinkExists(void)
    {
    if(usbGetAudioSink())
        {
        return TRUE;
        }
    return FALSE;
}

#endif /* ENABLE_USB_AUDIO */


static void sinkUsbReadConfig(void)
{
    ConfigRetrieve(CONFIG_USB_CONFIG, &theSink.usb->config, sizeof(usb_config));
}


static void sinkUsbVolumeEvent(sinkEvents_t event)
{
    MessageSend ( &theSink.task, event, 0 );
    theSink.usb->vol_event_allowed = FALSE;
    MessageSendLater ( &theSink.task, EventSysAllowUSBVolEvents, 0, VOLUME_USB_EVENT_WAIT );
}

void sinkUsbVolumeIncrement(void)
{
    if(theSink.usb->vol_event_allowed && sinkUsbVolumeIsAtMaximum())
    {
        /* Send EventSysVolumeMax if volume was increased, we are above the usb max volume and not muted */
        sinkUsbVolumeEvent(EventSysVolumeMax);
    }
    usbVolUp();
}

void sinkUsbVolumeDecrement(void)
{
    if(theSink.usb->vol_event_allowed && sinkUsbVolumeIsAtMinimum())
    {
        /* Send EventSysVolumeMin if volume was decreased and we are at the lowest volume level */
        sinkUsbVolumeEvent(EventSysVolumeMin);
    }
    usbVolDn();
}

bool sinkUsbProcessEventUsb(const MessageId EventUsb)
{
    switch(EventUsb)
    {
        case EventUsrUsbPlayPause:
           LOGD("HS : EventUsrUsbPlayPause");
           usbPlayPause();
           break;
        case EventUsrUsbStop:
           LOGD("HS : EventUsrUsbStop\n");
           usbStop();
           break;
        case EventUsrUsbFwd:
           LOGD("HS : EventUsrUsbFwd\n");
           usbFwd();
           break;
        case EventUsrUsbBack:
           LOGD("HS : EventUsrUsbBack\n");
           usbBck();
           break;
        case EventUsrUsbMute:
           LOGD("HS : EventUsrUsbMute");
           usbMute();
           break;
        case EventUsrUsbLowPowerMode:
            /* USB low power mode */
            usbSetBootMode(BOOTMODE_USB_LOW_POWER);
            break;
        case EventSysUsbDeadBatteryTimeout:
            usbSetVbatDead(FALSE);
            break;
        case EventSysAllowUSBVolEvents:
            theSink.usb->vol_event_allowed = TRUE;
            break;
    }
    return TRUE;
}


#endif /* ENABLE_USB */
