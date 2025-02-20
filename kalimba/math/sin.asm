// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************

#ifndef MATH_SIN_INCLUDED
#define MATH_SIN_INCLUDED

// *****************************************************************************
// MODULE:
//    $math.sin
//
// DESCRIPTION:
//    Library subroutine to evaluate the Sine of a number
//
// INPUTS:
//    - r0 = input angle -2^23 == -180�,  +2^23 == 180�
//
// OUTPUTS:
//    - r1 = result (accurate to 20bits. RMS error is 2bits))
//
// TRASHED REGISTERS:
//    rMAC, r2, r3, I0
//
// CPU USAGE:
//    18 cycles
//
// *****************************************************************************

.MODULE $M.math.sin;
   .CODESEGMENT MATH_SIN_PM;
   .DATASEGMENT DM;

   $math.sin:   

   .VAR/DM1 coefs[6] = -0.11366725989793,  0.39038741091078,
                   -0.02290539398603,  -0.64216866271009,
                   -0.00029745724368,  0.39270721646879;

   I0 = &coefs;
   push M1;
   M1 = 1;
   r1 = r0,                rMAC = M[I0,M1];

   // if 2nd or 4th quad. then invert input
   Null = r1 LSHIFT 1;
   if NEG r1 = -r1,        r2 = M[I0,M1];
   // remove sign bit
   r1 = r1 AND 0x7FFFFF;

   // x^1*coefs(6) + x^0*coefs(5)
   r2   = r2 + r1*rMAC,    rMAC=M[I0,M1];
   // x^2*coefs(6) + x^1*coefs(5) + x^0*coefs(4)
   rMAC = rMAC + r1*r2,    r2=M[I0,M1];
   // x^3*coefs(6) + x^2*coefs(5) + x^1*coefs(4) + x^0*coefs(3)
   r2   = r2 + r1*rMAC,    rMAC=M[I0,M1];
   // x^4*coefs(6) + x^3*coefs(5) + x^2*coefs(4) + x^1*coefs(3)+ x^0*coefs(2)
   rMAC = rMAC + r1*r2,    r2=M[I0,M1];
   // x^5*coefs(6) + x^4*coefs(5) + x^3*coefs(4) + x^2*coefs(3)+ x^1*coefs(2) + x^0*coefs(1)
   r2   = r2 + r1*rMAC;
   // x^6*coefs(6) + x^5*coefs(5) + x^4*coefs(4) + x^3*coefs(3)+ x^2*coefs(2) + x^1*coefs(1)
   rMAC = r1*r2 (frac);

   pop M1;
   // rescale
   r1 = rMAC ASHIFT 3;
   // negate output if needed
   Null = r0;
   if NEG r1 = -r1;
   rts;

.ENDMODULE;


#endif // MATH_SIN_INCLUDED
