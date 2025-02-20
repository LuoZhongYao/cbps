// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************


// *****************************************************************************
// FILE: wbs_decode.asm - 'WBS' SCO decoder API functions
// *****************************************************************************
#include "stack.h"
#include "cbuffer.h"
#include "wall_clock.h"
#ifndef WBS_DECOUPLED
#include "frame_codec.h"
#endif
#include "flash.h"

#include "sbc.h"
#include "cbuffer.h"

//#define SCOWBSDEBUG

.CONST $M.wbs.decoder.BFI_GOOD 0;
.CONST $M.wbs.decoder.BFI_BITERR 1;
.CONST $M.wbs.decoder.BFI_LOST 2;
.CONST $M.wbs.decoder.BFI_TIMING 3;

.CONST $M.wbs.decoder.BYTE_MASK                    0xff;
.CONST $M.wbs.decoder.WBS_SBC_H2LOBYTE_MASK        0x0f;
.CONST $M.wbs.decoder.WBS_SBC_H2_HEADER_HIBYTE     0x01;
.CONST $M.wbs.decoder.WBS_SBC_H2_HEADER_LOBYTE     0x08;
.CONST $M.wbs.decoder.WBS_SBC_HEADER_BYTE          0xAD;

.CONST $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD_MASK    ($M.wbs.decoder.BYTE_MASK<<8) | $M.wbs.decoder.WBS_SBC_H2LOBYTE_MASK;
.CONST $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD         ($M.wbs.decoder.WBS_SBC_H2_HEADER_HIBYTE<<8) | $M.wbs.decoder.WBS_SBC_H2_HEADER_LOBYTE;
.CONST $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD_MASK    ($M.wbs.decoder.BYTE_MASK<<8);
.CONST $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD         ($M.wbs.decoder.WBS_SBC_HEADER_BYTE<<8);
.CONST $M.wbs.decoder.WBS_SYNC_ODD_HIWORD_MASK     ($M.wbs.decoder.BYTE_MASK);
.CONST $M.wbs.decoder.WBS_SYNC_ODD_HIWORD          ($M.wbs.decoder.WBS_SBC_H2_HEADER_HIBYTE);
.CONST $M.wbs.decoder.WBS_SYNC_ODD_LOWORD_MASK     ($M.wbs.decoder.WBS_SBC_H2LOBYTE_MASK<<8) | $M.wbs.decoder.BYTE_MASK;
.CONST $M.wbs.decoder.WBS_SYNC_ODD_LOWORD          ($M.wbs.decoder.WBS_SBC_H2_HEADER_LOBYTE<<8) | $M.wbs.decoder.WBS_SBC_HEADER_BYTE;


.CONST $M.wbs.decoder.RETCODE_NO_OUTPUT -1;
.CONST $M.wbs.decoder.RETCODE_GENERATE_FAKE_FRAME -2;
.CONST $M.wbs.decoder.VALIDATE_NO_OUTPUT 1;
.CONST $M.wbs.decoder.VALIDATE_NOSYNC 2;

.CONST $M.wbs.decoder.DECODE_SUCCESS 0;
.CONST $M.wbs.decoder.DECODE_BAD_HEADER 1;
.CONST $M.wbs.decoder.DECODE_BAD_CRC 2;

.CONST $M.wbs.decoder.WBS_DECODED_FRAME_SIZE_W       120;
.CONST $M.wbs.decoder.WBS_ENCODED_FRAME_SIZE_B       60;


// *****************************************************************************
// MODULE:
//    $M.wbs.decoder
//
// DESCRIPTION:
//
// FUNCTIONS:
//
// MODIFICATIONS:
//
// NOTES:
// *****************************************************************************
.MODULE $M.wbs.decoder;

   .DATASEGMENT DM;



#ifdef SCOWBSDEBUG
   .VAR lostsynccount = 0;
   .VAR decodefailcount = 0;
#endif

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.initialize:
//
// DESCRIPTION:
//    wbs decoder initialize
//
// INPUTS:
//
//    r9 - data object pointer
// OUTPUTS:
//
// RESERVED REGISTERS
//    r7
//
// TRASHED REGISTERS:
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
// *****************************************************************************

.MODULE $M.sco_decoder.wbs.initialize;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_INIT_PM;

$sco_decoder.wbs.initialize:

#if defined(PATCH_LIBS)
   LIBS_PUSH_REGS_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_4)
#endif


   M[r9 + $sbc.mem.WBS_SYNC_FIELD] = 0;
   M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD] = 0;
   M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD] = 0;
   M[r9 + $sbc.mem.WBS_PAYLOAD_ALIGN_FIELD] = 0;


   rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.validate
//
// DESCRIPTION:
//    Verify sufficient output space for decode
//
// INPUTS:
//    r0 - Output space in samples
//    r5 - Input length in Bytes
//    r6 - Input length in words (rounded up)         UNUSED
//    r7 - Decoder Data Object                        UNUSED
//    r8 - WBS Data Object                            UNUSED
//    I0,L0,B0 - Payload packet as words (litte endian)
//    r9 - data object pointer
//
// OUTPUTS:
//    r1 - Output in samples, 0 to abort
//
// RESERVED REGISTERS
//    r5,r7,r9,I0,L0
//
// TRASHED REGISTERS:
//    all except reserved
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************
.MODULE $M.sco_decoder.wbs.validate;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_VALIDATE_PM;

$sco_decoder.wbs.validate:

   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_0, r6)
#endif


   M1 = 1;
   r6 = r0;

   M[r9 + $sbc.mem.WBS_PAYLOAD_ALIGN_FIELD] = 0; // new packet always has word alignment
   r0 = M[r9 + $sbc.mem.WBS_SYNC_FIELD];

   if NZ jump havesync;
      // prepend the last payload word from previous packet (override status word)
      // this will allow sync-words to cross SCO boundaries
      M2 = -5;
      r0 = M[I0,M2];
      r0 = M[I0,1];
      M2 = 3;
      r1 = M[I0,M2];
      Null = r5 AND 1;
      if Z jump insert_last_word;
         r0 = r0 AND 0xff;
         r0 = r0 LSHIFT 8;
         r1 = r1 LSHIFT -8;
         r1 = r1 AND 0xff;
         r1 = r1 OR r0;
      insert_last_word:
      M[I0,0] = r1;
      r5 = r5 + 2;

      // search for syncword and set payload align flag
      call $sco_decoder.wbs.findsync;
      r5 = r1; // update r5 to reflect how many bytes are available including syncword
      I0 = I1; // update I0 to point at syncword

      M[r9 + $sbc.mem.WBS_PAYLOAD_ALIGN_FIELD] = r4;
      M[r9 + $sbc.mem.WBS_SYNC_FIELD] = r2;

      if NZ jump havesync;
         // not syncd - signal framework to advance input, but produce no output
         r1 = $M.wbs.decoder.VALIDATE_NOSYNC;
         jump $pop_rLink_and_rts;

havesync:
   // find out whether a frame can be produced from the payload plus whatever is in frame buffer
   r2 = $M.wbs.decoder.WBS_DECODED_FRAME_SIZE_W;

   r0 = M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD];

   r0 = r0 + r5;
   r0 = r0 * 2(int); // r0 = output words which can be produced by the decoder
   r1 = $M.wbs.decoder.VALIDATE_NO_OUTPUT;      // default to no output signal
   Null = r0 - r2; // can we produce one frame?
   if POS r1 = r2;

   // make sure there's space to produce the desired output
   Null = r1 - $M.wbs.decoder.VALIDATE_NO_OUTPUT;
   if Z jump $pop_rLink_and_rts;
   Null = r6 - r1;
   if NEG r1 = Null;

   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.findsync
//
// DESCRIPTION:
//    byte-alligned search for WBS_SBC_HEADER pattern
//
// INPUT:
//    r5 = payload length in bytes
//    I0/L0 circular payload buffer
//    M1=1
//
// OUTPUT:
//    r2 : nonzero=found, 0=not found
//    r1 : available bytes in payload including sync
//    I1 : position sync was found, invalid if sync not found
//    r4 : alignment of syncword
//
// TRASHED REGISTERS:
//    r0,r3,r4,r10,L1,loop
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************
.MODULE $M.sco_decoder.wbs.findsync;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_FINDSYNC_PM;

$sco_decoder.wbs.findsync:

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_8, r1)
#endif

   L1 = L0;
   I1 = I0;
#ifdef BASE_REGISTER_MODE
   push B0;
   pop B1;
#endif

   // set loop counter, load first word
   r10 = r5 LSHIFT -1;
   Null = r5 AND 1;
   if Z r10 = r10 - M1, r1 = M[I1,M1];

   M2 = $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD;
   M3 = $M.wbs.decoder.WBS_SYNC_ODD_HIWORD;
   do findsync_loop;
      // init r4 as byte align, [r1,r3] contain header sync words
      r4 = M1, r3 = M[I1,M1];

      // test byte align
      r2 = r1 AND $M.wbs.decoder.WBS_SYNC_ODD_HIWORD_MASK;
      r0 = r3 AND $M.wbs.decoder.WBS_SYNC_ODD_LOWORD_MASK;
      Null = r0 - $M.wbs.decoder.WBS_SYNC_ODD_LOWORD;
      if Z Null = r2 - M3;
      if Z jump foundsync;

      // test word align, if detected r4 is cleared
      r2 = r1 AND $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD_MASK;
      r0 = r3 AND $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD_MASK;
      Null = r0 - $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD;
      if Z r4 = r2 - M2;
      if Z jump foundsync;

      // for next sync high word
      r1 = r3;
   findsync_loop:

   // no sync
   L1 = 0;
   r2 = 0;
   rts;

foundsync:
   // have sync
   // r2 = WBS_SYNC_HIWORD (non-zero)

   // back up two words to wbs header
   r1 = M[I1, -1];
   r1 = M[I1, -1];
   L1 =  0;
#ifdef BASE_REGISTER_MODE
   push Null;
   pop  B1;
#endif

   // recalculate number of bytes, including wbs header sync words
   r10 = r10 + 1;
   r1 = r10 LSHIFT 1;
   r1 = r1 - r4;
   Null = r5 AND 1;
   if NZ r1 = r1 - M1;

   rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.process:
//
// DESCRIPTION:
//    Decoding SCO WBS packets into DAC audio samples.
//
//    The SCO c-buffer contains WBS packet words to be decoded into DAC
//    audio samples. Refer to the function description of Frame_encode for
//    WBS packet word definition.  DAC audio could be mono-channel(left
//    only) or stereo-channels(both left and right).
//
// INPUTS:
//    r1    - Packet timestamp
//    r2    - Packet status
//    r5    - payload size in bytes
//    I0,L0,B0 - Input CBuffer
//    R9    - data object pointer
//
//    non-decoupled variant:
//    r6    - payload size in words (rounded up)      UNUSED
//    r7    - PLC Data Object
//    r8    - WBS Data Object                         UNUSED
//
//    decoupled variant
//    r8    - Output CBuffer
//    r6    - Validate return code
// OUTPUTS:
//    r5    - Output packet status
//    r7    - PLC Data Object
//
// TRASHED REGISTERS:
//    Assumes everything
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************
.MODULE $M.sco_decoder.wbs.process;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_PROC_PM;

$sco_decoder.wbs.process:

   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_1, r0)
#endif

   // save timestamp for the current packet
   M[r9 + $sbc.mem.TIMESTAMP_T2_FIELD] = r1;
   // exit if not syncd
   r0 = M[r9 + $sbc.mem.WBS_SYNC_FIELD];
   // No sync found
   if Z jump no_output;

   // if packet_bfi > stickybfi stickybfi = packet_bfi
   M[$scratch.s0] = r2;
   r0 = M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD];
   Null = r0 - r2;
   if NEG r0 = r2;
   M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD] = r0;


   // sync check: BFI3 = lost sync
   Null = r2 - $M.wbs.decoder.BFI_TIMING;
   if Z jump lostsync;


buffer_decode_loop:
   M1 = 1;
   r2 = $M.wbs.decoder.WBS_ENCODED_FRAME_SIZE_B;

   r0 = M[r9 + $sbc.mem.WBS_FRAME_BUFFER_PTR_FIELD];
   I4 = r0;
   r0 = M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD];
   r4 = M[r9 + $sbc.mem.WBS_PAYLOAD_ALIGN_FIELD];

   // copy bytes from payload to frame
   //    # bytes to copy = min(space_in_frame, data_in_payload)
   //    r0 contains #bytes in framebuffer
   //    r4 contains alignment of payload (0=word, 1=byte)
   //    r5 contains #bytes in payload
   //    I4 contains frame buffer
   //    I0/L0 contains payload buffer
#ifdef WBS_DECOUPLED
   push r6;
   call $sco_decoder.wbs.copy_frame;
   pop r6;
#else
   call $sco_decoder.wbs.copy_frame;
#endif


   M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD] = r0;  // store updated #bytes in frame
   M[r9 + $sbc.mem.WBS_PAYLOAD_ALIGN_FIELD] = r4;


#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_2, r1)
#endif

   // sync check : if sticky_bfi==0 && inframe_b > 4 && bad syncword, syncronization is lost

   r1 = M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD];
   if NZ jump skip_sync_check;
      r1 = M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD];
      Null = r1 - 4;
      if NEG jump skip_sync_check;
         r2 = $M.wbs.decoder.RETCODE_NO_OUTPUT;
         r1 = M[r9 + $sbc.mem.WBS_FRAME_BUFFER_PTR_FIELD];
         r3 = M[r1 + 0];
         r1 = M[r1 + 1];

         r3 = r3 AND $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD_MASK;
         Null = r3 - $M.wbs.decoder.WBS_SYNC_EVEN_HIWORD;
         if NZ jump lostsync;
         r1 = r1 AND $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD_MASK;
         Null = r1 - $M.wbs.decoder.WBS_SYNC_EVEN_LOWORD;
         if NZ jump lostsync;
   skip_sync_check:

   // if a complete frame is in the buffer, decode it... if not, exit
   Null = r0 - $M.wbs.decoder.WBS_ENCODED_FRAME_SIZE_B;
   if NZ jump decode_done;


      M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD] = 0;  // the frame buffer will be emptied by the decode process


      // save environment and decode a frame
      push I0;
      push L0;
#ifdef BASE_REGISTER_MODE
      push B0;
#endif
      push r5;
      push r8;
#ifdef WBS_DECOUPLED
      push r6;
#else
      push r7;
#endif

      // call the decoder

      r0 = M[r9 + $sbc.mem.WBS_FRAME_BUFFER_PTR_FIELD];
      I0 = r0;

      L0 = 0;

#ifdef BASE_REGISTER_MODE
      push Null;
      pop B0;
#endif

#ifdef WBS_DECOUPLED
      r0 = r8;
#else
      r0 = M[r7 + $sco_pkt_handler.OUTPUT_PTR_FIELD];
#endif
      call $sco_decoder.wbs.decode;      // r0 is returncode

#ifdef WBS_DECOUPLED
      pop r6;
#else
      pop r7;
#endif
      pop r8;
      pop r5;
#ifdef BASE_REGISTER_MODE
      pop B0;
#endif
      pop L0;
      pop I0;
      // check for errors
      Null = r0;
      if Z jump buffer_decode_loop;

#if defined(PATCH_LIBS)
   push r1;
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_3, r1)
   pop r1;
#endif

         // decode failed, set sticky_bfi to lost packet
#ifdef SCOWBSDEBUG
   r0 = M[$M.wbs.decoder.decodefailcount];
   r0 = r0 + 1;
   M[$M.wbs.decoder.decodefailcount] = r0;
#endif
         r0 = $M.wbs.decoder.BFI_LOST;

         M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD] = r0;

         jump buffer_decode_loop;   // bad decode set sticky bfi to "LOST"


decode_done:
   // done, set returncode
#ifdef WBS_DECOUPLED
   Null = r6 - $M.wbs.decoder.VALIDATE_NO_OUTPUT;
#else
   r0 = M[r7 + $sco_pkt_handler.PACKET_OUT_LEN_FIELD];
   Null = r0 - $M.wbs.decoder.VALIDATE_NO_OUTPUT;
#endif
   if Z jump no_output;
      // return sticky_bfi, set sticky_bfi to current_bfi

      r5 = M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD];
      r0 = M[$scratch.s0];
      M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD] = r0;

      jump exit_with_output;

no_output:
   r5 = $M.wbs.decoder.RETCODE_NO_OUTPUT;

   // check time stamp delta > 7.5ms
   r2 = M[r9 + $sbc.mem.TIMESTAMP_T2_FIELD];
   r1 = M[r9 + $sbc.mem.TIMESTAMP_T1_FIELD];

   r0 = r2 - r1;
   Null = r0 - $wall_clock.BT_TICKS_IN_7500_US;
   if POS jump output_fake_frame;
   jump exit;
output_fake_frame:
   // unable to decode, but we should generate a fake frame
   r5 = $M.wbs.decoder.RETCODE_GENERATE_FAKE_FRAME;
   jump exit_with_output;


lostsync:
#ifdef SCOWBSDEBUG
   r0 = M[$M.wbs.decoder.lostsynccount];
   r0 = r0 + 1;
   M[$M.wbs.decoder.lostsynccount] = r0;
#endif

   M[r9 + $sbc.mem.WBS_SYNC_FIELD] = 0;
   M[r9 + $sbc.mem.WBS_STICKY_BFI_FIELD] = 0;
   M[r9 + $sbc.mem.WBS_BYTES_IN_FRAME_BUFFER_FIELD] = 0;

   r5 = r2;
   Null = r5 - $M.wbs.decoder.RETCODE_NO_OUTPUT;
   if Z jump no_output;
   jump exit;


exit_with_output:

   // update timestamp
   r1 = M[r9 + $sbc.mem.TIMESTAMP_T2_FIELD];
   M[r9 + $sbc.mem.TIMESTAMP_T1_FIELD] = r1;

   // fallthrough
exit:
   Null = Null - r5;
   if GE jump $pop_rLink_and_rts;

   push r5;
   call $sbcdec.silence_decoder;
   pop r5;
   jump $pop_rLink_and_rts;

.ENDMODULE;



// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.decode
//
// DESCRIPTION:
//    decode a wbs frame
//
// INPUT:
//    r0 - output cbuffer
//    I0/L0 - ptr to frame buffer
//
// OUTPUT:
//    r0 : returncode
//             0 - success
//             1 - bad header
//             2 - failed crc
//
// TRASHED REGISTERS:
//    assume everything
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
//    since the frame buffer is guaranteed to be word-aligned,
//    alot of optimizations could be done (ie no need for putbits/getbits)
// *****************************************************************************
.MODULE $M.sco_decoder.wbs.decode;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_DECODE_PM;

$sco_decoder.wbs.decode:

   $push_rLink_macro;

#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_5, r3)
#endif


   M[$scratch.s2] = r0;

   r0 = M[I0, 1];
   r0 = 16;

   M[r9 + $sbc.mem.GET_BITPOS_FIELD] = r0;


   // crc_checksum = 0x0f
   r0 = 0x0f;

   M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD] = r0;


   // read 4 bytes
   call $sbcdec.get8bits; // header = 0xad

   call $sbcdec.get8bits; // codec = 0x02
   call $sbc.crc_calc;

   call $sbcdec.get8bits; // reserved = 0x00
   call $sbc.crc_calc;

   call $sbcdec.get8bits; // crc

   // store framecrc

   M[r9 + $sbc.mem.FRAMECRC_FIELD] = r1;

   // force 15 blocks and other settings
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





#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_6, r6)
#endif

   // re-calc the frame length
   // MONO and DUAL_CHANNEL:
   //  frame_length = 4 + (4 * nrof_subbands * nrof_channels) / 8 +
   //                 ceil(nrof_blocks * nrof_channels * bitpool / 8)

   //SP.  4 + (4*8*1)/8 + ceil(15*1*26/8) = 8+49
   r0 = 57;

   M[r9 + $sbc.mem.CUR_FRAME_LENGTH_FIELD] = r0;


   // read scalefactors
   call $sbcdec.read_scale_factors;

   // check CRC
   r5 = $M.wbs.decoder.DECODE_BAD_CRC;

   r0 = M[r9 + $sbc.mem.FRAMECRC_FIELD];
   r1 = M[r9 + $sbc.mem.CRC_CHECKSUM_FIELD];
   r0 = r0 - r1;

   r0 = r0 AND 0xff;
   if NZ jump decode_failed;

   // Calculate bit allocation
   call $sbc.calc_bit_allocation;

   // Read audio samples
   call $sbcdec.read_audio_samples;

   // padding

   r0 = M[r9 + $sbc.mem.GET_BITPOS_FIELD];

   Null = r0-16;
   if Z jump skip_padding;
   call $sbcdec.getbits;
skip_padding:


   // Subband sample reconstruction
   call $sbcdec.sample_reconstruction;

   // Synthesis subband filtering

   r0 = M[$scratch.s2];

#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   pop B1;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   I1 = r0;
   L1 = r1;

   // select left channel
   r5 = 0;

   // generate output samples
   call $sbcdec.synthesis_subband_filter;

   L1 = 0;
   r0 = 0;
#ifdef BASE_REGISTER_MODE
   push Null;
   pop B1;
#endif
   jump $pop_rLink_and_rts;

decode_failed:
   r0 = r5;
   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// FUNCTION:
//    $sco_decoder.wbs.copy_frame
//
// DESCRIPTION:
//    byte-aligned copy payload buffer to frame_buffer
//    # bytes copied = min(frame_buf_space, payload_buffer_bytes)
//
// INPUT:
//    r0    : #bytes in frame_buffer
//    r2    : size of frame_buffer in bytes
//    I0/L0 : circular payload buffer (source)
//    r4    : alignment of payload buffer (0=word, 1=byte)
//    I4    : frame buffer (dest)
//    r5    : #bytes available in payload buffer
//    M1    : 1
//
// OUTPUT:
//    r0    : updated #bytes in frame_buffer
//    I0/L0 : updated payload buffer pointer
//    r4    : updated alignment of payload buffer (0=word, 1=byte)
//    r5    : updated #bytes available in payload buffer
//    r2    : # bytes copied
//
// TRASHED REGISTERS:
//    r1,r3,r6,r10,I4, loop
//
// CPU USAGE:
//    D-MEMORY: xxx
//    P-MEMORY: xxx
//    CYCLES:   xxx
//
// NOTES:
// *****************************************************************************
.MODULE $M.sco_decoder.wbs.copy_frame;

   .DATASEGMENT DM;
   .CODESEGMENT SBCDEC_WBS_COPY_FRAME_PM;

$sco_decoder.wbs.copy_frame:

#if defined(PATCH_LIBS)
   LIBS_PUSH_R0_SLOW_SW_ROM_PATCH_POINT($wbs.WBS_SCO_DECODE_ASM.DECODER.SCO_DECODE.PATCH_ID_7, r1)
#endif


   M[$scratch.s1] = r4;


   r2 = r2 - r0; // r2 = space in frame
   Null = r5 - r2;
   if NEG r2 = r5;
   // r2 = #bytes to copy

   r3 = r0 LSHIFT -1;    // I4 = ptr to frame
   I4 = I4 + r3;
   r10 = r2 LSHIFT -1;   // r10 = #words to copy

   Null = r0;
   if Z jump frame_empty;
      // frame is partially full, payload is full and word-aligned
      Null = r0 AND 1;
      if Z jump copy_aligned;

      // copy first byte
      r3 = M[I0, 0], r4 = M[I4, 0];
      r3 = r3 LSHIFT -8;
      r3 = r3 AND 0x00ff;
      r4 = r4 AND 0xff00;
      r4 = r4 OR r3;
      M[I4, 1] = r4;
      r3 = 0;
      jump copy_unaligned;

frame_empty:
   // frame is empty, append payload to start of frame buffer
   Null = r4 AND 1;
   if Z jump copy_aligned;

   r3 = r2 AND 1;
copy_unaligned: // word-unaligned copy
   r10 = r10 + r3, r3 = M[I0, 1];
   r6 = 8;
   r3 = r3 LSHIFT r6, r4 = M[I0, 0];
   do unaligned_copy_loop;
      r4 = r4 LSHIFT -8;
      r1 = r3 OR r4, r3 = M[I0, M1];
      r1 = r1 AND 0xffff;
      r3 = r3 LSHIFT r6, M[I4, 1] = r1, r4 = M[I0, 0];
   unaligned_copy_loop:
   r3 = M[I0, -1]; // over-read
   jump done;

copy_aligned: // word-aligned copy
   r3 = M[I0, 1];
   do copy_loop;
      r3 = M[I0, 1], M[I4, 1] = r3;
   copy_loop:
   r3 = M[I0, -1]; // over-read

done:
   r0 = r0 + r2;
   r5 = r5 - r2;

   r4 = M[$scratch.s1];

   Null = r2 AND 1;
   if NZ r4 = M1 - r4;

   rts;

.ENDMODULE;


