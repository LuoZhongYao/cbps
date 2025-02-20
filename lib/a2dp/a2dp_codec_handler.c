/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_sep_handler.c

DESCRIPTION
    This file contains

NOTES

*/


/****************************************************************************
    Header files
*/

#include "a2dp_codec_handler.h"
#include "a2dp_data_block.h"
#include "a2dp_caps_parse.h"
#include "a2dp_codec_aac.h"
#include "a2dp_codec_atrac.h"
#include "a2dp_codec_csr_aptx.h"
#include "a2dp_codec_csr_aptx_acl_sprint.h"
#include "a2dp_codec_csr_faststream.h"
#include "a2dp_codec_csr_tws.h"
#include "a2dp_codec_mp3.h"
#include "a2dp_codec_sbc.h"
#include "a2dp_command_handler.h"

#include <print.h>
#include <string.h>
#include <stdlib.h>


#ifndef A2DP_SBC_ONLY
/*****************************************************************************/
static bool isCodecCsrFaststream(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_FASTSTREAM_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrAptx(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_APT_VENDOR_ID) && (codec_id == A2DP_CSR_APTX_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrAptxAclSprint(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_APTX_ACL_SPRINT_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrTwsSbc(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_TWS_SBC_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrTwsMp3(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_TWS_MP3_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrTwsAac(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_TWS_AAC_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
static bool isCodecCsrTwsAptx(const u8 *codec_caps)
{
    u32 vendor_id;
    u16 codec_id;

    vendor_id = a2dpConvertUint8ValuesToUint32(&codec_caps[4]);
    codec_id = (codec_caps[8] << 8) | codec_caps[9];

    if ((vendor_id == A2DP_CSR_VENDOR_ID) && (codec_id == A2DP_CSR_TWS_APTX_CODEC_ID))
    {
        return TRUE;
    }

    return FALSE;
}
#endif /* A2DP_SBC_ONLY */


static void clampSbcBitpool(remote_device *device, const u8 *codec_caps, codec_data_type *codec_data)
{
    /* As we support the full SBC range, we limit our bit pool range to the values passed by the other side. */
    /* Store configuration in SBC format */
    (void)a2dpFindStdEmbeddedCodecCaps(&codec_caps, AVDTP_MEDIA_CODEC_SBC);
    codec_data->format = a2dpSbcFormatFromConfig(codec_caps);

    /* Calculate the optimal bitpool to use for the required data rate */   /* TODO: Make multi-stream aware */
    if ((codec_data->format & 0x0c) == 0)
    {   /* Mono mode - 1 channel */
        codec_data->bitpool = a2dpSbcSelectBitpool(codec_data->format, SBC_ONE_CHANNEL_RATE, codec_data->packet_size);
    }
    else
    {   /* All other modes are 2 channel */
        codec_data->bitpool = a2dpSbcSelectBitpool(codec_data->format, SBC_TWO_CHANNEL_RATE, codec_data->packet_size);
    }

    /* Clamp bitpool to remote device's limits. TODO: B-4407 we could try and use a lower data rate. */
    if (codec_data->bitpool < codec_caps[6])  /* Min bitpool */
    {
        codec_data->bitpool = codec_caps[6];
    }

    if (codec_data->bitpool > codec_caps[7])  /* Max bitpool */
    {
        codec_data->bitpool = codec_caps[7];
    }
}


/*****************************************************************************/
bool a2dpSelectOptimalCodecSettings(remote_device *device, u8 *remote_codec_caps)
{
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->sep_config;
    const u8 *local_codec_caps = sep_config->caps;

    
    if ( !a2dpFindCodecSpecificInformation(&local_codec_caps,0) || !a2dpFindCodecSpecificInformation((const u8**)&remote_codec_caps, 0) )
    {   /* Unable to locate actual start of local/remote codec service caps */
        return FALSE;
    }
    
    /* If this is a sink we need to inform the client of the codec settings */
    if (sep_config->role == a2dp_sink)
    {
        PRINT(("a2dp_sink: "));
        /* Determine the optimal codec settings */
        switch ( local_codec_caps[3] )
        {
        case AVDTP_MEDIA_CODEC_SBC:
            PRINT(("AVDTP_MEDIA_CODEC_SBC\n"));
            selectOptimalSbcCapsSink(local_codec_caps, remote_codec_caps);
            break;
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
            PRINT(("AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO\n"));
            selectOptimalMp3CapsSink(local_codec_caps, remote_codec_caps);
            break;
#endif
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
            PRINT(("AVDTP_MEDIA_CODEC_MPEG2_4_AAC\n"));
            selectOptimalAacCapsSink(local_codec_caps, remote_codec_caps);
            break;
#endif
#ifdef INCLUDE_ATRAC
        case AVDTP_MEDIA_CODEC_ATRAC:
            PRINT(("AVDTP_MEDIA_CODEC_ATRAC\n"));
            selectOptimalAtracCapsSink(local_codec_caps, remote_codec_caps);
            break;
#endif /* INCLUDE_ATRAC */
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_NONA2DP:
            PRINT(("AVDTP_MEDIA_CODEC_NONA2DP - "));
            if ( isCodecCsrFaststream(local_codec_caps) && isCodecCsrFaststream(remote_codec_caps) )
            {
                PRINT(("FastStream\n"));
                selectOptimalCsrFastStreamCapsSink(local_codec_caps, remote_codec_caps);
                break;
                
            }
            else if (isCodecCsrAptxAclSprint(local_codec_caps) && isCodecCsrAptxAclSprint(remote_codec_caps) )
            {
                PRINT(("AptX Sprint\n"));
                selectOptimalCsrAptxAclSprintCapsSink(local_codec_caps, remote_codec_caps);
                break;
            }
            else if (isCodecCsrAptx(local_codec_caps) && isCodecCsrAptx(remote_codec_caps) )
            {
                PRINT(("AptX\n"));
                selectOptimalCsrAptxCapsSink(local_codec_caps, remote_codec_caps);
                break;
            }
            else if (isCodecCsrTwsSbc(local_codec_caps))
            {
                PRINT(("TWS-SBC\n"));
                selectOptimalCsrTwsSbcCapsSink(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsMp3(local_codec_caps))
            {
                PRINT(("TWS-MP3\n"));
                selectOptimalCsrTwsSbcCapsSink(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsAac(local_codec_caps))
            {
                PRINT(("TWS-AAC\n"));
                selectOptimalCsrTwsAacCapsSink(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsAptx(local_codec_caps))
            {
                PRINT(("TWS-APTX\n"));
                selectOptimalCsrTwsAptxCapsSink(local_codec_caps, remote_codec_caps);
            }
            else
            {
                PRINT(("UNSUPPORTED - [local vendor=0x%lX id=0x%X] [remote vendor=0x%lX id=0x%X]\n",a2dpConvertUint8ValuesToUint32(&local_codec_caps[4]),((local_codec_caps[8] << 8) | local_codec_caps[9]),a2dpConvertUint8ValuesToUint32(&remote_codec_caps[4]),(remote_codec_caps[8] << 8) | remote_codec_caps[9]));
                return FALSE;
            }
            break;
            
#endif
        default:
            break;
        }
    }
    else
    {
        PRINT(("a2dp_source: "));
        /* Local device is a source of one type or another */
        switch ( local_codec_caps[3] )
        {
        case AVDTP_MEDIA_CODEC_SBC:
            PRINT(("AVDTP_MEDIA_CODEC_SBC\n"));
            selectOptimalSbcCapsSource(local_codec_caps, remote_codec_caps);
            break;
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
            PRINT(("AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO\n"));
            selectOptimalMp3CapsSource(local_codec_caps, remote_codec_caps);
            break;
#endif
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
            PRINT(("AVDTP_MEDIA_CODEC_MPEG2_4_AAC\n"));
            selectOptimalAacCapsSource(local_codec_caps, remote_codec_caps);
            break;
#endif
#ifdef INCLUDE_ATRAC
        case AVDTP_MEDIA_CODEC_ATRAC:
            PRINT(("AVDTP_MEDIA_CODEC_ATRAC\n"));
            /* Not Yet Implemented */
            /*
            selectOptimalAtracCapsSource(local_codec_caps, remote_codec_caps);
            */
            return FALSE;
            break;
#endif
#ifndef A2DP_SBC_ONLY
        case AVDTP_MEDIA_CODEC_NONA2DP:
            PRINT(("AVDTP_MEDIA_CODEC_NONA2DP - "));
            if ( isCodecCsrFaststream(local_codec_caps) )
            {
                PRINT(("FastStream\n"));
                selectOptimalCsrFastStreamCapsSource(local_codec_caps, remote_codec_caps);
                break;
            }
            else if (isCodecCsrAptxAclSprint(local_codec_caps) ) 
            {
                PRINT(("AptX Sprint\n"));
                selectOptimalCsrAptxAclSprintCapsSource(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrAptx(local_codec_caps) ) 
            {
                PRINT(("AptX\n"));
                selectOptimalCsrAptxCapsSource(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsSbc(local_codec_caps))
            {
                PRINT(("TWS-SBC\n"));
                selectOptimalCsrTwsSbcCapsSource(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsMp3(local_codec_caps))
            {
                PRINT(("TWS-MP3\n"));
                selectOptimalCsrTwsMp3CapsSource(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsAac(local_codec_caps))
            {
                PRINT(("TWS-AAC\n"));
                selectOptimalCsrTwsAacCapsSource(local_codec_caps, remote_codec_caps);
            }
            else if (isCodecCsrTwsAptx(local_codec_caps))
            {
                PRINT(("TWS-APTX\n"));
                selectOptimalCsrTwsAptxCapsSource(local_codec_caps, remote_codec_caps);
            }
            else
            {
                PRINT(("UNSUPPORTED - [local vendor=0x%lX id=0x%X] [remote vendor=0x%lX id=0x%X]\n",a2dpConvertUint8ValuesToUint32(&local_codec_caps[4]),((local_codec_caps[8] << 8) | local_codec_caps[9]),a2dpConvertUint8ValuesToUint32(&remote_codec_caps[4]),(remote_codec_caps[8] << 8) | remote_codec_caps[9]));
                return FALSE;
            }
            break;
            
#endif
        default:
            return FALSE;
        }
    }

    return TRUE;
}


/*****************************************************************************/
a2dp_codec_settings * a2dpGetCodecAudioParams (remote_device *device)
{
    sep_data_type *current_sep;
    const u8 *codec_caps;
    const u8 *service_caps;
    u16 size_service_caps;
    a2dp_codec_settings *codec_settings;
    
    if ((codec_caps = (service_caps = blockGetBase( device->device_id, data_block_configured_service_caps ))) == NULL)
    {
        return NULL;
    }
    
    size_service_caps = blockGetSize( device->device_id, data_block_configured_service_caps );
    codec_settings = (a2dp_codec_settings *)PanicNull( malloc(sizeof(a2dp_codec_settings) + size_service_caps) );
    memset(codec_settings, 0, sizeof(a2dp_codec_settings) + size_service_caps);  /* Zero the Codec settings */
    codec_settings->size_configured_codec_caps = size_service_caps;
    memmove(codec_settings->configured_codec_caps, service_caps, size_service_caps);

    codec_settings->codecData.content_protection = a2dpGetContentProtection(service_caps, size_service_caps, NULL);
    codec_settings->codecData.latency_reporting = a2dpIsServiceSupported(AVDTP_SERVICE_DELAY_REPORTING, service_caps, size_service_caps);

    if (!a2dpFindCodecSpecificInformation(&codec_caps, &size_service_caps))
    {
        free(codec_settings);
        return NULL;
    }
    
    if ((current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list )) == NULL)
    {
        free(codec_settings);
        return NULL;
    }

    codec_settings->seid = current_sep->sep_config->seid;
    codec_settings->sink = device->media_conn[0].connection.active.sink;  /* TODO: Make multi-stream aware */
    
    codec_settings->codecData.packet_size = device->media_conn[0].connection.active.mtu;
    
    switch ( codec_caps[3] )
    {
    case AVDTP_MEDIA_CODEC_SBC:
        clampSbcBitpool(device, codec_caps, &codec_settings->codecData);
        getSbcConfigSettings(codec_caps, codec_settings);
        break;

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_MPEG1_2_AUDIO:
        getMp3ConfigSettings(codec_caps, codec_settings);
        break;
#endif

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_MPEG2_4_AAC:
        getAacConfigSettings(codec_caps, codec_settings);
        break;
#endif

#ifdef INCLUDE_ATRAC
    case AVDTP_MEDIA_CODEC_ATRAC:
        getAtracConfigSettings(codec_caps, codec_settings);
        break;
#endif /* INCLUDE_ATRAC */

#ifndef A2DP_SBC_ONLY
    case AVDTP_MEDIA_CODEC_NONA2DP:
        if (isCodecCsrFaststream(codec_caps))
        {
            /* Get the config settings so they can be sent to the client */
            getCsrFastStreamConfigSettings(codec_caps, current_sep->sep_config->role, codec_settings);
        }
        else if (isCodecCsrAptxAclSprint(codec_caps))
        {
            /* Get the config settings so they can be sent to the client */
            getCsrAptxAclSprintConfigSettings(codec_caps, codec_settings);
        }
        else if (isCodecCsrAptx(codec_caps))
        {
            /* Get the config settings so they can be sent to the client */
            getCsrAptxConfigSettings(codec_caps, codec_settings);
        }
        else if (isCodecCsrTwsSbc(codec_caps))
        {
            clampSbcBitpool(device, codec_caps, &codec_settings->codecData);
            getCsrTwsSbcConfigSettings(codec_caps, codec_settings);
        }
        else if (isCodecCsrTwsMp3(codec_caps))
        {
            getCsrTwsMp3ConfigSettings(codec_caps, codec_settings);
        }
        else if (isCodecCsrTwsAac(codec_caps))
        {
            getCsrTwsAacConfigSettings(codec_caps, codec_settings);
        }
        else if (isCodecCsrTwsAptx(codec_caps))
        {
            getCsrTwsAptxConfigSettings(codec_caps, codec_settings);
        }
        
        break;
#endif

    default:
        free(codec_settings);
        return NULL;
    }

    /* Tell the client so it can configure the codec */
    return codec_settings;
}

