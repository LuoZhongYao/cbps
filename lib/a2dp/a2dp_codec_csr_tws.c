/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
	a2dp_csr_tws.c        

DESCRIPTION
	This file contains 

NOTES

*/

#ifndef A2DP_SBC_ONLY

/****************************************************************************
	Header files
*/
#include "a2dp.h"
#include "a2dp_private.h"
#include "a2dp_codec_sbc.h"
#include "a2dp_codec_mp3.h"
#include "a2dp_codec_aac.h"
#include "a2dp_codec_csr_aptx.h"
#include "a2dp_codec_csr_tws.h"
#include "a2dp_caps_parse.h"


/*****************************************************************************/
void selectOptimalCsrTwsSbcCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_SBC) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_SBC))
    {
        selectOptimalSbcCapsSink(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsSbcCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_SBC) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_SBC))
    {
        selectOptimalSbcCapsSource(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsMp3CapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO))
    {
        selectOptimalMp3CapsSink(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsMp3CapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO))
    {
        selectOptimalMp3CapsSource(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsAacCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_MPEG2_4_AAC) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_MPEG2_4_AAC))
    {
        selectOptimalAacCapsSink(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsAacCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, AVDTP_MEDIA_CODEC_MPEG2_4_AAC) && 
        a2dpFindStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, AVDTP_MEDIA_CODEC_MPEG2_4_AAC))
    {
        selectOptimalAacCapsSource(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsAptxCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindNonStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, A2DP_APT_VENDOR_ID, A2DP_CSR_APTX_CODEC_ID) && 
        a2dpFindNonStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, A2DP_APT_VENDOR_ID, A2DP_CSR_APTX_CODEC_ID))
    {
        selectOptimalCsrAptxCapsSink(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void selectOptimalCsrTwsAptxCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps)
{
    if (a2dpFindNonStdEmbeddedCodecCaps((const u8 **)&local_codec_caps, A2DP_APT_VENDOR_ID, A2DP_CSR_APTX_CODEC_ID) && 
        a2dpFindNonStdEmbeddedCodecCaps((const u8 **)&remote_codec_caps, A2DP_APT_VENDOR_ID, A2DP_CSR_APTX_CODEC_ID))
    {
        selectOptimalCsrAptxCapsSource(local_codec_caps, remote_codec_caps);
    }
}


/*****************************************************************************/
void getCsrTwsSbcConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings)
{
    if (a2dpFindStdEmbeddedCodecCaps(&service_caps, AVDTP_MEDIA_CODEC_SBC))
    {
        getSbcConfigSettings(service_caps, codec_settings);
    }
}


/*****************************************************************************/
void getCsrTwsMp3ConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings)
{
    if (a2dpFindStdEmbeddedCodecCaps(&service_caps, AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO))
    {
        getMp3ConfigSettings(service_caps, codec_settings);
    }
}


/*****************************************************************************/
void getCsrTwsAacConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings)
{
    if (a2dpFindStdEmbeddedCodecCaps(&service_caps, AVDTP_MEDIA_CODEC_MPEG2_4_AAC))
    {
        getAacConfigSettings(service_caps, codec_settings);
    }
}


/*****************************************************************************/
void getCsrTwsAptxConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings)
{
    if (a2dpFindNonStdEmbeddedCodecCaps(&service_caps, A2DP_APT_VENDOR_ID, A2DP_CSR_APTX_CODEC_ID))
    {
        getCsrAptxConfigSettings(service_caps, codec_settings);
    }
}
#else
    static const int dummy;
#endif  /* A2DP_SBC_ONLY */
