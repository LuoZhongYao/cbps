/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_codec_csr_aptx.h

DESCRIPTION

*/

#ifndef A2DP_CODEC_CSR_APTX_H_
#define A2DP_CODEC_CSR_APTX_H_

#ifndef A2DP_SBC_ONLY

/*************************************************************************
NAME    
     selectOptimalCsrAptxCapsSink
    
DESCRIPTION
  Selects the optimal configuration for apt-X playback by setting a single 
	bit in each field of the passed caps structure.

	Note that the priority of selecting features is a
	design decision and not specified by the A2DP profiles.
   
*/
void selectOptimalCsrAptxCapsSink(const u8 *local_caps, u8 *remote_caps);


/*************************************************************************
NAME    
     selectOptimalCsrAptxCapsSource
    
DESCRIPTION
  Selects the optimal configuration for apt-X playback by setting a single 
	bit in each field of the passed caps structure.

	Note that the priority of selecting features is a
	design decision and not specified by the A2DP profiles.
   
*/
void selectOptimalCsrAptxCapsSource(const u8 *local_caps, u8 *remote_caps);


/*************************************************************************
NAME    
     getCsrAptxConfigSettings
    
DESCRIPTION    
	Return the codec configuration settings (rate and channel mode) for the physical codec based
	on the A2DP codec negotiated settings.
*/
void getCsrAptxConfigSettings(const u8 *service_caps, a2dp_codec_settings *codec_settings);

#endif  /* A2DP_SBC_ONLY */

#endif  /* A2DP_CODEC_CSR_APTX_H_ */
