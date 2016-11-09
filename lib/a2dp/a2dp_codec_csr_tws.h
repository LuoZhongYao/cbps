/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_csr_tws.h
    
DESCRIPTION
	
*/

#ifndef A2DP_CSR_TWS_H_
#define A2DP_CSR_TWS_H_

#ifndef A2DP_SBC_ONLY

/*************************************************************************/
void selectOptimalCsrTwsSbcCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsSbcCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsMp3CapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsMp3CapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsAacCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsAacCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*************************************************************************/
void selectOptimalCsrTwsAptxCapsSink(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void selectOptimalCsrTwsAptxCapsSource(const u8 *local_codec_caps, u8 *remote_codec_caps);

/*****************************************************************************/
void getCsrTwsSbcConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings);

/*****************************************************************************/
void getCsrTwsMp3ConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings);

/*****************************************************************************/
void getCsrTwsAacConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings);

/*****************************************************************************/
void getCsrTwsAptxConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings);

#endif  /* A2DP_SBC_ONLY */

#endif  /* A2DP_CSR_TWS_H_ */
