/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_usb_descriptors.h
    
DESCRIPTION
    
*/
#ifndef _SINK_USB_DESCRIPTORS_H_
#define _SINK_USB_DESCRIPTORS_H_

#define SAMPLE_RATE_CVC    ((u32) 8000)
#define SAMPLE_RATE_CVC_WB ((u32) 16000)
#define SAMPLE_RATE_STEREO ((u32) 48000)

/* Unit/Terminal IDs */
#define SPEAKER_IT  0x01
#define SPEAKER_FU  0x02
#define SPEAKER_OT  0x03
#define MIC_IT      0x04
#define MIC_FU      0x05
#define MIC_OT      0x06

/* audio volume configuration */
#define SPEAKER_VOLUME_MAX              0x0000 /* +00.00 */
#define SPEAKER_VOLUME_MIN              0xC400 /* -60.00 */
#define SPEAKER_VOLUME_RESOLUTION       0x0100 /* +01.00 */
#define SPEAKER_VOLUME_DEFAULT          0xEC00 /* -19.00 */

#define MICROPHONE_VOLUME_MAX           0x0000 /* +00.00 */
#define MICROPHONE_VOLUME_MIN           0xD200 /* -45.00 */
#define MICROPHONE_VOLUME_RESOLUTION    0x0300 /* +03.00 */
#define MICROPHONE_VOLUME_DEFAULT       0x0000 /* +00.00 */


#endif /* _SINK_USB_DESCRIPTORS_H_ */
