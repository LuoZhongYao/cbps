// *****************************************************************************
// Copyright (c) 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
// *****************************************************************************


// KAL_ARCH2 version of filter coefficient calculation library

// two nops after stack manipulation so macro can safetly be used in a loop
#define MACRO_SP_PLUS(x) r9 = M[$STACK_POINTER]; r9 = r9 + (x); M[$STACK_POINTER] = r9; null = null + null; null = null + null;
#define MACRO_SP_MINUS(x) r9 = M[$STACK_POINTER]; r9 = r9 - (x); M[$STACK_POINTER] = r9; null = null + null; null = null + null;
#define MACRO_FP_EQU_PUSH_SP r9 = M[$STACK_POINTER]; push r8; r8 = r9;

//------------------------------------------------------------------------------
__warp:
//------------------------------------------------------------------------------
// warp filter frequency
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = filter frequency (Hz)
//   r2:r3 = sample rate (Hz)
// on exit
//   r0:r1 = warped digital frequency
// registers registers destroyed
//   none
// stack usage
//   jump = 28
//   call = 29
//------------------------------------------------------------------------------
// wd = tan(PI*fc/Fs);
//------------------------------------------------------------------------------
// tangent function uses a truncated power series.  Constants of the series are
//    0.186 {e=0x00007d m=0xbe76c9 }
//   -0.221 {e=0x80007d m=0xe24dd3 }
//   -0.168 {e=0x80007d m=0xac0831 }
//    0.524 {e=0x00007f m=0x8624dd }
//    0.230 {e=0x00007d m=0xeb851f }
//    1.000 {e=0x000080 m=0x800000 }
//------------------------------------------------------------------------------

.var/dm __warp.constants[14] = 
                        0x000081, 0xc90fdb,         // PI
                        0x00007d, 0xbe76c9,         //  0.186 
                        0x80007d, 0xe24dd3,         // -0.221 
                        0x80007d, 0xac0831,         // -0.168 
                        0x00007f, 0x8624dd,         //  0.524 
                        0x00007d, 0xeb851f,         //  0.230 
                        0x000080, 0x800000;         //  1.000 

__warp.call_entry:
    push rLink;    
__warp.jump_entry:

    // pushm <r2,r3,r4,r5,r6,r7,r10>;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push r10;
    push i0;
    i0 = &__warp.constants;

    call $kal_float_lib.div;            // r0:r1 = fc/Fs
    r2 = m[i0,1];
    r3 = m[i0,1];
    call $kal_float_lib.mul;            // r0:r1 = w = PI*fc/Fs
    r2 = r0;
    r3 = r1;
    r4 = r0;
    r5 = r1;
    call $kal_float_lib.mul;            // r0:r1 = w^2
    r6 = r0;
    r7 = r1;
    
    // r = k[0];
    r0 = m[i0,1];
    r1 = m[i0,1];

    r10 = 5;
    do __warp.tangent_loop;
        // r = r * w^2
        r2 = r6;
        r3 = r7;
        call $kal_float_lib.mul;            // r0:r1 = r * w^2
        r2 = m[i0,1];
        r3 = m[i0,1];
        call $kal_float_lib.add;            // r0:r1 = k[1] + r * w^2
    __warp.tangent_loop:
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.mul;

    pop i0;
    // popm <r2,r3,r4,r5,r6,r7,r10>;
    pop r10;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    
    pop rLink;
    rts;



//------------------------------------------------------------------------------
__db2lin:
//------------------------------------------------------------------------------
// convert dB value to a linear value (y = 10^(x/20))
//------------------------------------------------------------------------------
// on entry
//   r0:r1 = db value
// on exit
//   r0:r1 = linear value
// registers destroyed
//   none
// stack usage
//   jump = 27
//   call = 28
//------------------------------------------------------------------------------
// conversion uses a truncated power series.  Constants of the series are
//   6.02059991 {e=0x000082 m=0xc0a8c1 }
//   0.00624400 {e=0x000078 m=0xcc9a78 }
//   0.00035858 {e=0x000074 m=0xbbffcb }
//   0.11550600 {e=0x00007c m=0xec8e69 }
//   1.00000000 {e=0x000080 m=0x800000 }
//------------------------------------------------------------------------------

.var/dm __db2lin.constants[10] = 
                        0x000082, 0xc0a8c1,         // 6.02059991
                        0x000074, 0xbbffcb,         // 0.00035858
                        0x000078, 0xcc9a78,         // 0.00624400
                        0x00007c, 0xec8e69,         // 0.11550600
                        0x000080, 0x800000;         // 1.00000000

__db2lin.call_entry:
    push rLink;    
__db2lin.jump_entry:

    // pushm <r2,r3,r4,r5,r6,r10>;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r10;
    push i0;
    i0 = &__db2lin.constants;

    r4 = r0;
    r5 = r1;
    
    r2 = m[i0,1];
    r3 = m[i0,-1];
    call $kal_float_lib.div;
    r2 = r0;
    r3 = r1;
    call $kal_float_lib.float_to_int;
    r6 = r0;                        // r6 = number of bits to shift at end
    call $kal_float_lib.int_to_float;
    r2 = m[i0,1];
    r3 = m[i0,1];
    call $kal_float_lib.mul;
    r2 = r0;
    r3 = r1;
    r0 = r4;
    r1 = r5;
    call $kal_float_lib.sub;
    r4 = r0;                        // r4:r5 = frac part to use in power series
    r5 = r1;

    // r = k[0];
    r0 = m[i0,1];
    r1 = m[i0,1];

    r10 = 3;
    do __db2lin.power_series_loop;
        r2 = r4;
        r3 = r5;
        call $kal_float_lib.mul;            // r0:r1 = r * frac
        r2 = m[i0,1];
        r3 = m[i0,1];
        call $kal_float_lib.add;            // r0:r1 = k[i] + r * w^2
    __db2lin.power_series_loop:
    
    r0 = r0 + r6;
    
    pop i0;
    // popm <r2,r3,r4,r5,r6,r10>;
    pop r10;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;

    pop rLink;
    rts;
    


//------------------------------------------------------------------------------
__scale_coefficients:
//------------------------------------------------------------------------------
// take coefficents in floating point format and convert them into fixed point
// for use in biquads
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const __scale_coefficients.entry_b0.e       10;
.const __scale_coefficients.entry_b0.m        9;
.const __scale_coefficients.entry_b1.e        8;
.const __scale_coefficients.entry_b1.m        7;
.const __scale_coefficients.entry_b2.e        6;
.const __scale_coefficients.entry_b2.m        5;
.const __scale_coefficients.entry_a1.e        4;
.const __scale_coefficients.entry_a1.m        3;
.const __scale_coefficients.entry_a2.e        2;
.const __scale_coefficients.entry_a2.m        1;
.const __scale_coefficients.entry_stack_size 10;
//------------------------------------------------------------------------------
// on exit - all return values on stack
.const __scale_coefficients.exit_b0           6;
.const __scale_coefficients.exit_b1           5;
.const __scale_coefficients.exit_b2           4;
.const __scale_coefficients.exit_a1           3;
.const __scale_coefficients.exit_a2           2;
.const __scale_coefficients.exit_scale        1;
.const __scale_coefficients.exit_stack_size   6;
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  = 10
//   temporary storage =  7
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),rlink>;
    MACRO_FP_EQU_PUSH_SP
    push rLink;

    // find maximum coefficient in terms of exponent (the bit that matters)
    r0 = $kal_float_lib.format.exp_mask;
    r1 = m[r8 - __scale_coefficients.entry_b0.e];      // b0.e
    r1 = r1 and r0;
    r2 = m[r8 - __scale_coefficients.entry_b1.e];      // b1.e
    r2 = r2 and r0;
    // r1 = max r2;
    null = r2 - r1;
    if pos r1 = r2;
    r2 = m[r8 - __scale_coefficients.entry_b2.e];      // b2.e
    r2 = r2 and r0;
    // r1 = max r2;
    null = r2 - r1;
    if pos r1 = r2;
    r2 = m[r8 - __scale_coefficients.entry_a1.e];      // a1.e
    r2 = r2 and r0;
    // r1 = max r2;
    null = r2 - r1;
    if pos r1 = r2;
    r2 = m[r8 - __scale_coefficients.entry_a2.e];      // a2.e
    r2 = r2 and r0;
    // r1 = max r2;
    null = r2 - r1;
    if pos r1 = r2;
    
    // r1 contains largest exponent
    r2 = r1 - $kal_float_lib.format.exp_offset;
    
    // b0
    r0 = m[r8 - __scale_coefficients.entry_b0.e];       // exp
    r1 = m[r8 - __scale_coefficients.entry_b0.m];       // mant
    r0 = r0 - r2;
    call $kal_float_lib.float_to_frac;
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_b0)] = r0;

    // b1
    r0 = m[r8 - __scale_coefficients.entry_b1.e];       // exp
    r1 = m[r8 - __scale_coefficients.entry_b1.m];       // mant
    r0 = r0 - r2;
    call $kal_float_lib.float_to_frac;
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_b1)] = r0;

    // b2
    r0 = m[r8 - __scale_coefficients.entry_b2.e];       // exp
    r1 = m[r8 - __scale_coefficients.entry_b2.m];       // mant
    r0 = r0 - r2;
    call $kal_float_lib.float_to_frac;
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_b2)] = r0;

    // a1
    r0 = m[r8 - __scale_coefficients.entry_a1.e];       // exp
    r1 = m[r8 - __scale_coefficients.entry_a1.m];       // mant
    r0 = r0 - r2;
    call $kal_float_lib.float_to_frac;
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_a1)] = r0;

    // a2
    r0 = m[r8 - __scale_coefficients.entry_a2.e];       // exp
    r1 = m[r8 - __scale_coefficients.entry_a2.m];       // mant
    r0 = r0 - r2;
    call $kal_float_lib.float_to_frac;
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_a2)] = r0;

    // scale
    m[r8 - (__scale_coefficients.entry_stack_size+__scale_coefficients.exit_scale)] = r2;

    // popm <r8,rlink>;
    pop rLink;
    pop r8;

//    sp = sp - __scale_coefficients.entry_stack_size;
    MACRO_SP_MINUS(__scale_coefficients.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_bypass:
//------------------------------------------------------------------------------
// return coefficients for bypass filter
//------------------------------------------------------------------------------
// on entry - no entry parameters
.const calc_bypass.entry_stack_size  0;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  0
//   temporary storage =  1
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push rLink;

    r0 = 0x000001;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    r0 = 0x000000;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    r0 = 0x400000;
    m[r8 - (calc_bypass.entry_stack_size+biquad_coeficients.exit_b0)] = r0;

    // popm <fp,r0,rlink>;
    pop rlink;
    pop r0;
    pop r8;

    rts;



//------------------------------------------------------------------------------
calc_low_pass_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order low pass filter
//
//            w
//   H(s) = -----
//          s + w
//
//------------------------------------------------------------------------------
//
//          w/(w+1) + (w/(w+1))z^-1
//   H(z) = ------------------------
//          1  +  ((w-1)/(w+1))z^-1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_low_pass_1st.entry_fc.e        4;
.const calc_low_pass_1st.entry_fc.m        3;
.const calc_low_pass_1st.entry_Fs.e        2;
.const calc_low_pass_1st.entry_Fs.m        1;
.const calc_low_pass_1st.entry_stack_size  4;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  4
//   temporary storage = 39
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;      // 10 stack locations
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    r0 = M[r8 - calc_low_pass_1st.entry_fc.e];
    r1 = M[r8 - calc_low_pass_1st.entry_fc.m];
    r2 = M[r8 - calc_low_pass_1st.entry_Fs.e];
    r3 = M[r8 - calc_low_pass_1st.entry_Fs.m];

    call __warp.call_entry;
    r4 = r0;
    r5 = r1;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    call $kal_float_lib.recip;
    r6 = r0;
    r7 = r1;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.mul;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // pushm <r0,r1>;                   // b0
    push r0;
    push r1;
    // pushm <r0,r1>;                   // b1
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // b2
    push r0;
    push r1;
    r0 = r4;
    r1 = r5;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.sub;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // a1
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // a2
    push r0;
    push r1;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;
    
    // sp = sp - calc_low_pass_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_low_pass_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_high_pass_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order high pass filter
//
//            s
//   H(s) = -----
//          s + w
//
//------------------------------------------------------------------------------
//
//          1/(w+1) + (-1/(w+1))z^-1
//   H(z) = ------------------------
//          1  +   ((w-1)/(w+1))z^-1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_high_pass_1st.entry_fc.e        4;
.const calc_high_pass_1st.entry_fc.m        3;
.const calc_high_pass_1st.entry_Fs.e        2;
.const calc_high_pass_1st.entry_Fs.m        1;
.const calc_high_pass_1st.entry_stack_size  4;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  4
//   temporary storage = 39
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    r0 = M[r8 - calc_high_pass_1st.entry_fc.e];
    r1 = M[r8 - calc_high_pass_1st.entry_fc.m];
    r2 = M[r8 - calc_high_pass_1st.entry_Fs.e];
    r3 = M[r8 - calc_high_pass_1st.entry_Fs.m];

    call __warp.call_entry;
    r4 = r0;
    r5 = r1;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    call $kal_float_lib.recip;
    r6 = r0;
    r7 = r1;
    
    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // pushm <r0,r1>;                   // b0
    push r0;
    push r1;
    r0 = r0 xor $kal_float_lib.format.sign_bit;
    // pushm <r0,r1>;                   // b1
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // b2
    push r0;
    push r1;
    r0 = r4;
    r1 = r5;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.sub;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // a1
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    //pushm <r0,r1>;                   // a2
    push r0;
    push r1;

    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_high_pass_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_high_pass_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_all_pass_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order all pass filter
//
//          s - w
//   H(s) = -----
//          s + w
//
//------------------------------------------------------------------------------
//
//          (1-w)/(w+1) + -1.z^-1
//   H(z) = ---------------------
//          1 + ((w-1)/(w+1))z^-1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_all_pass_1st.entry_fc.e        4;
.const calc_all_pass_1st.entry_fc.m        3;
.const calc_all_pass_1st.entry_Fs.e        2;
.const calc_all_pass_1st.entry_Fs.m        1;
.const calc_all_pass_1st.entry_stack_size  4;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  4
//   temporary storage = 39
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    r0 = M[r8 - calc_all_pass_1st.entry_fc.e];
    r1 = M[r8 - calc_all_pass_1st.entry_fc.m];
    r2 = M[r8 - calc_all_pass_1st.entry_Fs.e];
    r3 = M[r8 - calc_all_pass_1st.entry_Fs.m];

    call __warp.call_entry;
    r4 = r0;
    r5 = r1;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    call $kal_float_lib.recip;
    r6 = r0;
    r7 = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = (1-w) / denom
    r0 = $kal_float_lib.const.unity.exp;
    r1 = $kal_float_lib.const.unity.mant;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.sub;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b0
    push r0;
    push r1;
    r2 = r0;
    r3 = r1;

    // b1 = -1
    r0 = $kal_float_lib.const.unity.exp;
    r1 = $kal_float_lib.const.unity.mant;
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    // pushm <r0,r1>;                   // b1
    push r0;
    push r1;
    
    // b2 = 0
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // b2
    push r0;
    push r1;
    
    // a1 = (w-1) / denom  = -b0
    r0 = r2;
    r1 = r3;
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    // pushm <r0,r1>;                   // a1
    push r0;
    push r1;
    
    // a2 = 0
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // a2
    push r0;
    push r1;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_all_pass_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_all_pass_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_shelf_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order shelf filter with prescribed nyquist
// frequency gain (gh)
//
//   lg = 10^(gh/20)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//               s + w1
//   H(s) = lg * ------
//               s + w2
//
//------------------------------------------------------------------------------
//
//   lg = 10^(gh/20)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//          lg*(w1+1)/(w2+1) + lg*((w1-1)/(w2+1))z^-1
//   H(z) = -----------------------------------------
//                   1       +   ((w2-1)/(w2+1))z^-1
//
//------------------------------------------------------------------------------
// lg = Math.Pow(10, (gh / 20));
// wd = Math.Tan(Math.PI * fc / m_fs);
// wd1 = wd * Math.Pow(10, (g / 40));
// wd2 = wd / Math.Pow(10, (g / 40));
//
// denom = wd2 + 1.0;
//
// b[0] = lg*(wd1 + 1.0) / denom;
// b[1] = lg*(wd1 - 1.0) / denom;
// b[2] = 0.0;
// a[1] = (wd2 - 1.0) / denom;
// a[2] = 0.0;
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_shelf_1st.entry_fc.e        8;
.const calc_shelf_1st.entry_fc.m        7;
.const calc_shelf_1st.entry_g.e         6;
.const calc_shelf_1st.entry_g.m         5;
.const calc_shelf_1st.entry_gh.e        4;
.const calc_shelf_1st.entry_gh.m        3;
.const calc_shelf_1st.entry_Fs.e        2;
.const calc_shelf_1st.entry_Fs.m        1;
.const calc_shelf_1st.entry_stack_size  8;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_shelf_1st.stack_temp        10;
.const calc_shelf_1st.local_1           calc_shelf_1st.stack_temp + 0;
.const calc_shelf_1st.local_2           calc_shelf_1st.stack_temp + 1;
.const calc_shelf_1st.local_3           calc_shelf_1st.stack_temp + 2;
.const calc_shelf_1st.local_4           calc_shelf_1st.stack_temp + 3;
.const calc_shelf_1st.local_stack_size  4;
//------------------------------------------------------------------------------
// on exit - all return values on stack
.const calc_shelf_1st.exit_b0           6;
.const calc_shelf_1st.exit_b1           5;
.const calc_shelf_1st.exit_b2           4;
.const calc_shelf_1st.exit_a1           3;
.const calc_shelf_1st.exit_a2           2;
.const calc_shelf_1st.exit_scale        1;
.const calc_shelf_1st.exit_stack_size   6;
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  8
//   temporary storage = 43
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_shelf_1st.local_stack_size;
    MACRO_SP_PLUS(calc_shelf_1st.local_stack_size)

    r0 = M[r8 - calc_shelf_1st.entry_fc.e];
    r1 = M[r8 - calc_shelf_1st.entry_fc.m];
    r2 = M[r8 - calc_shelf_1st.entry_Fs.e];
    r3 = M[r8 - calc_shelf_1st.entry_Fs.m];
    call __warp.call_entry;
    r4 = r0;
    r5 = r1;

    r0 = M[r8 - calc_shelf_1st.entry_gh.e];
    r1 = M[r8 - calc_shelf_1st.entry_gh.m];
    call __db2lin.call_entry;
    m[r8 + calc_shelf_1st.local_3] = r0;
    m[r8 + calc_shelf_1st.local_4] = r1;
    
    r0 = M[r8 - calc_shelf_1st.entry_g.e];
    r1 = M[r8 - calc_shelf_1st.entry_g.m];
    r0 = r0 - 1;
    call __db2lin.call_entry;
    r2 = r0;
    r3 = r1;
    
    r0 = r4;
    r1 = r5;
    call $kal_float_lib.div;
    r6 = r0;                        // r6:r7 = w2
    r7 = r1;
    
    r0 = r4;
    r1 = r5;
    call $kal_float_lib.mul;
    r4 = r0;                        // r4:r5 = w1
    r5 = r1;

    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    call $kal_float_lib.recip;
    m[r8 + calc_shelf_1st.local_1] = r0;         // denom = 1/(w2+1)
    m[r8 + calc_shelf_1st.local_2] = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = lg*(wd1 + 1.0) / denom
    r0 = r4;
    r1 = r5;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    r2 = m[r8 + calc_shelf_1st.local_1];
    r3 = m[r8 + calc_shelf_1st.local_2];
    call $kal_float_lib.mul;
    r2 = m[r8 + calc_shelf_1st.local_3];
    r3 = m[r8 + calc_shelf_1st.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b0
    push r0;
    push r1;
    
    // b1 = lg*(wd1 - 1.0) / denom
    r0 = r4;
    r1 = r5;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.sub;
    r2 = m[r8 + calc_shelf_1st.local_1];
    r3 = m[r8 + calc_shelf_1st.local_2];
    call $kal_float_lib.mul;
    r2 = m[r8 + calc_shelf_1st.local_3];
    r3 = m[r8 + calc_shelf_1st.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b1
    push r0;
    push r1;
    
    // b2 = 0.0
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // b2
    push r0;
    push r1;

    // a1 = (wd2 - 1.0) / denom
    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.sub;
    r2 = m[r8 + calc_shelf_1st.local_1];
    r3 = m[r8 + calc_shelf_1st.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // a1
    push r0;
    push r1;

    // a2 = 0.0
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                   // a2
    push r0;
    push r1;

    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_shelf_1st.entry_stack_size+calc_shelf_1st.exit_b0)] = r0;
    
    // sp = sp - calc_shelf_1st.local_stack_size;
    MACRO_SP_MINUS(calc_shelf_1st.local_stack_size)
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_shelf_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_shelf_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_low_shelf_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order low frequency shelf filter
//
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//          s + w1
//   H(s) = ------
//          s + w2
//
//------------------------------------------------------------------------------
//
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//          (w1+1)/(w2+1) + ((w1-1)/(w2+1))z^-1
//   H(z) = -----------------------------------
//                1       + ((w2-1)/(w2+1))z^-1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_1st' to calculate coefficients
// - return (calc_shelf_1st(fc,g,0,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_low_shelf_1st.entry_fc.e        6;
.const calc_low_shelf_1st.entry_fc.m        5;
.const calc_low_shelf_1st.entry_g.e         4;
.const calc_low_shelf_1st.entry_g.m         3;
.const calc_low_shelf_1st.entry_Fs.e        2;
.const calc_low_shelf_1st.entry_Fs.m        1;
.const calc_low_shelf_1st.entry_stack_size  6;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 61
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_1st"
    // sp = sp + calc_shelf_1st.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_1st.exit_stack_size)
    
    r0 = M[r8 - calc_low_shelf_1st.entry_fc.e];
    r1 = M[r8 - calc_low_shelf_1st.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_low_shelf_1st.entry_g.e];
    r1 = M[r8 - calc_low_shelf_1st.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                              // gh (0dB)
    push r0;
    push r1;
    r0 = M[r8 - calc_low_shelf_1st.entry_Fs.e];
    r1 = M[r8 - calc_low_shelf_1st.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_1st;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_low_shelf_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_low_shelf_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_high_shelf_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order high frequency shelf filter
//
//   lg = 10^(g/20)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//               s + w1
//   H(s) = lg * ------
//               s + w2
//
//------------------------------------------------------------------------------
//
//   lg = 10^(g/20)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//          lg*(w1+1)/(w2+1) + lg*((w1-1)/(w2+1))z^-1
//   H(z) = -----------------------------------------
//                   1       +   ((w2-1)/(w2+1))z^-1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_1st' to calculate coefficients
// - return (calc_shelf_1st(fc,g,g,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_high_shelf_1st.entry_fc.e        6;
.const calc_high_shelf_1st.entry_fc.m        5;
.const calc_high_shelf_1st.entry_g.e         4;
.const calc_high_shelf_1st.entry_g.m         3;
.const calc_high_shelf_1st.entry_Fs.e        2;
.const calc_high_shelf_1st.entry_Fs.m        1;
.const calc_high_shelf_1st.entry_stack_size  6;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 61
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_1st"
    // sp = sp + calc_shelf_1st.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_1st.exit_stack_size)
    
    r0 = M[r8 - calc_high_shelf_1st.entry_fc.e];
    r1 = M[r8 - calc_high_shelf_1st.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_1st.entry_g.e];
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r1 = M[r8 - calc_high_shelf_1st.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    // pushm <r0,r1>;                              // gh
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_1st.entry_Fs.e];
    r1 = M[r8 - calc_high_shelf_1st.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_1st;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_high_shelf_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_high_shelf_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_tilt_1st:
//------------------------------------------------------------------------------
// calculate coefficients for first order tilt filter
//
//   lg = 10^(g/40)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//               s + w1
//   H(s) = lg * ------
//               s + w2
//
//------------------------------------------------------------------------------
//
//   lg = 10^(g/40)
//   w1 = w/10^(g/40)
//   w2 = w*10^(g/40)
//
//          lg*(w1+1)/(w2+1) + lg*((w1-1)/(w2+1))z^-1
//   H(z) = -----------------------------------------
//                   1       +   ((w2-1)/(w2+1))z^-1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_1st' to calculate coefficients
// - return (calc_shelf_1st(fc,g,g/2,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_tilt_1st.entry_fc.e        6;
.const calc_tilt_1st.entry_fc.m        5;
.const calc_tilt_1st.entry_g.e         4;
.const calc_tilt_1st.entry_g.m         3;
.const calc_tilt_1st.entry_Fs.e        2;
.const calc_tilt_1st.entry_Fs.m        1;
.const calc_tilt_1st.entry_stack_size  6;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 61
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_1st"
    // sp = sp + calc_shelf_1st.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_1st.exit_stack_size)
    
    r0 = M[r8 - calc_tilt_1st.entry_fc.e];
    r1 = M[r8 - calc_tilt_1st.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_1st.entry_g.e];
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r1 = M[r8 - calc_tilt_1st.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r0 = r0 - 1;
    // pushm <r0,r1>;                              // gh
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_1st.entry_Fs.e];
    r1 = M[r8 - calc_tilt_1st.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_1st;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_tilt_1st.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_tilt_1st.entry_stack_size;
    MACRO_SP_MINUS(calc_tilt_1st.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_low_pass_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order low pass filter
//
//                  w^2
//   H(s) = ------------------
//          s^2 + (w/q)s + w^2
//
//------------------------------------------------------------------------------
//
//               w^2            2.w^2                 w^2
//          ------------- + ------------- z^-1 + ------------- z^-2
//          w^2 + w/q + 1   w^2 + w/q + 1        w^2 + w/q + 1
//   H(z) = -------------------------------------------------------
//                            2.w^2 - 2          w^2 - w/q + 1
//                1       + ------------- z^-1 + ------------- z^-2
//                          w^2 + w/q + 1        w^2 + w/q + 1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_low_pass_2nd.entry_fc.e        6;
.const calc_low_pass_2nd.entry_fc.m        5;
.const calc_low_pass_2nd.entry_q.e         4;
.const calc_low_pass_2nd.entry_q.m         3;
.const calc_low_pass_2nd.entry_Fs.e        2;
.const calc_low_pass_2nd.entry_Fs.m        1;
.const calc_low_pass_2nd.entry_stack_size  6;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_low_pass_2nd.stack_temp        10;
.const calc_low_pass_2nd.local_1           calc_low_pass_2nd.stack_temp + 0;
.const calc_low_pass_2nd.local_2           calc_low_pass_2nd.stack_temp + 1;
.const calc_low_pass_2nd.local_stack_size  2;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 41
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_low_pass_2nd.local_stack_size;
    MACRO_SP_PLUS(calc_low_pass_2nd.local_stack_size)
    
    r0 = M[r8 - calc_low_pass_2nd.entry_fc.e];
    r1 = M[r8 - calc_low_pass_2nd.entry_fc.m];
    r2 = M[r8 - calc_low_pass_2nd.entry_Fs.e];
    r3 = M[r8 - calc_low_pass_2nd.entry_Fs.m];

    call __warp.call_entry;
    r2 = r0;
    r3 = r1;
    call $kal_float_lib.mul;
    r6 = r0;						// r6:r7 = w^2
    r7 = r1;
    r0 = r2;
    r1 = r3;
    r2 = M[r8 - calc_low_pass_2nd.entry_q.e];
    r3 = M[r8 - calc_low_pass_2nd.entry_q.m];
    call $kal_float_lib.div;        // w/q
    r4 = r0;						// r4:r5 = w/q
    r5 = r1;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.add;        // w^2 + w/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w^2 + w/q + 1
    call $kal_float_lib.recip;      // denom
    m[r8 + calc_low_pass_2nd.local_1] = r0;         // denom = 1/(w^2+w/q+1)
    m[r8 + calc_low_pass_2nd.local_2] = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = w^2 / denom
    r0 = r6;
    r1 = r7;
    r2 = m[r8 + calc_low_pass_2nd.local_1];
    r3 = m[r8 + calc_low_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    // b1 = 2.w^2 / denom
    r0 = r0 + 1;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // b2 = w^2 / denom
    r0 = r0 - 1;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // a1 = (2.w^2 - 2) / denom
    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w^2
    call $kal_float_lib.sub;        // 2.w^2 - 2
    r2 = m[r8 + calc_low_pass_2nd.local_1];
    r3 = m[r8 + calc_low_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    // a2 = (w^2 - w/q + 1) / denom
    r0 = r6;
    r1 = r7;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.sub;    
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    r2 = m[r8 + calc_low_pass_2nd.local_1];
    r3 = m[r8 + calc_low_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_low_pass_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // sp = sp - calc_low_pass_2nd.local_stack_size;
    MACRO_SP_MINUS(calc_low_pass_2nd.local_stack_size)
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;
    
    // sp = sp - calc_low_pass_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_low_pass_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_high_pass_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order high pass filter
//
//                  s^2
//   H(s) = ------------------
//          s^2 + (w/q)s + w^2
//
//------------------------------------------------------------------------------
//
//                1               -2                   1
//          ------------- + ------------- z^-1 + ------------- z^-2
//          w^2 + w/q + 1   w^2 + w/q + 1        w^2 + w/q + 1
//   H(z) = -------------------------------------------------------
//                            2.w^2 - 2          w^2 - w/q + 1
//                1       + ------------- z^-1 + ------------- z^-2
//                          w^2 + w/q + 1        w^2 + w/q + 1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_high_pass_2nd.entry_fc.e        6;
.const calc_high_pass_2nd.entry_fc.m        5;
.const calc_high_pass_2nd.entry_q.e         4;
.const calc_high_pass_2nd.entry_q.m         3;
.const calc_high_pass_2nd.entry_Fs.e        2;
.const calc_high_pass_2nd.entry_Fs.m        1;
.const calc_high_pass_2nd.entry_stack_size  6;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_high_pass_2nd.stack_temp        10;
.const calc_high_pass_2nd.local_1           calc_high_pass_2nd.stack_temp + 0;
.const calc_high_pass_2nd.local_2           calc_high_pass_2nd.stack_temp + 1;
.const calc_high_pass_2nd.local_stack_size  2;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 41
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_high_pass_2nd.local_stack_size;
    MACRO_SP_PLUS(calc_high_pass_2nd.local_stack_size)
    
    r0 = M[r8 - calc_high_pass_2nd.entry_fc.e];
    r1 = M[r8 - calc_high_pass_2nd.entry_fc.m];
    r2 = M[r8 - calc_high_pass_2nd.entry_Fs.e];
    r3 = M[r8 - calc_high_pass_2nd.entry_Fs.m];

    call __warp.call_entry;
    r2 = r0;
    r3 = r1;
    call $kal_float_lib.mul;
    r6 = r0;						// r6:r7 = w^2
    r7 = r1;
    r0 = r2;
    r1 = r3;
    r2 = M[r8 - calc_high_pass_2nd.entry_q.e];
    r3 = M[r8 - calc_high_pass_2nd.entry_q.m];
    call $kal_float_lib.div;        // w/q
    r4 = r0;						// r4:r5 = w/q
    r5 = r1;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.add;        // w^2 + w/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w^2 + w/q + 1
    call $kal_float_lib.recip;      // denom
    m[r8 + calc_high_pass_2nd.local_1] = r0;         // denom = 1/(w^2+w/q+1)
    m[r8 + calc_high_pass_2nd.local_2] = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = 1 / denom
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    // b1 = -2 / denom
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r0 = r0 + 1;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // b2 = 1 / denom
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r0 = r0 - 1;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // a1 = (2.w^2 - 2) / denom
    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w^2
    call $kal_float_lib.sub;        // 2.w^2 - 2
    r2 = m[r8 + calc_high_pass_2nd.local_1];
    r3 = m[r8 + calc_high_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    // a2 = (w^2 - w/q + 1) / denom
    r0 = r6;
    r1 = r7;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.sub;    
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    r2 = m[r8 + calc_high_pass_2nd.local_1];
    r3 = m[r8 + calc_high_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_high_pass_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // sp = sp - calc_high_pass_2nd.local_stack_size;
    MACRO_SP_MINUS(calc_high_pass_2nd.local_stack_size)
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_high_pass_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_high_pass_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_all_pass_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order all pass filter
//
//          s^2 - (w/q)s + w^2
//   H(s) = ------------------
//          s^2 + (w/q)s + w^2
//
//------------------------------------------------------------------------------
//
//          w^2 - w/q + 1     2.w^2 - 2
//          ------------- + ------------- z^-1 +        z^-2
//          w^2 + w/q + 1   w^2 + w/q + 1
//   H(z) = -------------------------------------------------------
//                            2.w^2 - 2          w^2 - w/q + 1
//                1       + ------------- z^-1 + ------------- z^-2
//                          w^2 + w/q + 1        w^2 + w/q + 1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_all_pass_2nd.entry_fc.e        6;
.const calc_all_pass_2nd.entry_fc.m        5;
.const calc_all_pass_2nd.entry_q.e         4;
.const calc_all_pass_2nd.entry_q.m         3;
.const calc_all_pass_2nd.entry_Fs.e        2;
.const calc_all_pass_2nd.entry_Fs.m        1;
.const calc_all_pass_2nd.entry_stack_size  6;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_all_pass_2nd.stack_temp        10;
.const calc_all_pass_2nd.local_1           calc_all_pass_2nd.stack_temp + 0;
.const calc_all_pass_2nd.local_2           calc_all_pass_2nd.stack_temp + 1;
.const calc_all_pass_2nd.local_stack_size  2;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  6
//   temporary storage = 41
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_all_pass_2nd.local_stack_size;
    MACRO_SP_PLUS(calc_all_pass_2nd.local_stack_size)
    
    r0 = M[r8 - calc_all_pass_2nd.entry_fc.e];
    r1 = M[r8 - calc_all_pass_2nd.entry_fc.m];
    r2 = M[r8 - calc_all_pass_2nd.entry_Fs.e];
    r3 = M[r8 - calc_all_pass_2nd.entry_Fs.m];

    call __warp.call_entry;
    r2 = r0;
    r3 = r1;
    call $kal_float_lib.mul;
    r6 = r0;						// r6:r7 = w^2
    r7 = r1;
    r0 = r2;
    r1 = r3;
    r2 = M[r8 - calc_all_pass_2nd.entry_q.e];
    r3 = M[r8 - calc_all_pass_2nd.entry_q.m];
    call $kal_float_lib.div;        // w/q
    r4 = r0;						// r4:r5 = w/q
    r5 = r1;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.add;        // w^2 + w/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w^2 + w/q + 1
    call $kal_float_lib.recip;      // denom
    m[r8 + calc_all_pass_2nd.local_1] = r0;         // denom = 1/(w^2+w/q+1)
    m[r8 + calc_all_pass_2nd.local_2] = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = (w^2 - w/q + 1) / denom
    r0 = r6;
    r1 = r7;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.sub;    
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    r2 = m[r8 + calc_all_pass_2nd.local_1];
    r3 = m[r8 + calc_all_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    r4 = r0;
    r5 = r1;
    
    // b1 = (2.w^2 - 2) / denom
    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w^2
    call $kal_float_lib.sub;        // 2.w^2 - 2
    r2 = m[r8 + calc_all_pass_2nd.local_1];
    r3 = m[r8 + calc_all_pass_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;
    r6 = r0;
    r7 = r1;

    // b2 = 1.0
    r0 = $kal_float_lib.const.unity.exp;
    r1 = $kal_float_lib.const.unity.mant;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // a1 = (2.w^2 - 2) / denom   = b1
    // pushm <r6,r7>;
    push r6;
    push r7;
    
    // a2 = (w^2 - w/q + 1) / denom   = b0
    // pushm <r4,r5>;
    push r4;
    push r5;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_all_pass_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // sp = sp - calc_all_pass_2nd.local_stack_size;
    MACRO_SP_MINUS(calc_all_pass_2nd.local_stack_size)
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;
    
    // sp = sp - calc_all_pass_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_all_pass_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_shelf_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order shelf filter with prescribed nyquist
// frequency gain (gh)
//
//   lg = 10^(gh/20)
//   w1 = w/10^(g/80)
//   w2 = w*10^(g/80)
//
//               s^2 + (w1/q)s + w1^2
//   H(s) = lg * --------------------
//               s^2 + (w2/q)s + w2^2
//
//------------------------------------------------------------------------------
//
//               w1^2 + w1/q + 1           2.w1^2 - 2               w1^2 - w1/q + 1
//          lg * --------------- + lg * --------------- z^-1 + lg * --------------- z^-2
//               w2^2 + w2/q + 1        w2^2 + w2/q + 1             w2^2 + w2/q + 1
//   H(z) = ----------------------------------------------------------------------------
//                                       2.w2^2 - 2              w2^2 - w2/q + 1
//                     1         +    --------------- z^-1   +   --------------- z^-2
//                                    w2^2 + w2/q + 1            w2^2 + w2/q + 1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_shelf_2nd.entry_fc.e       10;
.const calc_shelf_2nd.entry_fc.m        9;
.const calc_shelf_2nd.entry_g.e         8;
.const calc_shelf_2nd.entry_g.m         7;
.const calc_shelf_2nd.entry_q.e         6;
.const calc_shelf_2nd.entry_q.m         5;
.const calc_shelf_2nd.entry_gh.e        4;
.const calc_shelf_2nd.entry_gh.m        3;
.const calc_shelf_2nd.entry_Fs.e        2;
.const calc_shelf_2nd.entry_Fs.m        1;
.const calc_shelf_2nd.entry_stack_size 10;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_shelf_2nd.stack_temp        10;
.const calc_shelf_2nd.local_1           calc_shelf_2nd.stack_temp + 0;
.const calc_shelf_2nd.local_2           calc_shelf_2nd.stack_temp + 1;
.const calc_shelf_2nd.local_3           calc_shelf_2nd.stack_temp + 2;
.const calc_shelf_2nd.local_4           calc_shelf_2nd.stack_temp + 3;
.const calc_shelf_2nd.local_5           calc_shelf_2nd.stack_temp + 4;
.const calc_shelf_2nd.local_6           calc_shelf_2nd.stack_temp + 5;
.const calc_shelf_2nd.local_7           calc_shelf_2nd.stack_temp + 6;
.const calc_shelf_2nd.local_8           calc_shelf_2nd.stack_temp + 7;
.const calc_shelf_2nd.local_stack_size  8;
//------------------------------------------------------------------------------
// on exit - all return values on stack
.const calc_shelf_2nd.exit_b0           6;
.const calc_shelf_2nd.exit_b1           5;
.const calc_shelf_2nd.exit_b2           4;
.const calc_shelf_2nd.exit_a1           3;
.const calc_shelf_2nd.exit_a2           2;
.const calc_shelf_2nd.exit_scale        1;
.const calc_shelf_2nd.exit_stack_size   6;
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  = 10
//   temporary storage = 47
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_shelf_2nd.local_stack_size;
    MACRO_SP_PLUS(calc_shelf_2nd.local_stack_size)

    r0 = M[r8 - calc_shelf_2nd.entry_fc.e];
    r1 = M[r8 - calc_shelf_2nd.entry_fc.m];
    r2 = M[r8 - calc_shelf_2nd.entry_Fs.e];
    r3 = M[r8 - calc_shelf_2nd.entry_Fs.m];
    call __warp.call_entry;
    r4 = r0;
    r5 = r1;

    r0 = M[r8 - calc_shelf_2nd.entry_gh.e];
    r1 = M[r8 - calc_shelf_2nd.entry_gh.m];
    call __db2lin.call_entry;
    m[r8 + calc_shelf_2nd.local_3] = r0;
    m[r8 + calc_shelf_2nd.local_4] = r1;
    
    r0 = M[r8 - calc_shelf_2nd.entry_g.e];
    r1 = M[r8 - calc_shelf_2nd.entry_g.m];
    r0 = r0 - 2;
    call __db2lin.call_entry;
    r2 = r0;
    r3 = r1;
    
    r0 = r4;
    r1 = r5;
    call $kal_float_lib.div;        // w2
    r6 = r0;
    r7 = r1;
    
    r0 = r4;
    r1 = r5;
    call $kal_float_lib.mul;        // w1
    r4 = r0;
    r5 = r1;

    r2 = M[r8 - calc_shelf_2nd.entry_q.e];
    r3 = M[r8 - calc_shelf_2nd.entry_q.m];
    call $kal_float_lib.div;        // w1/q
    m[r8 + calc_shelf_2nd.local_5] = r0;
    m[r8 + calc_shelf_2nd.local_6] = r1;
    
    r0 = r6;
    r1 = r7;
    call $kal_float_lib.div;        // w2/q
    m[r8 + calc_shelf_2nd.local_7] = r0;
    m[r8 + calc_shelf_2nd.local_8] = r1;
    
    r0 = r6;
    r1 = r7;
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.mul;        // w2^2
    r6 = r0;
    r7 = r1;
    r2 = m[r8 + calc_shelf_2nd.local_7];
    r3 = m[r8 + calc_shelf_2nd.local_8];
    call $kal_float_lib.add;        // w2^2 + w2/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w2^2 + w2/q + 1
    call $kal_float_lib.recip;      // denom = 1/(w2^2 + w2/q + 1)
    m[r8 + calc_shelf_2nd.local_1] = r0;
    m[r8 + calc_shelf_2nd.local_2] = r1;

    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = lg * (w1^2 + w1/q + 1) / denom
    r0 = r4;
    r1 = r5;
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.mul;        // w1^2
    r4 = r0;
    r5 = r1;
    r2 = m[r8 + calc_shelf_2nd.local_5];
    r3 = m[r8 + calc_shelf_2nd.local_6];
    call $kal_float_lib.add;        // w1^2 + w1/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w1^2 + w1/q + 1
    r2 = m[r8 + calc_shelf_2nd.local_1];
    r3 = m[r8 + calc_shelf_2nd.local_2];
    call $kal_float_lib.mul;
    r2 = m[r8 + calc_shelf_2nd.local_3];
    r3 = m[r8 + calc_shelf_2nd.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b0
    push r0;
    push r1;

    // b1 = (2.w1^2 - 2) / denom
    r0 = r4;
    r1 = r5;
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w1^2
    call $kal_float_lib.sub;        // 2.w1^2 - 2
    r2 = m[r8 + calc_shelf_2nd.local_1];
    r3 = m[r8 + calc_shelf_2nd.local_2];
    call $kal_float_lib.mul;
    r2 = m[r8 + calc_shelf_2nd.local_3];
    r3 = m[r8 + calc_shelf_2nd.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b1
    push r0;
    push r1;

    // b2 = lg * (w1^2 - w1/q + 1) / denom
    r0 = r4;
    r1 = r5;
    r2 = m[r8 + calc_shelf_2nd.local_5];
    r3 = m[r8 + calc_shelf_2nd.local_6];
    call $kal_float_lib.sub;        // w1^2 - w1/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w1^2 - w1/q + 1
    r2 = m[r8 + calc_shelf_2nd.local_1];
    r3 = m[r8 + calc_shelf_2nd.local_2];
    call $kal_float_lib.mul;
    r2 = m[r8 + calc_shelf_2nd.local_3];
    r3 = m[r8 + calc_shelf_2nd.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // b2
    push r0;
    push r1;
    
    // a1 = (2.w2^2 - 2) / denom
    r0 = r6;
    r1 = r7;
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w2^2
    call $kal_float_lib.sub;        // 2.w2^2 - 2
    r2 = m[r8 + calc_shelf_2nd.local_1];
    r3 = m[r8 + calc_shelf_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // a1
    push r0;
    push r1;

    // a2 = lg * (w2^2 - w2/q + 1) / denom
    r0 = r6;
    r1 = r7;
    r2 = m[r8 + calc_shelf_2nd.local_7];
    r3 = m[r8 + calc_shelf_2nd.local_8];
    call $kal_float_lib.sub;        // w2^2 - w2/q
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;        // w2^2 - w2/q + 1
    r2 = m[r8 + calc_shelf_2nd.local_1];
    r3 = m[r8 + calc_shelf_2nd.local_2];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;                   // a2
    push r0;
    push r1;
    
    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_shelf_2nd.entry_stack_size+calc_shelf_2nd.exit_b0)] = r0;
    
    // sp = sp - calc_shelf_2nd.local_stack_size;
    MACRO_SP_MINUS(calc_shelf_2nd.local_stack_size)
    
    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;
    
    // sp = sp - calc_shelf_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_shelf_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_low_shelf_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order low frequency shelf filter
//
//   w1 = w/10^(g/80)
//   w2 = w*10^(g/80)
//
//          s^2 + (w1/q)s + w1^2
//   H(s) = --------------------
//          s^2 + (w2/q)s + w2^2
//
//------------------------------------------------------------------------------
//
//          w1^2 + w1/q + 1      2.w1^2 - 2          w1^2 - w1/q + 1
//          --------------- + --------------- z^-1 + --------------- z^-2
//          w2^2 + w2/q + 1   w2^2 + w2/q + 1        w2^2 + w2/q + 1
//   H(z) = -------------------------------------------------------------
//                               2.w2^2 - 2          w2^2 - w2/q + 1
//                  1       + --------------- z^-1 + --------------- z^-2
//                            w2^2 + w2/q + 1        w2^2 + w2/q + 1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_2nd' to calculate coefficients
// - return (calc_shelf_2nd(fc,g,q,0,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_low_shelf_2nd.entry_fc.e        8;
.const calc_low_shelf_2nd.entry_fc.m        7;
.const calc_low_shelf_2nd.entry_g.e         6;
.const calc_low_shelf_2nd.entry_g.m         5;
.const calc_low_shelf_2nd.entry_q.e         4;
.const calc_low_shelf_2nd.entry_q.m         3;
.const calc_low_shelf_2nd.entry_Fs.e        2;
.const calc_low_shelf_2nd.entry_Fs.m        1;
.const calc_low_shelf_2nd.entry_stack_size  8;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  8
//   temporary storage = 67
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_2nd"
    // sp = sp + calc_shelf_2nd.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_2nd.exit_stack_size)
    
    r0 = M[r8 - calc_low_shelf_2nd.entry_fc.e];
    r1 = M[r8 - calc_low_shelf_2nd.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_low_shelf_2nd.entry_g.e];
    r1 = M[r8 - calc_low_shelf_2nd.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
    r0 = M[r8 - calc_low_shelf_2nd.entry_q.e];
    r1 = M[r8 - calc_low_shelf_2nd.entry_q.m];
    // pushm <r0,r1>;                              // q
    push r0;
    push r1;
    r0 = $kal_float_lib.const.zero.exp;
    r1 = $kal_float_lib.const.zero.mant;
    // pushm <r0,r1>;                              // gh (0dB)
    push r0;
    push r1;
    r0 = M[r8 - calc_low_shelf_2nd.entry_Fs.e];
    r1 = M[r8 - calc_low_shelf_2nd.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_2nd;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_low_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_low_shelf_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_low_shelf_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_high_shelf_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order low frequency shelf filter
//
//   lg = 10^(g/20)
//   w1 = w/10^(g/80)
//   w2 = w*10^(g/80)
//
//               s^2 + (w1/q)s + w1^2
//   H(s) = lg * --------------------
//               s^2 + (w2/q)s + w2^2
//
//------------------------------------------------------------------------------
//
//               w1^2 + w1/q + 1           2.w1^2 - 2               w1^2 - w1/q + 1
//          lg * --------------- + lg * --------------- z^-1 + lg * --------------- z^-2
//               w2^2 + w2/q + 1        w2^2 + w2/q + 1             w2^2 + w2/q + 1
//   H(z) = ----------------------------------------------------------------------------
//                                       2.w2^2 - 2              w2^2 - w2/q + 1
//                     1         +    --------------- z^-1   +   --------------- z^-2
//                                    w2^2 + w2/q + 1            w2^2 + w2/q + 1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_2nd' to calculate coefficients
// - return (calc_shelf_2nd(fc,g,q,g,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_high_shelf_2nd.entry_fc.e        8;
.const calc_high_shelf_2nd.entry_fc.m        7;
.const calc_high_shelf_2nd.entry_g.e         6;
.const calc_high_shelf_2nd.entry_g.m         5;
.const calc_high_shelf_2nd.entry_q.e         4;
.const calc_high_shelf_2nd.entry_q.m         3;
.const calc_high_shelf_2nd.entry_Fs.e        2;
.const calc_high_shelf_2nd.entry_Fs.m        1;
.const calc_high_shelf_2nd.entry_stack_size  8;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  8
//   temporary storage = 67
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_2nd"
    // sp = sp + calc_shelf_2nd.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_2nd.exit_stack_size)
    
    r0 = M[r8 - calc_high_shelf_2nd.entry_fc.e];
    r1 = M[r8 - calc_high_shelf_2nd.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_2nd.entry_g.e];
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r1 = M[r8 - calc_high_shelf_2nd.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_2nd.entry_q.e];
    r1 = M[r8 - calc_high_shelf_2nd.entry_q.m];
    // pushm <r0,r1>;                              // q
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_2nd.entry_g.e];
    r1 = M[r8 - calc_high_shelf_2nd.entry_g.m];
    // pushm <r0,r1>;                              // gh (g)
    push r0;
    push r1;
    r0 = M[r8 - calc_high_shelf_2nd.entry_Fs.e];
    r1 = M[r8 - calc_high_shelf_2nd.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_2nd;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_high_shelf_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_high_shelf_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_high_shelf_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_tilt_2nd:
//------------------------------------------------------------------------------
// calculate coefficients for second order tilt filter
//
//   lg = 10^(g/40)
//   w1 = w/10^(g/80)
//   w2 = w*10^(g/80)
//
//               s^2 + (w1/q)s + w1^2
//   H(s) = lg * --------------------
//               s^2 + (w2/q)s + w2^2
//
//------------------------------------------------------------------------------
//
//               w1^2 + w1/q + 1           2.w1^2 - 2               w1^2 - w1/q + 1
//          lg * --------------- + lg * --------------- z^-1 + lg * --------------- z^-2
//               w2^2 + w2/q + 1        w2^2 + w2/q + 1             w2^2 + w2/q + 1
//   H(z) = ----------------------------------------------------------------------------
//                                       2.w2^2 - 2              w2^2 - w2/q + 1
//                     1         +    --------------- z^-1   +   --------------- z^-2
//                                    w2^2 + w2/q + 1            w2^2 + w2/q + 1
//
//------------------------------------------------------------------------------
// uses 'calc_shelf_2nd' to calculate coefficients
// - return (calc_shelf_2nd(fc,g,q,g/2,Fs));
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_tilt_2nd.entry_fc.e        8;
.const calc_tilt_2nd.entry_fc.m        7;
.const calc_tilt_2nd.entry_g.e         6;
.const calc_tilt_2nd.entry_g.m         5;
.const calc_tilt_2nd.entry_q.e         4;
.const calc_tilt_2nd.entry_q.m         3;
.const calc_tilt_2nd.entry_Fs.e        2;
.const calc_tilt_2nd.entry_Fs.m        1;
.const calc_tilt_2nd.entry_stack_size  8;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  8
//   temporary storage = 67
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push rLink;
    
    // make space for return coefficient of "calc_shelf_2nd"
    // sp = sp + calc_shelf_2nd.exit_stack_size;
    MACRO_SP_PLUS(calc_shelf_2nd.exit_stack_size)
    
    r0 = M[r8 - calc_tilt_2nd.entry_fc.e];
    r1 = M[r8 - calc_tilt_2nd.entry_fc.m];
    // pushm <r0,r1>;                              // fc
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_2nd.entry_g.e];
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    r1 = M[r8 - calc_tilt_2nd.entry_g.m];
    // pushm <r0,r1>;                              // g
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_2nd.entry_q.e];
    r1 = M[r8 - calc_tilt_2nd.entry_q.m];
    // pushm <r0,r1>;                              // q
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_2nd.entry_g.e];
    r0 = r0 - 1;
    r1 = M[r8 - calc_tilt_2nd.entry_g.m];
    // pushm <r0,r1>;                              // gh (g)
    push r0;
    push r1;
    r0 = M[r8 - calc_tilt_2nd.entry_Fs.e];
    r1 = M[r8 - calc_tilt_2nd.entry_Fs.m];
    // pushm <r0,r1>;                              // Fs
    push r0;
    push r1;

    call calc_shelf_2nd;
    
    // copy return value from "calc_shelf_1st" onto return stack
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_tilt_2nd.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // popm <fp,r0,r1,rlink>;
    pop rlink;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_tilt_2nd.entry_stack_size;
    MACRO_SP_MINUS(calc_tilt_2nd.entry_stack_size)

    rts;



//------------------------------------------------------------------------------
calc_peq:
//------------------------------------------------------------------------------
// calculate coefficients for parametric filter
//
//          s^2 + (w/q1)s + w^2
//   H(s) = -------------------
//          s^2 + (w/q2)s + w^2
//
//------------------------------------------------------------------------------
//
//          w^2 + w/q1 + 1      2.w^2 - 2          w^2 - w/q1 + 1
//          -------------- + -------------- z^-1 + -------------- z^-2
//          w^2 + w/q2 + 1   w^2 + w/q2 + 1        w^2 + w/q2 + 1
//   H(z) = ----------------------------------------------------------
//                              2.w^2 - 2          w^2 - w/q2 + 1
//                1        + -------------- z^-1 + -------------- z^-2
//                           w^2 + w/q2 + 1        w^2 + w/q2 + 1
//
//------------------------------------------------------------------------------
// on entry - all parameters on stack
.const calc_peq.entry_fc.e        8;
.const calc_peq.entry_fc.m        7;
.const calc_peq.entry_g.e         6;
.const calc_peq.entry_g.m         5;
.const calc_peq.entry_q.e         4;
.const calc_peq.entry_q.m         3;
.const calc_peq.entry_Fs.e        2;
.const calc_peq.entry_Fs.m        1;
.const calc_peq.entry_stack_size  8;
//------------------------------------------------------------------------------
// local stack space (accessed via frame pointer)
.const calc_peq.stack_temp        10;
.const calc_peq.local_1           calc_peq.stack_temp + 0;
.const calc_peq.local_2           calc_peq.stack_temp + 1;
.const calc_peq.local_3           calc_peq.stack_temp + 2;
.const calc_peq.local_4           calc_peq.stack_temp + 3;
.const calc_peq.local_stack_size  4;
//------------------------------------------------------------------------------
// on exit - all return values on stack
//------------------------------------------------------------------------------
// registers destroyed
//   none
//------------------------------------------------------------------------------
// stack usage
//   entry parameters  =  8
//   temporary storage = 43
//   return values     =  6
//------------------------------------------------------------------------------

    // pushm <fp(=sp),r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    MACRO_FP_EQU_PUSH_SP
    push r0;
    push r1;
    push r2;
    push r3;
    push r4;
    push r5;
    push r6;
    push r7;
    push rLink;

    // sp = sp + calc_peq.local_stack_size;
    MACRO_SP_PLUS(calc_peq.local_stack_size)

	// first calculate 'alpha' from q
    r0 = M[r8 - calc_peq.entry_q.e];
    r0 = r0 + 1;
    r1 = M[r8 - calc_peq.entry_q.m];
    call $kal_float_lib.recip;		// r0:r1 = 1/(2q)
	r0 = r0 or $kal_float_lib.format.sign_bit;			// r0:r1 = -1/(2q)
	r2 = r0;
	r3 = r1;
	r4 = r0;
	r5 = r1;
    call $kal_float_lib.mul;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;
    call $kal_float_lib.sqrt;
	r2 = r4;
	r3 = r5;
    call $kal_float_lib.add;		// r0:r1 = alpha

    r2 = M[r8 - calc_peq.entry_fc.e];
    r3 = M[r8 - calc_peq.entry_fc.m];
    call $kal_float_lib.mul;
    r2 = M[r8 - calc_peq.entry_Fs.e];
    r3 = M[r8 - calc_peq.entry_Fs.m];
    call __warp.call_entry;
	r4 = r0;
	r5 = r1;						// r4:r5 = awd
    r0 = M[r8 - calc_peq.entry_fc.e];
    r1 = M[r8 - calc_peq.entry_fc.m];
    call __warp.call_entry;			// r0:r1 = wd
    M[r8 + calc_peq.local_1] = r0;
    M[r8 + calc_peq.local_2] = r1;
       
	r2 = r0;
	r3 = r1;
    call $kal_float_lib.mul;
	// pushm <r2,r3>;
    push r2;
    push r3;
	// pushm <r0,r1>;
    push r0;
    push r1;
	r0 = r4;
	r1 = r5;
	r2 = r4;
	r3 = r5;
    call $kal_float_lib.mul;
	//popm <r2,r3>;
    pop r3;
    pop r2;
    call $kal_float_lib.sub;
	r0 = r0 xor $kal_float_lib.format.sign_bit;
    call $kal_float_lib.recip;
	//popm <r2,r3>;
    pop r3;
    pop r2;
    call $kal_float_lib.mul;
	r2 = r4;
	r3 = r5;
    call $kal_float_lib.mul;		// r0:r1 = qd

	r4 = r0;
	r5 = r1;						// r4:r5 = qd
	
    r0 = M[r8 - calc_peq.entry_g.e];
    r1 = M[r8 - calc_peq.entry_g.m];
    r2 = $kal_float_lib.const.half.exp;
    r3 = $kal_float_lib.const.half.mant;
    call $kal_float_lib.mul;
	call __db2lin.call_entry;
	r2 = r0;
	r3 = r1;
	r0 = r4;
	r1 = r5;
    call $kal_float_lib.mul;
	r6 = r0;
	r7 = r1;						// r6:r7 = q2
	r0 = r4;
	r1 = r5;
    call $kal_float_lib.div;
	r2 = r0;
	r3 = r1;						// r2:r3 = q1

    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    call $kal_float_lib.div;
    r4 = r0;                        // r4:r5 = w/q1
    r5 = r1;

    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.div;
    r6 = r0;                        // r6:r7 = w/q2
    r7 = r1;

    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = r0;
    r3 = r1;
    call $kal_float_lib.mul;
    M[r8 + calc_peq.local_1] = r0;  // w^2
    M[r8 + calc_peq.local_2] = r1;

    // calculate denominator
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.add;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;		// r0:r1 = w^2 + w/q2 + 1
    call $kal_float_lib.recip;      // denom
    M[r8 + calc_peq.local_3] = r0;
    M[r8 + calc_peq.local_4] = r1;
    
    // make space for return coefficient of "__scale_coefficients"
    // sp = sp + __scale_coefficients.exit_stack_size;
    MACRO_SP_PLUS(__scale_coefficients.exit_stack_size)

    // b0 = (w^2 + w/q1 + 1) / denom
    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.add;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;		// r0:r1 = w^2 + w/q1 + 1
    r2 = m[r8 + calc_peq.local_3];
    r3 = m[r8 + calc_peq.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // b1 = (2.w^2 - 2) / denom
    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w^2
    call $kal_float_lib.sub;        // 2.w^2 - 2
    r2 = m[r8 + calc_peq.local_3];
    r3 = m[r8 + calc_peq.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // b2 = (w^2 - w/q1 + 1) / denom
    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = r4;
    r3 = r5;
    call $kal_float_lib.sub;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;		// r0:r1 = w^2 + w/q1 + 1
    r2 = m[r8 + calc_peq.local_3];
    r3 = m[r8 + calc_peq.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // a1 = (2.w^2 - 2) / denom
    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = $kal_float_lib.const.two.exp;
    r3 = $kal_float_lib.const.two.mant;
    call $kal_float_lib.mul;        // 2.w^2
    call $kal_float_lib.sub;        // 2.w^2 - 2
    r2 = m[r8 + calc_peq.local_3];
    r3 = m[r8 + calc_peq.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;

    // a2 = (w^2 - w/q2 + 1) / denom
    r0 = M[r8 + calc_peq.local_1];
    r1 = M[r8 + calc_peq.local_2];
    r2 = r6;
    r3 = r7;
    call $kal_float_lib.sub;
    r2 = $kal_float_lib.const.unity.exp;
    r3 = $kal_float_lib.const.unity.mant;
    call $kal_float_lib.add;		// r0:r1 = w^2 + w/q2 + 1
    r2 = m[r8 + calc_peq.local_3];
    r3 = m[r8 + calc_peq.local_4];
    call $kal_float_lib.mul;
    // pushm <r0,r1>;
    push r0;
    push r1;

    call __scale_coefficients;
    
    // copy return value from "__scale_coefficients" onto return stack
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_scale)] = r0;
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_a2)] = r0;
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_a1)] = r0;
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_b2)] = r0;
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_b1)] = r0;
    pop r0;
    m[r8 - (calc_peq.entry_stack_size+biquad_coeficients.exit_b0)] = r0;
    
    // sp = sp - calc_peq.local_stack_size;
    MACRO_SP_MINUS(calc_peq.local_stack_size)

    // popm <fp,r0,r1,r2,r3,r4,r5,r6,r7,rlink>;
    pop rlink;
    pop r7;
    pop r6;
    pop r5;
    pop r4;
    pop r3;
    pop r2;
    pop r1;
    pop r0;
    pop r8;

    // sp = sp - calc_peq.entry_stack_size;
    MACRO_SP_MINUS(calc_peq.entry_stack_size)

    rts;

