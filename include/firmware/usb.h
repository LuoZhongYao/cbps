/*
 * Copyright 2015 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.0
 */

#ifndef __USB_H

#define __USB_H

#include <csrtypes.h>
#include <app/usb/usb_if.h>
/*! @file usb.h @brief Control of USB EndPoints, and other USB related traps.
**
**
Bluelab applications have access to the on-chip USB interface.
USB devices communications occur through channels known as endpoints.
The USB specification defines a number of USB device classes through
Class Specifications. Each of the Class Specifications define the type
and direction of endpoints required to comply with that particular device
class. Examples include the Human Interface Device (HID) class, and the
Communication Device Class (CDC).
**
**
In addition to the documention provided here, the USB2.0 specification and
the "Bluetooth and USB Design Considerations" document available from
www.csrsupport.com are useful sources of information.
*/


/*!
    @brief Determine if USB connection is attached or detached 
    
    @return A value to indicate whether the USB connection is currently
    attached or detached. If USB charger detection is enabled then,
    if attached, the value returned will give information as to the
    type of device you are attached to (for example, a standard host/hub
    or a dedicated charger).

    Consult the USB battery charging specification available at www.usb.org
    for more details on the difference between a dedicated charger and a standard host.

    Note that BlueCore will only report a correct AttachedStatus if PSKEY_USB_PIO_VBUS
    is set correctly and either a PIO or the internal battery charger (if available) is
    connected to the USB VBUS pin.
*/
usb_attached_status UsbAttachedStatus(void);

/*!
   @brief Adds a USB interface. See the USB2.0 spec section 9.6.5
   @param codes Defines the USB class, sub-class and protocol.
   @param type Defines the class specific descriptor type.
   @param if_descriptor Points to the class specific descriptor.
   @param descriptor_length The length in bytes of if_descriptor.

   @return An interface number on success, else returns usb_interface_error.
   Possible reasons for usb_interface_error include insufficient pmalloc space
   for new firmware data structures, BlueCore is already enumerated on the bus,
   PSKEY_HOST_INTERFACE is not set to USB or PSKEY_USB_VM_CONTROL is FALSE,
   invalid combination of "descriptor_length" and "type" field.

   This API is used to register both the USB interface descriptor details, and any class specific
   descriptors to be returned to the host. 

   If there are no class specific descriptors then the type, if_descriptor, and
   descriptor_length parameters must be set to 0, NULL, 0 respectively.
   Otherwise, usb_interface_error is returned. 
*/
UsbInterface UsbAddInterface(const UsbCodes *codes, u16 type, const u8 *if_descriptor, u16 descriptor_length);

/*!
   @brief Adds USB endpoints to the interface passed. See the USB2.0 spec section 9.6.6
   @param interface The interface to add endpoints to (returned by a call to UsbAddInterface()).
   @param num_end_points The number of endpoints to be added.
   @param end_point_info Points to an array of endpoint definitions.

   @return TRUE if the endpoints were successfully added, else FALSE.
   Possible reasons for a FALSE return value include an invalid "interface" parameter,
   insufficient pmalloc space for new firmware data structures, 
   BlueCore is already enumerated on the bus, PSKEY_HOST_INTERFACE is not set 
   to USB or PSKEY_USB_VM_CONTROL is FALSE.
*/
bool UsbAddEndPoints(UsbInterface interface, u16 num_end_points, const EndPointInfo *end_point_info);

/*!
   @brief Adds a USB descriptor to an interface or endpoint.
   @param interface The interface to add the descriptor to.
   @param type Descriptor type (lower 8 bits).
   @param descriptor Pointer to the descriptor to add.
   @param descriptor_length The length of the descriptor in bytes.

   @return TRUE if the descriptor was successfully added, else FALSE.
   
   Descriptor type for a class specific endpoint descriptor will contain the address of the endpoint in the upper 8 bits.
   The upper 8 bits should be 0 otherwise.
   
   This API is normally used to add HID report or class specific endpoint descriptors.
*/
bool UsbAddDescriptor(UsbInterface interface, u16 type, const u8 *descriptor, u16 descriptor_length);

/*!
  @brief Adds a USB String Descriptor. See the USB2.0 spec section 9.6.7
  @param string_index The USB String Descriptor number. Only index in the range 5 to 255 can be used as 1 to 4 are reserved.
  @param string_descriptor A pointer to the string descriptor.

  @return TRUE if the descriptor was added, else FALSE.
  A FALSE value will be returned if an invalid string_index parameter is given.

  The string_descriptor passed is in UTF16 format and MUST be NUL terminated.
  The NUL terminator will not be passed to USB host as part of the string
  descriptor when a string descriptor request is received, it is simply
  to denote the end of the string.

  Example:
  In order to register string descriptor 11 as "Test".

  const u16 myStringDescriptor[] = {((u16)'T'), ((u16)'e'), ((u16)'s'), ((u16)'t'), 
                                       ((u16)'\\0')};
  UsbAddStringDescriptor(11, myStringDescriptor);
*/
bool UsbAddStringDescriptor(u8 string_index, const u16 *string_descriptor);

/*!
    @brief Add an Interface Association Descriptor to a USB interface
    @param if_num The interface number returned by a call to UsbAddInterface()
    @param ia_descriptor A pointer to the Interface Association descriptor in VM address space
    @param descriptor_length The length of the Interface Association descriptor pointed to by ia_descriptor.
    @return TRUE if the Interface Association descriptor was added correctly, else FALSE.
    A FALSE value will be returned if an invalid if_num is given.
*/
bool UsbAddInterfaceAssociationDescriptor(u16 if_num, const u8 *ia_descriptor, u16 descriptor_length);

/*!
  @brief Adds a DFU interface to the USB port.
*/
void UsbAddDfuInterface(void);

/*!
    @brief Creates an additonal USB configuration.
    @param desc_info Points to info for the new configuration descriptor.

    @return On success, returns a value corresponding to the
    bConfigurationValue of the new configuration. This will start at 2 for
    the first extra configuration created.
    On error, returns zero.
    Possible reasons for an error include calling the function after
    interfaces have been added, insufficient pmalloc space for new
    firmware data structures, BlueCore is already enumerated on the bus,
    PSKEY_HOST_INTERFACE is not set to USB or PSKEY_USB_VM_CONTROL is FALSE.

    All devices initially have one configuration (with iConfiguration,
    bmAttributes and bMaxPower in the configuration descriptor being provided
    by pskeys), and for most devices one configuration is all you want.
    However, if desired, this function can be used to create additional
    configurations. It will add a new configuration each time it is called,
    it can be called repeatedly, but must be called before any interfaces
    have been added.
    The support for multiple configurations is limited in that the
    interfaces/endpoints have to be the same for all configurations, the
    only values that can be different are the iConfiguration, bmAttributes and
    bMaxPower fields in the configuration descriptor. For the new
    configuration, these values are provided by the members of 'desc_info'.
    Each member of 'desc_info' can be set to the special value
    SAME_AS_FIRST_CONFIG in which case the corresponding entry in the
    configuration descriptor for the new configuration will just be copied
    over from the first configuration.
*/
u16 UsbAddConfiguration(const ConfigDescriptorInfo *desc_info);

/*!
    @brief Determine the state of the USB device
    
    @return The current state of the USB device from #usb_device_state
*/
usb_device_state UsbDeviceState(void);

#endif
