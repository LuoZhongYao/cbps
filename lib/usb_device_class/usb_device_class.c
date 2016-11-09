/*******************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/


#include <panic.h>
#include <sink.h>
#include <stdlib.h>
#include <usb.h>

#include "usb_device_class.h"
#include "usb_device_audio.h"
#include "usb_device_battery_charging.h"
#include "usb_device_mass_storage.h"
#include "usb_device_hid.h"
#include "usb_device_class_private.h"

#include <print.h>


device_class_state *device;

/* helper function for allocating space in Sink */
u8 *claimSink(Sink sink, u16 size)
{
    u8 *dest = SinkMap(sink);
    u16 claim_result = SinkClaim(sink, size);
    if (claim_result == 0xffff)
    {
        return 0;
    }

    return (dest + claim_result);
}


usb_device_class_status UsbDeviceClassConfigure(usb_device_class_config config, u16 value_16, u32 value_32, const u8 *params)
{
#ifndef USB_DEVICE_CLASS_REMOVE_MASS_STORAGE    
    if (device && (device->usb_classes & USB_DEVICE_CLASS_TYPE_MASS_STORAGE))
    {
        /* Return success if this is a mass storage config request */
        if(usbConfigureMassStorage(config, value_16, value_32, (u8 *)params))
            return usb_device_class_status_success;
    }
#endif   
#ifndef USB_DEVICE_CLASS_REMOVE_AUDIO
    if (config == USB_DEVICE_CLASS_CONFIG_AUDIO_INTERFACE_DESCRIPTORS)
    {
        /* Return success if this is an audio descriptors config request */
        if(usbConfigureAudioDescriptors(config, (const usb_device_class_audio_config*)params))
            return usb_device_class_status_success;
    }
    else if (config == USB_DEVICE_CLASS_CONFIG_AUDIO_VOLUMES)
    {        
        /* Return success if this is an audio volume config request */
        if(usbConfigureAudioVolume(config, (const usb_device_class_audio_volume_config*)params))           
            return usb_device_class_status_success;
    }
#endif
#ifndef USB_DEVICE_CLASS_REMOVE_HID
    /* Return success if this is an HID config request */
    if (usbConfigureHidConsumerTransport(config, (const usb_device_class_hid_consumer_transport_config*)params))
        return usb_device_class_status_success;
#endif            
    return usb_device_class_status_invalid_param_value;
}


usb_device_class_status UsbDeviceClassEnumerate(Task app_task, u16 usb_device_class)
{
    usb_device_class_status status = usb_device_class_status_invalid_param_value;
    
    device = PanicUnlessNew(device_class_state);
    PRINT(("USB: Allocate memory [0x%x]\n", sizeof(device_class_state)));
    
#ifndef USB_DEVICE_CLASS_REMOVE_AUDIO    
    if ((status != usb_device_class_status_failed_enumeration) &&
        ((usb_device_class & USB_DEVICE_CLASS_TYPE_AUDIO_MICROPHONE) || (usb_device_class & USB_DEVICE_CLASS_TYPE_AUDIO_SPEAKER)))
    {
        status = usbEnumerateAudio(usb_device_class) == TRUE ? usb_device_class_status_success : usb_device_class_status_failed_enumeration;
    }
#endif   
    
#ifndef USB_DEVICE_CLASS_REMOVE_MASS_STORAGE    
    if ((status != usb_device_class_status_failed_enumeration) &&
        (usb_device_class & USB_DEVICE_CLASS_TYPE_MASS_STORAGE))
    {
        status = usbEnumerateMassStorage() == TRUE ? usb_device_class_status_success : usb_device_class_status_failed_enumeration;
    }
#endif    
 
#ifndef USB_DEVICE_CLASS_REMOVE_CHARGING    
    if ((status != usb_device_class_status_failed_enumeration) &&
        (usb_device_class & USB_DEVICE_CLASS_TYPE_BATTERY_CHARGING))
    {
        status = usbEnumerateBatteryCharging(app_task) == TRUE ? usb_device_class_status_success : usb_device_class_status_failed_enumeration;
    }
#endif
    
#ifndef USB_DEVICE_CLASS_REMOVE_HID    
    if ((status != usb_device_class_status_failed_enumeration) &&
        ((usb_device_class & USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL) || (usb_device_class & USB_DEVICE_CLASS_TYPE_HID_KEYBOARD)))
    {
        status = usbEnumerateHid(usb_device_class) == TRUE ? usb_device_class_status_success : usb_device_class_status_failed_enumeration;
    }
#endif    
    
    if (status == usb_device_class_status_success)
    {
        device->app_task = app_task;
        device->usb_classes = usb_device_class;
        MessageSystemTask(app_task);
    }
    else
    {
        free(device);
    }
   
    return status;    
}


usb_device_class_status UsbDeviceClassGetValue(usb_device_class_get_value id, void *value)
{
    switch (id)
    {
#ifndef USB_DEVICE_CLASS_REMOVE_AUDIO 
        case USB_DEVICE_CLASS_GET_VALUE_AUDIO_SOURCE:
        {
            *((Source*)value) = usbAudioSpeakerSource();
            return usb_device_class_status_success;
        }
        case USB_DEVICE_CLASS_GET_VALUE_AUDIO_SINK:
        {
            *((Sink*)value) = usbAudioMicSink();
            return usb_device_class_status_success;
        }
        case USB_DEVICE_CLASS_GET_VALUE_AUDIO_LEVELS:
        {
            usbAudioGetLevels(value);
            return usb_device_class_status_success;
            
        }
        case USB_DEVICE_CLASS_GET_MIC_INTERFACE_ID:
        case USB_DEVICE_CLASS_GET_SPEAKER_INTERFACE_ID:
        {
            *(UsbInterface*)value = usbAudioGetInterfaceId(id);
            return usb_device_class_status_success;
        }
        case USB_DEVICE_CLASS_GET_VALUE_SPEAKER_SAMPLE_FREQ:
        {
            *(u32*)value = usbAudioGetSpeakerSampleFreq();
            return usb_device_class_status_success;
        }
        case USB_DEVICE_CLASS_GET_VALUE_MIC_SAMPLE_FREQ:
        {
            *(u32*)value = usbAudioGetMicSampleFreq();
            return usb_device_class_status_success;
        }
#endif
#ifndef USB_DEVICE_CLASS_REMOVE_MASS_STORAGE
        case USB_DEVICE_CLASS_GET_VALUE_MASS_STORAGE_SOURCE:
        {
            *(Source*)value = usbMassStorageSource();
            return usb_device_class_status_success;
        }
        case USB_DEVICE_CLASS_GET_VALUE_MASS_STORAGE_SINK:
        {
            *(Sink*)value = usbMassStorageSink();
            return usb_device_class_status_success;
        }
#endif
        default:
        {
            break;
        }
    }
    
    return usb_device_class_status_invalid_param_value;
}


usb_device_class_status UsbDeviceClassSendEvent(usb_device_class_event event)
{
    
#ifndef USB_DEVICE_CLASS_REMOVE_HID    
    if ((event <= USB_DEVICE_CLASS_EVENT_HID_CONSUMER_TRANSPORT_MUTE) &&
        (device->usb_classes & USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL))
    {
        return usbSendDefaultHidConsumerEvent(event);              
    }
    if ((event <= USB_DEVICE_CLASS_EVENT_HID_KEYBOARD_ALT_END) &&
        (device->usb_classes & USB_DEVICE_CLASS_TYPE_HID_KEYBOARD))
    {
        return usbSendDefaultHidKeyboardEvent(event);              
    }
#endif    
    
    return usb_device_class_status_invalid_param_value;
}


usb_device_class_status UsbDeviceClassSendReport(usb_device_class_type class_type, u16 report_id, u16 size_report, u8 *report)
{
#ifndef USB_DEVICE_CLASS_REMOVE_HID
    if ((class_type == USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL) &&
        (device->usb_classes & USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL))
    {
        if (usbSendReportHidConsumerTransport(report_id, size_report, report))
            return usb_device_class_status_success;
    }
#endif
    return usb_device_class_status_invalid_param_value;
}


