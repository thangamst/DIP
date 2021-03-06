/*
 * Copyright (c) 2014 Seppo Tomperi <seppo.tomperi@vtt.fi>
 *               2015 Morgan LACOUR <morgan.lacour@insa-rennes.fr>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include "libavutil/arm/asm.S"
#include "neon.S"

.macro regshuffle_d5
    vmov d16, d17
    vmov d17, d18
    vmov d18, d19
    vmov d19, d20
    vmov d20, d21
.endm

.macro regshuffle_q5
    vmov q0, q1
    vmov q1, q2
    vmov q2, q3
    vmov q3, q4
    vmov q4, q5
.endm

.macro vextin5
        pld       [r2]
        vld1.8    {q11}, [r2], r3
        vext.8    d16, d22, d23, #0
        vext.8    d17, d22, d23, #1
        vext.8    d18, d22, d23, #2
        vext.8    d19, d22, d23, #3
        vext.8    d20, d22, d23, #4
.endm

.macro loadin5
        pld       [r2]
        vld1.8    {d16}, [r2], r3
        pld       [r2]
        vld1.8    {d17}, [r2], r3
        pld       [r2]
        vld1.8    {d18}, [r2], r3
        pld       [r2]
        vld1.8    {d19}, [r2], r3
        pld       [r2]
        vld1.8    {d20}, [r2], r3
        pld       [r2]
        vld1.8    {d21}, [r2], r3
.endm

// 20 cycles
//  a  -  8*b  +  58*c  +  16*d - 3*e    <-------free------>
// q0      q1      q2       q3     q4    q5       q6      q7
// d0,1    d2,3    d4,5     d6,7  d8,9  d10,11   d12,13  d14,15
.macro qpel_filter5_1_32b
        vmov.i16   d16, #58
        vmov.i16   d17, #3
        vshll.s16  q6, d6, #4    // 16 * d0
        vshll.s16  q7, d7, #4    // 16 * d1
        vmull.s16   q9, d4, d16   // 58 * c0
        vmull.s16  q10, d5, d16   // 58 * c1
        vshll.s16  q11, d2, #3    // 8 * b0
        vshll.s16  q12, d3, #3    // 8 * b1
        vaddw.s16   q6, q6, d0       // 16 * d0 + a0
        vaddw.s16   q7, q7, d1       // 16 * d1 + a1
        vadd.s32    q9, q6       // 58 * c0 + 16 * d0 + a0
        vadd.s32   q10, q7       // 58 * c1 + 16 * d1 + a1
        vmull.s16  q6, d8, d17   // 3 * e0
        vmull.s16  q7, d9, d17   // 3 * e1
        vadd.s32    q11, q6       // 8*b0 + 3*e0
        vadd.s32    q12, q7       // 8*b1 + 3*e1
        vsub.s32    q9, q11       // 58 * c0 + 16 * d0 + a0 - 8*b0 - 3*e0
        vsub.s32   q10, q12       // 58 * c1 + 16 * d1 + a1 - 8*b1 - 3*e1
        vqshrn.s32  d16, q9, #6
        vqshrn.s32  d17, q10, #6
.endm


// 18 cycles
// 2*a  -  9*b  +  40*c  + 40*d - 9*e    <-------free------>
// q0      q1      q2       q3     q4    q5       q6      q7
// d0,1    d2,3    d4,5     d6,7  d8,9  d10,11   d12,13  d14,15
.macro qpel_filter5_2_32b
        vmov.i32   q8, #40
        vaddl.s16  q9, d6, d4  // d0 + c0
        vaddl.s16  q10, d7, d5  // d1 + c1
        vaddl.s16  q11, d2, d8 // b0 + e0
        vaddl.s16  q12, d3, d9 // b1 + e1
        vmul.s32   q9, q8   // 40 * (d0 + c0)
        vmul.s32   q10, q8     // 40 * (d1 + c1)
        vmov.i32   q8, #9
        vmul.s32   q11, q8      // 9 * (b0 + e0)
        vmul.s32   q12, q8      // 9 * (b1 + e1)
        vshll.s16  q6, d0, #1    // 2 * a0
        vshll.s16  q7, d1, #1    // 2 * a1
        vadd.s32   q9, q6       // 40 * (d0 + c0) + 2*a0
        vadd.s32   q10, q7       // 40 * (d1 + c1) + 2*a1
        vsub.s32   q9, q11       // 40 * (d0 + c0) + 2*a0 - 9 * (b0 + e0)
        vsub.s32   q10, q12      // 40 * (d1 + c1) + 2*a1 - 9 * (b1 + e1)
        vqshrn.s32  d16, q9, #6
        vqshrn.s32  d17, q10, #6
.endm


// 18 cycles
//  a   -  4*b  +  17*c  + 54*d - 4*e    <-------free------>
// q0      q1      q2       q3     q4    q5       q6      q7
// d0,1    d2,3    d4,5     d6,7  d8,9  d10,11   d12,13  d14,15
.macro qpel_filter5_3_32b
        vmov.i16   d16, #54
        vmull.s16   q9, d6, d16   // 54 * d0
        vmull.s16  q10, d7, d16   // 54 * d1
        vmov.i16   d17, #17
        vmull.s16  q11, d4, d17   // 17 * c0
        vmull.s16  q12, d5, d17   // 17 * c1
        vaddl.s16    q13, d2, d8    // b0 + e0
        vaddl.s16    q14, d3, d9    // b1 + e1
        vshl.s32     q13, #2        //  4 * ( b0 + e0 )
        vshl.s32     q14, #2        //  4 * ( b1 + e1 )  
        vaddw.s16    q9, d0       // 54 * d0 + a0
        vaddw.s16   q10, d1       // 54 * d1 + a1
        vsub.s32   q11, q13        // 17 * c0 - 4 * ( b0 + e0 )
        vsub.s32   q12, q14        // 17 * c1 - 4 * ( b1 + e1 )
        vadd.s32    q9, q11       // 54 * d0 + a0 + 17 * c0 - 4 * ( b0 + e0 )
        vadd.s32   q10, q12       // 54 * d1 + a1 + 17 * c1 - 4 * ( b1 + e1 )
        vqshrn.s32  d16, q9, #6
        vqshrn.s32  d17, q10, #6
.endm


//        a - 8*b + 58*c + 16*d - 3*e   <---free--->
// input d16  d17   d18     d19   d20   d21  d22  d23
// QPEL
// 10 cycles
.macro qpel_filter5_1 out=q7
        vmov.u8   d24, #58
        vshll.u8  q13, d19, #4     // 16*d
        vmov.u8   d25, #3
        vshll.u8  q14, d17, #3     // 8*b
        vaddw.u8 q13, q13, d16     // 16*d + a    
        vmull.u8  \out, d18, d24   // 58*c
        vadd.u16  \out, q13        // 16*d + 58*c + a
        vmull.u8   q13, d25, d20    // 3*e
        vadd.u16   q14, q13         // 8*b + 3*e
        vsub.s16   \out, q14         // 16*d + 58*c - 8*b - 3*e + a
.endm


//       2*a - 9*b + 40*c + 40*d - 9*e   <---free--->
// input d16  d17   d18     d19   d20   d21  d22  d23
// HPEL
// 9 cycles
.macro qpel_filter5_2 out=q7
        vmov.i16   q12, #40
        vmov.i16   q14, #9
        vaddl.u8   q13, d19, d18   // d + c
        vaddl.u8   q15, d17, d20   // b + e
        vmul.u16   q13, q12        // 40 * (d+c)
        vmul.u16   q15, q14        // 9 * ( b + e)
        vshll.u8   q12, d16, #1    // 2*a
        vsub.s16   \out, q13, q15   // 40 * (d+c) - 9 * ( b + e)
        vadd.s16   \out, q12   // 40 * (d+c) - 9 * ( b + e) +  2*a
.endm


//        a - 4*b + 17*c + 54*d - 4*e   <---free--->
// input d16  d17   d18     d19   d20   d21  d22  d23
// QPEL ret
// 9 cycles
.macro qpel_filter5_3 out=q7
        vmov.u8   d24, #54
        vmull.u8   \out, d19, d24    // 54*d
        vaddl.u8  q13, d17, d20    // b + e
        vshl.u16  q13, #2          // 4 * (b + e)
        vmov.u8   d25, #17
        vmull.u8   q15, d18, d25    // 17*c
        vaddw.u8   \out, \out, d16    // 54*d + a
        vsub.s16   q15, q13         // 17*c - 4 * (b + e)
        vadd.s16   \out, q15         // 54*d + a - 4 * (b + e) + 17*c
.endm

.macro  hevc_put_qpel5_vX_neon_8 filter
        push   {r4, r5, r6, r7}
        ldr    r4, [sp, #16] // height
        ldr    r5, [sp, #20] // width
        vpush {d8-d15}
        sub       r2, r2, r3, lsl #1	 	// pointe le premier pixel
        mov       r12, r4 		//_height
        mov       r6, r0 		// _dst
        mov       r7, r2 		// _src
        lsl       r1, #1 		// dststride*2
0:      loadin5 			
        cmp       r5, #4 	// width==4?
        beq       4f 		// br 4f si width=4
8:      subs r4, #1 		// height--
        \filter 			// q7 = vect filtre
        vst1.16    {q7}, [r0], r1 	// *dst=q7, dst+=dststride
        regshuffle_d5 		//
        vld1.8    {d21}, [r2], r3 //MODIF Green
        bne 8b 				// height!=0 boucle
        subs  r5, #8 		// width-=8
        beq       99f 		// si width==0 fin
        mov r4, r12 		// reload height
        add r6, #16 		// _dst += 16 (8*2octets)
        mov r0, r6 			// dst = _dst
        add r7, #8 			// _src += 8
        mov r2, r7 			// src = _src
        b     0b 			// br 0b
4:      subs r4, #1 		// height--
        \filter 			//q7 = vect filtre
        vst1.16    d14, [r0], r1 	// *dst=d14, dst+=dststride
        regshuffle_d5		//veillissement
        vld1.32    {d21[0]}, [r2], r3 	// MODIF Green
        bne 4b 
99:     vpop {d8-d15}
        pop {r4, r5, r6, r7}
        bx lr
.endm

.macro  hevc_put_qpel5_uw_vX_neon_8 filter
        push   {r4-r10}
        ldr    r5, [sp, #28] // width
        ldr    r4, [sp, #32] // height
        ldr    r8, [sp, #36] // src2
        ldr    r9, [sp, #40] // src2stride
        vpush {d8-d15}
        sub       r2, r2, r3, lsl #1	
        mov       r12, r4
        mov       r6, r0
        mov       r7, r2
        cmp       r8, #0
        bne       .Lbi\@
0:      loadin5
        cmp       r5, #4
        beq       4f
8:      subs r4, #1
        \filter
        vqrshrun.s16   d0, q7, #6
        vst1.8    d0, [r0], r1
        regshuffle_d5
        vld1.8    {d21}, [r2], r3 	// MODIF Green
        bne 8b
        subs  r5, #8
        beq       99f
        mov r4, r12
        add r6, #8
        mov r0, r6
        add r7, #8
        mov r2, r7
        b     0b
4:      subs r4, #1
        \filter
        vqrshrun.s16   d0, q7, #6
        vst1.32    d0[0], [r0], r1
        regshuffle_d5
        vld1.32    {d21[0]}, [r2], r3 	//MODIF Green
        bne 4b
        b   99f
.Lbi\@: lsl       r9, #1
        mov       r10, r8 	// _src2
0:      loadin5
        cmp       r5, #4
        beq       4f
8:      subs r4, #1
        \filter
        vld1.16        {q0}, [r8], r9
        vqadd.s16      q0, q7
        vqrshrun.s16   d0, q0, #7
        vst1.8         d0, [r0], r1
        regshuffle_d5
        vld1.8    {d21}, [r2], r3	//MODIF Green
        bne 8b
        subs  r5, #8
        beq       99f
        mov r4, r12
        add r6, #8
        mov r0, r6
        add r10, #16
        mov r8, r10
        add r7, #8
        mov r2, r7
        b     0b
4:      subs r4, #1
        \filter
        vld1.16      d0, [r8], r9
        vqadd.s16    d0, d14
        vqrshrun.s16 d0, q0, #7
        vst1.32      d0[0], [r0], r1
        regshuffle_d5
        vld1.32    {d21[0]}, [r2], r3	//MODIF Green
        bne 4b
99:     vpop {d8-d15}
        pop {r4-r10}
        bx lr
.endm


function ff_hevc_put_qpel5_v1_neon_8, export=1
        hevc_put_qpel5_vX_neon_8 qpel_filter5_1
endfunc

function ff_hevc_put_qpel5_v2_neon_8, export=1
        hevc_put_qpel5_vX_neon_8 qpel_filter5_2
endfunc

function ff_hevc_put_qpel5_v3_neon_8, export=1
        hevc_put_qpel5_vX_neon_8 qpel_filter5_3
endfunc


function ff_hevc_put_qpel5_uw_v1_neon_8, export=1
        hevc_put_qpel5_uw_vX_neon_8 qpel_filter5_1
endfunc

function ff_hevc_put_qpel5_uw_v2_neon_8, export=1
        hevc_put_qpel5_uw_vX_neon_8 qpel_filter5_2
endfunc

function ff_hevc_put_qpel5_uw_v3_neon_8, export=1
        hevc_put_qpel5_uw_vX_neon_8 qpel_filter5_3
endfunc


.macro hevc_put_qpel5_hX_neon_8 filter
        push     {r4, r5, r6, r7}
        ldr    r4, [sp, #16] // height
        ldr    r5, [sp, #20] // width
        vpush    {d8-d15}
        sub       r2, #2
        lsl       r1, #1
        mov      r12, r4
        mov       r6, r0
        mov       r7, r2
        cmp       r5, #4
        beq       4f
8:      subs      r4, #1
        vextin5
        \filter
        vst1.16   {q7}, [r0], r1
        bne       8b
        subs      r5, #8
        beq      99f
        mov       r4, r12
        add       r6, #16
        mov       r0, r6
        add       r7, #8
        mov       r2, r7
        cmp       r5, #4
        bne       8b
4:      subs      r4, #1
        vextin5
        \filter
        vst1.16  d14, [r0], r1
        bne       4b
99:     vpop     {d8-d15}
        pop      {r4, r5, r6, r7}
        bx lr
.endm

/*(uint8_t *dst,  ptrdiff_t dststride,     \
                                       uint8_t *_src, ptrdiff_t _srcstride,    \
                                       int width, int height,                  \
                                       int16_t* src2, ptrdiff_t src2stride)*/


.macro hevc_put_qpel5_uw_hX_neon_8 filter
        push     {r4-r10}
        ldr       r5, [sp, #28] // width
        ldr       r4, [sp, #32] // height
        ldr       r8, [sp, #36] // src2
        ldr       r9, [sp, #40] // src2stride
        vpush    {d8-d15}
        sub       r2, #2
        mov      r12, r4
        mov       r6, r0
        mov       r7, r2
        cmp       r8, #0
        bne       .Lbi\@
        cmp       r5, #4
        beq       4f
8:      subs      r4, #1
        vextin5
        \filter
        vqrshrun.s16   d0, q7, #6
        vst1.8    d0, [r0], r1
        bne       8b
        subs      r5, #8
        beq      99f
        mov       r4, r12
        add       r6, #8
        mov       r0, r6
        add       r7, #8
        mov       r2, r7
        cmp       r5, #4
        bne       8b
4:      subs      r4, #1
        vextin5
        \filter
        vqrshrun.s16   d0, q7, #6
        vst1.32  d0[0], [r0], r1
        bne       4b
        b         99f
.Lbi\@:
        lsl       r9, #1
        cmp       r5, #4
        beq       4f
        mov       r10, r8
8:      subs      r4, #1
        vextin5
        \filter
        vld1.16        {q0}, [r8], r9
        vqadd.s16      q0, q7
        vqrshrun.s16   d0, q0, #7
        vst1.8         d0, [r0], r1
        bne       8b
        subs      r5, #8
        beq      99f
        mov       r4, r12
        add       r6, #8
        add       r10, #16
        mov       r8, r10
        mov       r0, r6
        add       r7, #8
        mov       r2, r7
        cmp       r5, #4
        bne       8b
4:      subs      r4, #1
        vextin5
        \filter
        vld1.16      d0, [r8], r9
        vqadd.s16    d0, d14
        vqrshrun.s16 d0, q0, #7
        vst1.32      d0[0], [r0], r1
        bne       4b
99:     vpop     {d8-d15}
        pop      {r4-r10}
        bx lr
.endm


function ff_hevc_put_qpel5_h1_neon_8, export=1
        hevc_put_qpel5_hX_neon_8 qpel_filter5_1
endfunc

function ff_hevc_put_qpel5_h2_neon_8, export=1
        hevc_put_qpel5_hX_neon_8 qpel_filter5_2
endfunc

function ff_hevc_put_qpel5_h3_neon_8, export=1
        hevc_put_qpel5_hX_neon_8 qpel_filter5_3
endfunc


function ff_hevc_put_qpel5_uw_h1_neon_8, export=1
        hevc_put_qpel5_uw_hX_neon_8 qpel_filter5_1
endfunc

function ff_hevc_put_qpel5_uw_h2_neon_8, export=1
        hevc_put_qpel5_uw_hX_neon_8 qpel_filter5_2
endfunc

function ff_hevc_put_qpel5_uw_h3_neon_8, export=1
        hevc_put_qpel5_uw_hX_neon_8 qpel_filter5_3
endfunc

//hevc.c:41:const uint8_t ff_hevc_qpel_extra_before[4] = { 0, 3, 3, 2 };
//hevc.c:42:const uint8_t ff_hevc_qpel_extra_after[4]  = { 0, 3, 4, 4 };
//hevc.c:43:const uint8_t ff_hevc_qpel_extra[4]        = { 0, 6, 7, 6 };

.macro hevc_put_qpel5_hXvY_neon_8 filterh filterv
        push   {r4, r5, r6, r7}
        ldr    r4, [sp, #16] // height
        ldr    r5, [sp, #20] // width

        vpush {d8-d15}
        sub       r2, #2
	    sub       r2, r2, r3, lsl #1
        lsl       r1, #1
        mov       r12, r4
        mov       r6, r0
        mov       r7, r2
0:      vextin5
        \filterh q0
        vextin5
        \filterh q1
        vextin5
        \filterh q2
        vextin5
        \filterh q3
        vextin5
        \filterh q4
        vextin5
        \filterh q5
        cmp r5, #4
        beq 4f
8:      subs  r4, #1
        \filterv
        vst1.16    {q8}, [r0], r1
        regshuffle_q5
        vextin5
        \filterh q5 	//MODIF Green
        bne 8b
        subs  r5, #8
        beq 99f
        mov r4, r12
        add r6, #16
        mov r0, r6
        add r7, #8
        mov r2, r7
        b 0b
4:      subs  r4, #1
        \filterv
        vst1.16    d16, [r0], r1
        regshuffle_q5
        vextin5
        \filterh q5 	// MODIF Green
        bne 4b
99:     vpop {d8-d15}
        pop {r4, r5, r6, r7}
        bx lr
.endm

.macro hevc_put_qpel5_uw_hXvY_neon_8 filterh filterv
        push     {r4-r10}
        ldr       r5, [sp, #28] // width
        ldr       r4, [sp, #32] // height
        ldr       r8, [sp, #36] // src2
        ldr       r9, [sp, #40] // src2stride
        vpush {d8-d15}
        sub       r2, #2
        sub       r2, r2, r3, lsl #1  // extra_before 2
        mov       r12, r4 		// _height
        mov       r6, r0 		// _dst
        mov       r7, r2 		// _src
        cmp       r8, #0 		// src2?
        bne       .Lbi\@ 		//src2 branch
0:      vextin5
        \filterh q0
        vextin5
        \filterh q1
        vextin5
        \filterh q2
        vextin5
        \filterh q3
        vextin5
        \filterh q4
        vextin5
        \filterh q5
        cmp r5, #4
        beq 4f
8:      subs  r4, #1
        \filterv
        vqrshrun.s16   d0, q8, #6
        vst1.8    d0, [r0], r1
        regshuffle_q5
        vextin5
        \filterh q5	//MODIF Green
        bne 8b
        subs  r5, #8
        beq 99f
        mov r4, r12
        add r6, #8
        mov r0, r6
        add r7, #8
        mov r2, r7
        b 0b
4:      subs  r4, #1 	// height--
        \filterv
        vqrshrun.s16   d0, q8, #6
        vst1.32        d0[0], [r0], r1
        regshuffle_q5
        vextin5
        \filterh q5 	//MODIF Green
        bne 4b
        b   99f
.Lbi\@: lsl      r9, #1 	// BIDIR PROCESS
        mov      r10, r8 	//_src2
0:      vextin5 		// load src1
        \filterh q0
        vextin5
        \filterh q1
        vextin5
        \filterh q2
        vextin5
        \filterh q3
        vextin5
        \filterh q4
        vextin5
        \filterh q5
        cmp r5, #4
        beq 4f
8:      subs  r4, #1 	//height--
        \filterv 		// q8 out
        vld1.16        {q0}, [r8], r9 	//q0=src2[0-15], src2+=src2stride
        vqadd.s16      q0, q8 			// q0+=q8
        vqrshrun.s16   d0, q0, #7 		// recadrage
        vst1.8         d0, [r0], r1 	// *dst=d0, dst+=dststride
        regshuffle_q5
        vextin5
        \filterh q5 	//MODIF Green
        bne 8b
        subs  r5, #8
        beq 99f
        mov r4, r12
        add r6, #8
        mov r0, r6
        add r10, #16
        mov r8, r10
        add r7, #8
        mov r2, r7
        b 0b
4:      subs  r4, #1
        \filterv
        vld1.16      d0, [r8], r9
        vqadd.s16    d0, d16
        vqrshrun.s16 d0, q0, #7
        vst1.32      d0[0], [r0], r1
        regshuffle_q5
        vextin5
        \filterh q5 	//MODIF Green
        bne 4b
99:     vpop {d8-d15}
        pop {r4-r10}
        bx lr
.endm



function ff_hevc_put_qpel5_h1v1_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_1 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_h2v1_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_2 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_h3v1_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_3 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_h1v2_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_1 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_h2v2_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_2 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_h3v2_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_3 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_h1v3_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_1 qpel_filter5_3_32b
endfunc

function ff_hevc_put_qpel5_h2v3_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_2 qpel_filter5_3_32b
endfunc

function ff_hevc_put_qpel5_h3v3_neon_8, export=1
        hevc_put_qpel5_hXvY_neon_8 qpel_filter5_3 qpel_filter5_3_32b
endfunc


function ff_hevc_put_qpel5_uw_h1v1_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_1 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_uw_h2v1_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_2 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_uw_h3v1_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_3 qpel_filter5_1_32b
endfunc

function ff_hevc_put_qpel5_uw_h1v2_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_1 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_uw_h2v2_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_2 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_uw_h3v2_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_3 qpel_filter5_2_32b
endfunc

function ff_hevc_put_qpel5_uw_h1v3_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_1 qpel_filter5_3_32b
endfunc

function ff_hevc_put_qpel5_uw_h2v3_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_2 qpel_filter5_3_32b
endfunc

function ff_hevc_put_qpel5_uw_h3v3_neon_8, export=1
        hevc_put_qpel5_uw_hXvY_neon_8 qpel_filter5_3 qpel_filter5_3_32b
endfunc