/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_api.h
    
DESCRIPTION

*/

#ifndef A2DP_API_H_
#define A2DP_API_H_

void a2dpSignallingConnectInd (remote_device *device);
void a2dpSignallingConnectCfm (remote_device *device, a2dp_status_code status);
void a2dpSignallingDisconnectInd (remote_device *device, a2dp_status_code status);
void a2dpSignallingLinklossInd (remote_device *device);

void a2dpMediaOpenInd (remote_device *device, u8 seid);
void a2dpMediaOpenCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaCloseInd (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaStartInd (remote_device *device, media_channel *media);
void a2dpMediaStartCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaSuspendInd (remote_device *device, media_channel *media);
void a2dpMediaSuspendCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpCodecConfigureInd (remote_device *device, u8 local_seid, u16 size_remote_service_caps, u8* remote_service_caps);
void a2dpMediaReconfigureInd (remote_device *device, media_channel *media);
void a2dpMediaReconfigureCfm (remote_device *device, media_channel *media, a2dp_status_code status);
void a2dpMediaAvSyncDelayInd (remote_device *device, u8 seid);
void a2dpMediaAvSyncDelayCfm (remote_device *device, u8 seid, a2dp_status_code status);
void a2dpMediaAvSyncDelayUpdatedInd (remote_device *device, u8 seid, u16 delay);
void a2dpLinklossReconnectCancelledInd (remote_device *device);

#endif
