/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
*/

/*!
@file    sink_device_id.h
@brief   Interface to the sink Device Id profile functions. 
*/

#ifndef _SINK_DEVICE_ID_H_
#define _SINK_DEVICE_ID_H_

#define VENDOR_ID_SOURCE_BT_SIG    0x0001   /* Indicates the Vendor Id has been assigned by the Bluetooth Special Interest Group */
#define VENDOR_ID_SOURCE_USB_IF    0x0002   /* Indicates the Vendor Id has been assigned by the USB Implementers Forum           */
#define VENDOR_ID_SOURCE_UNDEFINED 0xFFFF


/* If DEVICE_ID_CONST is defined, the registered Device Id service record will use the following defines */
#define DEVICE_ID_VENDOR_ID_SOURCE VENDOR_ID_SOURCE_UNDEFINED
#define DEVICE_ID_VENDOR_ID        0xFFFF   /* As assigned by the appropriate source */
#define DEVICE_ID_PRODUCT_ID       0xFFFF   /* Customer specific                     */
#define DEVICE_ID_BCD_VERSION      0xFFFF   /* Where 0xABCD = vAB.C.D                */

bool RequestRemoteDeviceId (Task task, const bdaddr *bd_addr);
remote_device CheckRemoteDeviceId (const u8 *attr_data, u16 attr_data_size);
void RegisterDeviceIdServiceRecord( void );
u16 GetDeviceIdEirDataSize( void );
u16 WriteDeviceIdEirData( u8 *p );
bool CheckEirDeviceIdData (u16 size_eir_data, const u8 *eir_data);
void ValidatePeerUseDeviceIdFeature(void);
void GetDeviceIdFullVersion(u16 *buffer);


#endif
