// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************


// *****************************************************************************
// NAME:
//    Mix operator
//
// DESCRIPTION:
//    This routine mixes n channels of audio streams with some other mono audio
// stream, e.g. Mix a decoded stereo SBC stream with WAV file data from the file
// system.
//
//    If this operator is to be used to mix tones generated by the VM with an
// audio stream decoded by the DSP it may be necessary to buffer the tone data
// in the DSP before passing it to this mixing routine. If this is the case,
// copy the data into an intermediate buffer WITHOUT applying any shift.
//
// When using the operator the following data structure is used:
//    - $cbops.mix.MIX_SOURCE_FIELD = mix stream source (address of cbuffer
//       structure)
//    - $cbops.mix.MIX_VOL_FIELD = volume to apply to mix stream data
//       (fractional in range 0 - 1.0)
//    - $cbops.mix.AUDIO_VOL_FIELD = volume to apply to audio data
//       (fractional in range 0 - 1.0)
//    - $cbops.mix.MIXING_STATE_FIELD = tone playing state (initialise as 0)
//    - $cbops.mix.MIXING_START_LEVEL_FIELD = the number of samples to have
//       in the mix input buffer before mixing starts
//    - $cbops.mix.NUMBER_OF_INPUTS_FIELD = the number of audio input
//       channels that the mix input is being mixed with
//    - $cbops.mix.INPUT_START_INDEX_FIELD = the start index of the inputs
//       in the operator copy framework parameter area
//
// *****************************************************************************

#include "stack.h"
#include "cbops.h"

.MODULE $M.cbops.mix;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops.mix[$cbops.function_vector.STRUC_SIZE] =
      &$cbops.mix.reset,                    // reset function
      $cbops.function_vector.NO_FUNCTION,   // amount to use function
      &$cbops.mix.main;                     // main function

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cbops.mix.reset
//
// DESCRIPTION:
//    Reset routine for the mix operator, see $cbops.mix.main
//
// INPUTS:
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

.MODULE $M.cbops.mix.reset;
   .CODESEGMENT CBOPS_MIX_RESET_PM;
   .DATASEGMENT DM;

   // ** reset routine **
   $cbops.mix.reset:
   // set the state to not mixing
   M[r8 + $cbops.mix.MIXING_STATE_FIELD] = Null;

   rts;

.ENDMODULE;




// *****************************************************************************
// MODULE:
//    $cbops.mix.main
//
// DESCRIPTION:
//    Mix a mono stream with each of the input streams.
//
// INPUTS:
//    - r6 = pointer to the list of input and output buffer pointers
//    - r7 = pointer to the list of buffer lengths
//    - r8 = pointer to operator structure
//    - r10 = the number of samples to process
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0-5, r10, I0, I4, L0, L4, DoLoop
//
// *****************************************************************************
.MODULE $M.cbops.mix.main;
   .CODESEGMENT CBOPS_MIX_MAIN_PM;
   .DATASEGMENT DM;

   // ** main function **
   $cbops.mix.main:

   // push rLink onto stack
   $push_rLink_macro;

   // start profiling if enabled
   #ifdef ENABLE_PROFILER_MACROS
      .VAR/DM1 $cbops.profile_mix[$profiler.STRUC_SIZE] = $profiler.UNINITIALISED, 0 ...;
      r0 = &$cbops.profile_mix;
      call $profiler.start;
   #endif

   // work out how much tone data we've got
   r0 = M[r8 + $cbops.mix.MIX_SOURCE_FIELD];

   #ifdef CBOPS_DEBUG
      // Check if the mix input is a port - this can't be handled
      Null = SIGNDET r0;
      if Z call $error;
   #endif // CBOPS_DEBUG

   call $cbuffer.calc_amount_data;

   // check to see if we are mixing tones
   r1 = M[r8 + $cbops.mix.MIXING_STATE_FIELD];
   if NZ jump mix_tone_data;

      r1 = M[r8 + $cbops.mix.MIXING_START_LEVEL_FIELD];
      Null = r0 - r1;
      // if not enough tones just exit
      if NEG jump done;

      // set state to playing
      r1 = $cbops.mix.MIXING_STATE_MIXING;
      M[r8 + $cbops.mix.MIXING_STATE_FIELD] = r1;

   mix_tone_data:

   // Get the number of input buffers that are being mixed with the mix input
   r2 = M[r8 + $cbops.mix.NUMBER_OF_INPUTS_FIELD];

   // Check if there are enough mix samples available
   // r0 contains the number of samples available
   Null = r10 - r0;
   if NEG jump dont_stop_mixing;
      // Set the state to stopped and adjust r10
      M[r8 + $cbops.mix.MIXING_STATE_FIELD] = $cbops.mix.MIXING_STATE_STOPPED;
      // Set r10 to the actual number of samples we have for mixing
      r10 = r0;
   dont_stop_mixing:


   // Get the mix and audio volume levels
   r4 = M[r8 + $cbops.mix.MIX_VOL_FIELD];
   r5 = M[r8 + $cbops.mix.AUDIO_VOL_FIELD];

   // Point I4 at the mix input source
   r0 = M[r8 + $cbops.mix.MIX_SOURCE_FIELD];
   call $cbuffer.get_read_address_and_size;
   I4 = r0;
   L4 = r1;

   // Store the loop counter for later
   M2 = r10;
   M1 = -r10;

   // Get the index of the first index to use
   r0 = M[r8 + $cbops.mix.INPUT_START_INDEX_FIELD];
   I1 = r0;

   mix_stream_loop:
      // Get the input buffer read address
      r3 = M[r6 + r0];
      // Store the value in I0
      I0 = r3;
      // Get the input buffer length
      r3 = M[r7 + r0];
      // Store the value in L0
      L0 = r3;

      // Decrement loop count (in M2) to do one sample outside the loop
      r10 = M2 - 1;

      // Get the first mix input sample
      r0 = M[I4,1];

      // Apply gain to mix input sample
      // and get the first stream input sample
      rMAC = r0 * r4, r1 = M[I0,1];

      // Apply gain to stream input sample and add to the mix input sample
      // and read the next stream sample
      // and read next tone sample
      rMAC = rMAC + r1 * r5, r1 = M[I0,-1], r0 = M[I4,1];

      do mix_loop_mono;

         // Apply gain to mix input sample
         // and write previous sample to the output buffer
         rMAC = r0 * r4, M[I0,2] = rMAC;

         // Apply gain to stream input sample and add to the mix input sample
         // and read the next stream sample
         // and read next tone sample
         rMAC = rMAC + r1 * r5, r1 = M[I0,-1], r0 = M[I4,1];

      mix_loop_mono:

      // Store the final left value
      // and do a dummy read to put I4 back in the correct place
      M[I0,1] = rMAC, r0 = M[I4,-1];

      // Set the buffer index for next time
      r0 = I1 + 1;

      // Decrement the input stream counter
      // If all streams have been mixed, jump out
      r2 = r2 - 1;
      if Z jump done_mixing;

      // Restore the index into the tone buffer by doing a dummy read
      r3 = M[I4, M1];

   jump mix_stream_loop;



   done_mixing:

   // Zero the length registers that have been used
   L0 = 0;
   L4 = 0;

   // Update the tone data buffer
   r1 = I4;

   // Always update the cbuffer
   r0 = M[r8 + $cbops.mix.MIX_SOURCE_FIELD];
   call $cbuffer.set_read_address;

   done:


   #ifdef ENABLE_PROFILER_MACROS
      r0 = &$cbops.profile_mix;
      call $profiler.stop;
   #endif

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

