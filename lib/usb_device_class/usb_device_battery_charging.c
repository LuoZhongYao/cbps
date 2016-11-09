/*******************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/

#ifndef USB_DEVICE_CLASS_REMOVE_CHARGING

#include <message.h>
#include <stream.h>
#include <usb.h>

#include "usb_device_class.h"
#include <print.h>

#include "usb_device_battery_charging.h"


bool usbEnumerateBatteryCharging(Task app_task)
{
    PRINT(("USB: setup batt charge 0x%x\n",(u16)app_task));
    StreamConfigure((vm_stream_config_key)VM_STREAM_USB_ATTACH_MSG_ENABLED, 1);
    return TRUE;
}

     
#else /* !USB_DEVICE_CLASS_REMOVE_CHARGING */
    static const int usb_device_class_charging_unused;
#endif /* USB_DEVICE_CLASS_REMOVE_CHARGING */
    
