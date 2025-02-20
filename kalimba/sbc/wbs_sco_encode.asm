// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************

#include "stack.h"
#include "flash.h"
#include "sbc.h"
#include "cbuffer.h"

#define WBS_SBC_FRAME_SYNCHRONISATION_WORD_MASK   0x0108


.MODULE $M.wbsenc.wbs_pre_proc_func;

   .CODESEGMENT WBSENC_FRAME_ENCODE_PM;
   .DATASEGMENT DM;



$wbsenc.wbs_pre_proc_func:

   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbsenc.WBS_SCO_ENCODE_ASM.WBS_PRE_PROC_FUNC.PATCH_ID_0, r4)
#endif

      // Calculate the two top bits of the sequence number

   r4 = M[r9 + $sbc.mem.WBS_SEND_FRAME_COUNTER_FIELD];

   r2 = r4 AND 0x2;
   r3 = r2 LSHIFT 2;
   r2 = r2 LSHIFT 1;
   r3 = r3 OR r2;

   // Calculate the two bottom bits of the sequence number
   r2 = r4 AND 0x1;
   r1 = r2 LSHIFT 1;
   r1 = r1 OR r2;

   // Put them together into r2
   r2 = r1 OR r3;
   r2 = r2 LSHIFT 4;

   r1 = WBS_SBC_FRAME_SYNCHRONISATION_WORD_MASK;
   r1 = r1 OR r2;
   // r1 now contains the WBS synchronisation header
   // write the synchronisation word to the output
   r0 = 16;
   call $sbcenc.putbits;

   // Increment the frame counter
   r4 = r4 + 1;
   r4 = r4 AND 0x3;

   M[r9 + $sbc.mem.WBS_SEND_FRAME_COUNTER_FIELD] = r4;



   jump $pop_rLink_and_rts;

.ENDMODULE;


.MODULE $wbs_frame_encode;
   .DATASEGMENT DM;

   .VAR/DM1  cbuf_in_ptr;
   .VAR/DM1  cbuf_out_ptr;
   .VAR      can_idle;
.ENDMODULE;


.MODULE $M.wbsenc.wbs_frame_encode;
   .CODESEGMENT WBSENC_FRAME_ENCODE_PM;

$wbsenc.wbs_frame_encode:

   // push rLink onto stack
   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbsenc.WBS_SCO_ENCODE_ASM.WBS_FRAME_ENCODE.PATCH_ID_0, r1)
#endif

   call $wbsenc.set_up_frame_encode;

   // SP.  WB Frame is 128, WBS Packet input is 120, WBS output is 30 (60 bytes)
   // Every 15 frames and extra packet is generated

   r7 = 30;
jp_frame_loop:

   // -- Check that we have enough input audio data --
   r0 = M[$wbs_frame_encode.cbuf_in_ptr];
   call $cbuffer.calc_amount_data;
   Null = r0 - 120;
   if NEG jump exit;

   // -- Check that we have enough output space for wbs data --
   // MONO and DUAL_CHANNEL:
   //  frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +
   //                 ceil(nrof_blocks * nrof_channels * bitpool / 8)
   //r0 = 57;
   //r4 = r0 ASHIFT -1;
   // add 3 for safety
   //r4 = r4 + 3;

   // SP.  To minimize latency.  Limit output
   r0 = M[$wbs_frame_encode.cbuf_out_ptr];
   call $cbuffer.calc_amount_data;
   Null = r0 - r7;
   if POS jump exit;

   call $wbsenc.process_frame;

   // Allow an extra output packet
   r7 = 60;
   jump jp_frame_loop;

exit:
   r0 = M[$wbs_frame_encode.can_idle];
   jump $pop_rLink_and_rts;


.ENDMODULE;



.MODULE $M.wbsenc.set_up_frame_encode;
   .CODESEGMENT WBSENC_SET_UP_FRAME_ENCODE_PM;

   $wbsenc.set_up_frame_encode:

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbsenc.WBS_SCO_ENCODE_ASM.SET_UP_FRAME_ENCODE.PATCH_ID_0, r1)
#endif

   r0 = 1;
   M[$wbs_frame_encode.can_idle] = r0;

   // save r7, r8, used as buffer input parameters
   M[$wbs_frame_encode.cbuf_in_ptr]  = r7;
   M[$wbs_frame_encode.cbuf_out_ptr] = r8;



   // SP.  COnnection not included so no way to automate purge
   // if the output is not connected, purge input cbuffer and exit
   //r0 = M[r8 + $cbuffer.CONNECT_STATUS_FIELD];
   //if Z jump jp_purge_input;

   // -- Setup encoder parameters --
   // force mSBC settings
   M[r9 + $sbc.mem.SAMPLING_FREQ_FIELD] = Null;
   r1 = 15;
   M[r9 + $sbc.mem.NROF_BLOCKS_FIELD] = r1;
   M[r9 + $sbc.mem.CHANNEL_MODE_FIELD] = Null;
   r1 = 1;
   M[r9 + $sbc.mem.NROF_CHANNELS_FIELD] = r1;
   M[r9 + $sbc.mem.ALLOCATION_METHOD_FIELD] = Null;
   r1 = 8;
   M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD] = r1;
   r1 = 26;
   M[r9 + $sbc.mem.BITPOOL_FIELD] = r1;
   r1 = 1;
   M[r9 + $sbc.mem.FORCE_WORD_ALIGN_FIELD] = r1;



   rts;

.ENDMODULE;


.MODULE $M.wbsenc.process_frame;
   .CODESEGMENT WBSENC_PROCESS_FRAME_PM;

   $wbsenc.process_frame:

   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbsenc.WBS_SCO_ENCODE_ASM.PROCESS_FRAME.PATCH_ID_0, r1)
#endif

   // -- Analysis subband filtering Left --

   // setup variables for share code block
   r0 = M[$wbs_frame_encode.cbuf_in_ptr];
#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_read_address_and_size_and_start_address;
   push r2;
   pop B1;
#else
   call $cbuffer.get_read_address_and_size;
#endif
   I1 = r0;
   L1 = r1;

   M[$wbs_frame_encode.can_idle] = 0;

   // select sample increment per block
   //  = 0 for proper mono
   //  = nrof_subbands for stereo and mono (if converted from a stereo input)
   r6 = 0;
   // select left channel
   r5 = 0;
   // process input samples
   call $sbcenc.analysis_subband_filter;

   // store updated cbuffer pointers for left audio input
   r1 = I1;
   r0 = M[$wbs_frame_encode.cbuf_in_ptr];
   call $cbuffer.set_read_address;
   L1 = 0;
#ifdef BASE_REGISTER_MODE
   push Null;
   pop B1;
#endif
   // -- Calculate scalefactors --
   call $sbc.calc_scale_factors;

   // -- Setup WBS output stream buffer info --
   // set I0 to point to cbuffer for wbs output stream
   r0 = M[$wbs_frame_encode.cbuf_out_ptr];
#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   pop B0;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   I0 = r0;
   L0 = r1;

   // Set the WBS pre function to use
   call $wbsenc.wbs_pre_proc_func;

   // -- Write frame header --
   // init crc_checksum = 0x0f
   r0 = 0x0f;

   M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD] = r0;


   // write wbs header
   r0 = 8;
   r1 = 0xAD;
   call $sbcenc.putbits;
   r1 = 0x00;
   call $sbcenc.putbits;
   call $sbc.crc_calc;
   r1 = 0x00;
   call $sbcenc.putbits;
   call $sbc.crc_calc;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbsenc.WBS_SCO_ENCODE_ASM.PROCESS_FRAME.PATCH_ID_1, r1)
#endif

   // do crc on scalefactors
   r0 = M[r9 + $sbc.mem.SCALE_FACTOR_FIELD];
   I1 = r0;
   r0 = M[r9 + $sbc.mem.NROF_CHANNELS_FIELD];
   r1 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
   r6 = r0 * r1 (int);
   r0 = 4;
   scalefactor_loop:
      r1 = M[I1, 1];
      call $sbc.crc_calc;
      r6 = r6 - 1;
   if NZ jump scalefactor_loop;

   // write crc check word
   r0 = 8;

   r1 = M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD];

   // select just lower 8 bits
   r1 = r1 AND 0xff;
   call $sbcenc.putbits;


   // -- Write scale factors --
   call $sbcenc.write_scale_factors;


   // -- Calculate bit allocation --
   call $sbc.calc_bit_allocation;


   // -- Quantize samples --
   call $sbcenc.quantize_samples;


   // -- Write audio samples --
   call $sbcenc.write_audio_samples;


   // -- Write padding bits --
   // get aligned add zero padding bits
   r1 = 0;

   r0 = M[r9 + $sbc.mem.PUT_BITPOS_FIELD];

   r0 = r0 AND 0xF;
   if NZ call $sbcenc.putbits;


   // -- Save back WBS input stream buffer info --
   // store updated cbuffer pointers for wbs input stream
   r1 = I0;
   r0 = M[$wbs_frame_encode.cbuf_out_ptr];
   call $cbuffer.set_write_address;
   L0 = 0;
#ifdef BASE_REGISTER_MODE
   push Null;
   pop B0;
#endif

#ifdef DEBUG_WBSENC
      // -- increment frame counter --
      r0 = M[$sbcenc.framecount];
      r0 = r0 + 1;
      M[$sbcenc.framecount] = r0;
#endif

   jump $pop_rLink_and_rts;

.ENDMODULE;
