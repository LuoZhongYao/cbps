// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************


#include "mute_control.h"
#include "flash.h"
#ifdef PATCH_LIBS
   #include "patch_library.h"
#endif
#include "cbuffer.h"

.MODULE $M.MUTE_CONTROL;

   .DATASEGMENT DM;

   .VAR     Version = $MUTE_CONTROL_VERSION;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.MUTE_CONTROL.Process.func
//
// DESCRIPTION:
// Switch between the desired gain and zero based on the state variable
//
//  INPUT:
//      r7 - Pointer to the MUTE_CONTROL data buffer.
//  OUTPUT:
//    none
//
// TRASHED REGISTERS:
//      r0, r1,M1,I0,L0,I2,M0,M1
//
// *****************************************************************************
.MODULE $M.MUTE_CONTROL.Process;

 .CODESEGMENT AUDIO_PROC_MUTE_CONTROL_PROCESS_PM;
func:
#if defined(PATCH_LIBS)
   LIBS_SLOW_SW_ROM_PATCH_POINT($audio_proc.MUTE_CONTROL_ASM.PROCESS.PATCH_ID_0,r1) // af05_CVC_patches1
#endif
   push rLink;      
   // Get Input Buffer
   r0  = M[r7 + $M.MUTE_CONTROL.OFFSET_INPUT_PTR];
#ifdef BASE_REGISTER_MODE  
   call $frmbuffer.get_buffer_with_start_address;
   push r2;
   pop  B0;
#else
   call $frmbuffer.get_buffer;
#endif
   pop rLink;
   
   // Use input frame size
   r2 = M[r7 + $M.MUTE_CONTROL.OFFSET_PTR_STATE];
   r4 = M[r7 + $M.MUTE_CONTROL.OFFSET_MUTE_VAL];
   r2 = M[r2];
   r4 = r4 - r2;
   if NZ jump loop_mute;
   r10 = r3;
   I0  = r0;
   L0  = r1;

   // zero buffer
   do loop_mute;
      M[I0,1] = r4;
loop_mute:
   // clear L0 and exit
   L0 = 0;
#ifdef BASE_REGISTER_MODE  
   push Null;
   pop  B0;
#endif   
   rts;

.ENDMODULE;
