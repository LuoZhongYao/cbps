// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************
#ifndef SBC_WBS_ONLY
#ifndef SBC_FRAME_ENCODE_INCLUDED
#define SBC_FRAME_ENCODE_INCLUDED

#include "core_library.h"
#include "codec_library.h"
#include "sbc_profiler_macros.h"

#include "sbc.h"

// *****************************************************************************
// MODULE:
//    $sbcenc.frame_encode
//
// DESCRIPTION:
//    Encode an SBC frame
//
// INPUTS:
//    - r5 = pointer to a $codec.ENCODER_STRUC structure
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    assume everything
//
// NOTES:
//    To support mono, stereo, and dual mono encoding the operation of the
//    routine is as follows:
//
//    @verbatim
//    nrof_ch | left_buf | right_buf
//    --------|----------|-----------
//       1    | enabled  | disabled    - Mono encoding with left data [1]
//       1    | disabled | enabled     - Mono encoding with right data [1]
//       1    | enabled  | enabled     - Mono encoding with (left + right)
//       2    | enabled  | enabled     - Standard stereo encoding
//       2    | enabled  | disabled    - Stereo encoding but with right = left
//       2    | disabled | enabled     - Stereo encoding but with left = right
//    @endverbatim
//
//  [1] These modes are designed to support wireless speaker scenarios were a
//  single Bluecore will stream an individual left and right sbc stream.
//  The routine will make sure that state information about each stream is
//  stored individually.
//
// *****************************************************************************
//
//  @verbatim
//  audio_frame()
//  (
//     frame_header()
//     scale_factors()
//     audio_samples()
//     padding()
//  )
//  @endverbatim
//
// *****************************************************************************
.MODULE $M.sbcenc.frame_encode;
   .CODESEGMENT SBCENC_FRAME_ENCODE_PM;
   .DATASEGMENT DM;



   $sbcenc.frame_encode:

   // push rLink onto stack
   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_0, r1)
#endif

   // -- Start overall profiling if enabled --
   PROFILER_START_SBCENC(&$sbcenc.profile_frame_encode)

   // -- Save $codec.ENCODER_STRUC pointer --
   M[$sbcenc.codec_struc] = r5;


   // -- Load memory structure pointer
   // This pointer should have been initialised externally
   r9 = M[r5 + $codec.ENCODER_DATA_OBJECT_FIELD];


   // -- Setup encoder parameters --
   call $sbcenc.encoder_set_parameters;

   // -- Check that we have enough input audio data --
   r0 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
   r1 = M[r9 + $sbc.mem.NROF_BLOCKS_FIELD];

   r4 = r0 * r1 (int);
   r0 = M[r5 + $codec.ENCODER_IN_LEFT_BUFFER_FIELD];
   if Z jump input_check_no_left_channel;
      call $cbuffer.calc_amount_data;
      Null = r0 - r4;
      if POS jump enough_input_data_left;
         jump not_enough_data;
      enough_input_data_left:
   input_check_no_left_channel:

   r0 = M[r5 + $codec.ENCODER_IN_RIGHT_BUFFER_FIELD];
   if Z jump input_check_no_right_channel;
      call $cbuffer.calc_amount_data;
      Null = r0 - r4;
      if POS jump enough_input_data_right;
      not_enough_data:
         r0 = $codec.NOT_ENOUGH_INPUT_DATA;
         M[r5 + $codec.ENCODER_MODE_FIELD] = r0;
         jump exit;
      enough_input_data_right:
   input_check_no_right_channel:


   // -- Check that we have enough output space for sbc data --
   call $sbc.calc_frame_length;
   r4 = r0 ASHIFT -1;
   // add 3 for safety
   r4 = r4 + 3;
   r0 = M[r5 + $codec.ENCODER_OUT_BUFFER_FIELD];
   call $cbuffer.calc_amount_space;
   Null = r0 - r4;
   if POS jump enough_output_space;
      r0 = $codec.NOT_ENOUGH_OUTPUT_SPACE;
      M[r5 + $codec.ENCODER_MODE_FIELD] = r0;
      jump exit;
   enough_output_space:


#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_1, r1)
#endif

   // -- Analysis subband filtering Left --

   // setup variables for share code block

   r0 = M[r5 + $codec.ENCODER_IN_LEFT_BUFFER_FIELD];
   M[$scratch.s0] = r0;
   r0 = M[r5 + $codec.ENCODER_IN_RIGHT_BUFFER_FIELD];
   M[$scratch.s1] = r0;
   r0 = 0;
   M[$scratch.s2] = r0;


   call channel_processing;

   // -- Analysis subband filtering Right --

   // setup variables for share code block

   r0 = M[r5 + $codec.ENCODER_IN_LEFT_BUFFER_FIELD];
   M[$scratch.s1] = r0;
   r0 = M[r5 + $codec.ENCODER_IN_RIGHT_BUFFER_FIELD];
   M[$scratch.s0] = r0;
   r0 = 1;
   M[$scratch.s2] = r0;


   call channel_processing;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_2, r2)
#endif

   // -- Merge stereo audio inputs if mono sbc output --

   r0 = M[r9 + $sbc.mem.NROF_CHANNELS_FIELD];

   Null = r0 - 1;
   if NZ jump dont_merge_left_and_right;
      r0 = M[r5 + $codec.ENCODER_IN_LEFT_BUFFER_FIELD];
      if Z jump dont_merge_left_and_right;
      r0 = M[r5 + $codec.ENCODER_IN_RIGHT_BUFFER_FIELD];
      if Z jump dont_merge_left_and_right;


      // audio_sample(mono) = audio_sample(left)/2 + audio_sample(right)/2;
      r2 = M[r9 + $sbc.mem.AUDIO_SAMPLE_FIELD];
      I0 = r2;
      r10 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];

      I1 = I0 + r10;
      I2 = I0;

      r2 = M[r9 + $sbc.mem.NROF_BLOCKS_FIELD];


      mono_merge_block_loop:
         do mono_merge_subband_loop;
            r0 = M[I0, 1];
            r0 = r0 * 0.5 (frac);
            r1 = M[I1, 1];
            r1 = r1 * 0.5 (frac);
            r0 = r0 + r1;
            M[I2, 1] = r0;
         mono_merge_subband_loop:

         r10 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];


         I0 = I0 + r10;
         I1 = I1 + r10;
         r2 = r2 - 1;
      if NZ jump mono_merge_block_loop;
   dont_merge_left_and_right:


   // -- Calculate scalefactors --
   PROFILER_START_SBCENC(&$sbcenc.profile_calc_scale_factors)
   call $sbc.calc_scale_factors;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_calc_scale_factors)


   // -- Joint stereo encode if required --
   PROFILER_START_SBCENC(&$sbcenc.profile_joint_stereo_encode)

   r0 = M[r9 + $sbc.mem.CHANNEL_MODE_FIELD];

   Null = r0 - $sbc.JOINT_STEREO;
   if Z call $sbcenc.joint_stereo_encode;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_joint_stereo_encode)

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_3, r5)
#endif

   // -- Setup SBC output stream buffer info --
   // set I0 to point to cbuffer for sbc output stream
   r5 = M[$sbcenc.codec_struc];
   r0 = M[r5 + $codec.ENCODER_OUT_BUFFER_FIELD];
#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   pop B0;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   I0 = r0;
   L0 = r1;

   // Pre-Processing Hook
   r5 = M[$sbcenc.codec_struc];
   r1 = M[r9 + $sbc.mem.PRE_POST_PROC_STRUC_FIELD];

   r0 = M[ r1 + $codec.pre_post_proc.PRE_PROC_FUNC_ADDR_FIELD];
   if NZ call r0;

   // -- Write frame header --
   PROFILER_START_SBCENC(&$sbcenc.profile_write_frame_header)
   call $sbcenc.write_frame_header;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_write_frame_header)


   // -- Write scale factors --
   PROFILER_START_SBCENC(&$sbcenc.profile_write_scale_factors)
   call $sbcenc.write_scale_factors;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_write_scale_factors)


   // -- Calculate bit allocation --
   PROFILER_START_SBCENC(&$sbcenc.profile_calc_bit_allocation)
   call $sbc.calc_bit_allocation;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_calc_bit_allocation)


   // -- Quantize samples --
   PROFILER_START_SBCENC(&$sbcenc.profile_quantize_samples)
   call $sbcenc.quantize_samples;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_quantize_samples)


   // -- Write audio samples --
   PROFILER_START_SBCENC(&$sbcenc.profile_write_audio_samples)
   call $sbcenc.write_audio_samples;
   PROFILER_STOP_SBCENC(&$sbcenc.profile_write_audio_samples)


   // -- Write padding bits --
   call $sbcenc.write_padding_bits;


   // Post Processing Hook

   r1 = M[r9 + $sbc.mem.PRE_POST_PROC_STRUC_FIELD];
   r5 = M[$sbcenc.codec_struc];

   r0 = M[ r1 + $codec.pre_post_proc.POST_PROC_FUNC_ADDR_FIELD];
   if NZ call r0;


   // -- Save back SBC input stream buffer info --
   // store updated cbuffer pointers for sbc input stream
   r5 = M[$sbcenc.codec_struc];
   r1 = I0;
   r0 = M[r5 + $codec.ENCODER_OUT_BUFFER_FIELD];
   call $cbuffer.set_write_address;
   L0 = 0;


   #ifdef DEBUG_SBCENC
      // -- increment frame counter --
      r0 = M[$sbcenc.framecount];
      r0 = r0 + 1;
      M[$sbcenc.framecount] = r0;
   #endif


   // -- update $codec.ENCODER_STRUC --
   r0 = $codec.SUCCESS;
   M[r5 + $codec.ENCODER_MODE_FIELD] = r0;


   exit:
   // -- Stop overall profiling if enabled --
   PROFILER_STOP_SBCENC(&$sbcenc.profile_frame_encode)

   // pop rLink from stack
   jump $pop_rLink_and_rts;




channel_processing:

   // push rLink onto stack
   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_4, r1)
#endif


   r0 = M[$scratch.s0];

   if Z jump no_current_channel;

#ifdef BASE_REGISTER_MODE
      call $cbuffer.get_read_address_and_size_and_start_address;
      push r2;
      pop B1;
#else
      call $cbuffer.get_read_address_and_size;
#endif
      I1 = r0;
      L1 = r1;

      // select sample increment per block
      //  = 0 for proper mono
      //  = nrof_subbands for stereo and mono (if converted from a stereo input)
      r6 = 0;

      r1 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
      r0 = M[r9 + $sbc.mem.NROF_CHANNELS_FIELD];


      Null = r0 - 2;
      if Z r6 = r1;

      r0 = M[$scratch.s1];

      if NZ r6 = r1;

      // select left channel

      r5 = M[$scratch.s2];


      // process input samples
      PROFILER_START_SBCENC(&$sbcenc.profile_analysis_subband_filter)
      call $sbcenc.analysis_subband_filter;
      PROFILER_STOP_SBCENC(&$sbcenc.profile_analysis_subband_filter)

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($sbcenc.FRAME_ENCODE_ASM.FRAME_ENCODE.PATCH_ID_5, r1)
#endif

      // store updated cbuffer pointers for left audio input
      r5 = M[$sbcenc.codec_struc];
      r1 = I1;

      r0 = M[$scratch.s0];

      call $cbuffer.set_read_address;
      L1 = 0;

      // if stereo and no right audio buffer available then repeat the left channel

      r0 = M[r9 + $sbc.mem.NROF_CHANNELS_FIELD];

      Null = r0 - 1;
      if Z jump dont_repeat_cur_channel;

         r0 = M[$scratch.s1];

         if NZ jump dont_repeat_cur_channel;

            // set I1 = left samples
            // set I2 = right samples (ie. where we copy to)
            r4 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];
            r3 = M[r9 + $sbc.mem.AUDIO_SAMPLE_FIELD];
            I1 = r3;
            I2 = I1 + r4;

            // loop around all blocks
            r3 = M[r9 + $sbc.mem.NROF_BLOCKS_FIELD];

            repeat_cur_channel_block_loop:

              // loop around all subbands

               r10 = M[r9 + $sbc.mem.NROF_SUBBANDS_FIELD];

               do repeat_cur_channel_subband_loop;
                  r0 = M[I1, 1];
                  M[I2, 1] = r0;
               repeat_cur_channel_subband_loop:
               // move into the next block
               I1 = I1 + r4;
               I2 = I2 + r4;
               r3 = r3 - 1;
            if NZ jump repeat_cur_channel_block_loop;

      dont_repeat_cur_channel:
   no_current_channel:

   // pop rLink from stack
   jump $pop_rLink_and_rts;


.ENDMODULE;

#endif
#endif
