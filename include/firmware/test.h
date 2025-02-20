/*
 * Copyright 2015 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.0
 */

#ifndef __TEST_H

#define __TEST_H

#include <csrtypes.h>
/*! @file test.h @brief Functions to enter BlueCore test modes */
#define FIRST_PCM_IF 0


/*!
  @brief Internal function. 
  Don't call this function directly.
  Don't rely on this being present.
*/
bool TestPerform_(u16 len, const u16 *args);

/*!
    @brief Radio test pause.
*/
bool TestPause(void);

/*!
   @brief Radio test tx start.
   @param lo_freq  The lo_freq to use.
   @param level The level to use. The lower 8 bits are the internal gain. 
   The upper 8 bits are the external gain.
   @param mod_freq The mod_freq to use.
*/
bool TestTxStart(u16 lo_freq, u16 level, u16 mod_freq);

/*!
  @brief Radio test tx data 1.
  @param lo_freq  Local oscillator frequency to use.
  @param level    Transmit level to use. The lower 8 bits are the internal gain. 
  The upper 8 bits are the external gain.
*/
bool TestTxData1(u16 lo_freq, u16 level);

/*!
   @brief Radio test tx data 2.
   @param cc The cc to use.
   @param level The level to use. The lower 8 bits are the internal gain. 
   The upper 8 bits are the external gain.
*/
bool TestTxData2(u16 cc, u16 level);

/*!
   @brief Radio test rx.
   @param lo_freq The lo_freq to use.
   @param highside The high side to use.
   @param attn The attn to use. 
*/
bool TestRxStart(u16 lo_freq, u16 highside, u16 attn);

/*!
   @brief Radio test deep sleep.
*/
bool TestDeepSleep(void);

/*!
   @brief  Configure hardware loopback for PCM port.
   @param  pcm_mode  Chosen loopback mode. Valid values: 0, 1, 2
*/
bool TestPcmLb(u16 pcm_mode);

/*!
   @brief  Radio test loop back.
   @param lo_freq The lo_freq to use.
   @param level The level to use.
*/
bool TestLoopback(u16 lo_freq, u16 level);

/*!
   @brief  Configure external hardware loopback for PCM port.
           A block of random data is written to the PCM output
           port and is read back again on the PCM input port. 
   @param pcm_mode  Chosen loopback mode. Valid values: 0, 1, 2
*/
bool TestPcmExtLb(u16 pcm_mode);

/*!
   @brief  Radio test for configuring the crystal trim value.
   @param  xtal_ftrim Selected crystal trim value.
*/
bool TestCfgXtalFtrim(u16 xtal_ftrim);

/*!
   @brief  Play a constant tone on the PCM port (or the codec for
           BC02 with PSKEY_HOSTIO_MAP_SCO_CODEC set).
   @param  freq  Chosen frequency.
   @param  ampl  Chosen amplitude.
   @param  dc    Specifies a constant offset to add to the audio data.
*/
bool TestPcmTone(u16 freq, u16 ampl, u16 dc);

/*!
   @brief  Turn on codec hardware for stereo loopback 
   @param  samp_rate     Sampling rate. Valid values: 8000, 11025, 
                         16000, 22050, 24000, 32000 and 44100
   @param  reroute_optn  Routing option. Valid values: 0, 1, 2, 3
*/ 
bool TestCodecStereoLb(u16 samp_rate, u16 reroute_optn);

/*!
   @brief  Play a constant tone on the PCM port (or the codec for
           BC02 with PSKEY_HOSTIO_MAP_SCO_CODEC set).
   @param  freq  Chosen frequency. Valid range: 0 (low) to 3 (high).
   @param  ampl  Chosen amplitude. Valid range: 0 (minimum) to 8 (maximum).
   @param  dc    Specifies a constant offset to add to the audio data.
   @param  interface Chosen PCM interface. A value from the #audio_instance enumeration.
   @return TRUE if successful, else FALSE.
*/
bool TestPcmToneIf(u16 freq, u16 ampl, u16 dc, u16 interface);

/*!
   @brief  Configure hardware loopback for PCM port.
   @param  pcm_mode  Chosen mode. Valid values: 0 (slave), 1 (master), 2 (Manchester slave)
   @param  interface Chosen PCM interface. A value from the #audio_instance enumeration.
   @return TRUE if successful, else FALSE.
*/
bool TestPcmLbIf(u16 pcm_mode, u16 interface);

/*!
   @brief  Configure external hardware loopback for PCM port.
           A block of random data is written to the PCM output
           port and is read back again on the PCM input port. 
   @param  pcm_mode  Chosen mode. Valid values: 0 (slave), 1 (master), 2 (Manchester slave)
   @param  interface Chosen PCM interface. A value from the #audio_instance enumeration.
   @return TRUE if successful, else FALSE.
*/
bool TestPcmExtLbIf(u16 pcm_mode, u16 interface);

#endif
