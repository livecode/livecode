
/* filter_neon.S - NEON optimised filter functions
 *
 * Copyright (c) 2011 Glenn Randers-Pehrson
 * Written by Mans Rullgard, 2011.
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

// IM-2012-10-03 Modified to compile for iOS

.macro begin_func
	.align 2
	.arm
	.globl _$0
	.no_dead_strip _$0
	.private_extern _$0
_$0:
.endmacro

begin_func png_read_filter_row_sub4_neon
        ldr             r3,  [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
1:
        vld4.32         {d4[],d5[],d6[],d7[]},    [r1,:128]
        vadd.u8         d0,  d3,  d4
        vadd.u8         d1,  d0,  d5
        vadd.u8         d2,  d1,  d6
        vadd.u8         d3,  d2,  d7
        vst4.32         {d0[0],d1[0],d2[0],d3[0]},[r1,:128]!
        subs            r3,  r3,  #16
        bgt             1b

        bx              lr

begin_func png_read_filter_row_sub3_neon
        ldr             r3,  [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
        mov             r0,  r1
        mov             r2,  #3
        mov             r12, #12
        vld1.8          {q11},    [r0], r12
1:
        vext.8          d5,  d22, d23, #3
        vadd.u8         d0,  d3,  d22
        vext.8          d6,  d22, d23, #6
        vadd.u8         d1,  d0,  d5
        vext.8          d7,  d23, d23, #1
        vld1.8          {q11},    [r0], r12
        vst1.32         {d0[0]},  [r1,:32], r2
        vadd.u8         d2,  d1,  d6
        vst1.32         {d1[0]},  [r1], r2
        vadd.u8         d3,  d2,  d7
        vst1.32         {d2[0]},  [r1], r2
        vst1.32         {d3[0]},  [r1], r2
        subs            r3,  r3,  #12
        bgt             1b

        bx              lr

begin_func png_read_filter_row_up_neon
        ldr             r3,  [r0, #4]           @ rowbytes
1:
        vld1.8          {q0}, [r1,:128]
        vld1.8          {q1}, [r2,:128]!
        vadd.u8         q0,  q0,  q1
        vst1.8          {q0}, [r1,:128]!
        subs            r3,  r3,  #16
        bgt             1b

        bx              lr

begin_func png_read_filter_row_avg4_neon
        ldr             r12, [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
1:
        vld4.32         {d4[],d5[],d6[],d7[]},    [r1,:128]
        vld4.32         {d16[],d17[],d18[],d19[]},[r2,:128]!
        vhadd.u8        d0,  d3,  d16
        vadd.u8         d0,  d0,  d4
        vhadd.u8        d1,  d0,  d17
        vadd.u8         d1,  d1,  d5
        vhadd.u8        d2,  d1,  d18
        vadd.u8         d2,  d2,  d6
        vhadd.u8        d3,  d2,  d19
        vadd.u8         d3,  d3,  d7
        vst4.32         {d0[0],d1[0],d2[0],d3[0]},[r1,:128]!
        subs            r12, r12, #16
        bgt             1b

        bx              lr

begin_func png_read_filter_row_avg3_neon
        push            {r4,lr}
        ldr             r12, [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
        mov             r0,  r1
        mov             r4,  #3
        mov             lr,  #12
        vld1.8          {q11},    [r0], lr
1:
        vld1.8          {q10},    [r2], lr
        vext.8          d5,  d22, d23, #3
        vhadd.u8        d0,  d3,  d20
        vext.8          d17, d20, d21, #3
        vadd.u8         d0,  d0,  d22
        vext.8          d6,  d22, d23, #6
        vhadd.u8        d1,  d0,  d17
        vext.8          d18, d20, d21, #6
        vadd.u8         d1,  d1,  d5
        vext.8          d7,  d23, d23, #1
        vld1.8          {q11},    [r0], lr
        vst1.32         {d0[0]},  [r1,:32], r4
        vhadd.u8        d2,  d1,  d18
        vst1.32         {d1[0]},  [r1], r4
        vext.8          d19, d21, d21, #1
        vadd.u8         d2,  d2,  d6
        vhadd.u8        d3,  d2,  d19
        vst1.32         {d2[0]},  [r1], r4
        vadd.u8         d3,  d3,  d7
        vst1.32         {d3[0]},  [r1], r4
        subs            r12, r12, #12
        bgt             1b

        pop             {r4,pc}

.macro  paeth
		vaddl.u8        q12, $1, $2           @ a + b
		vaddl.u8        q15, $3, $3           @ 2*c
		vabdl.u8        q13, $2, $3           @ pa
		vabdl.u8        q14, $1, $3           @ pb
		vabd.u16        q15, q12, q15           @ pc
		vcle.u16        q12, q13, q14           @ pa <= pb
		vcle.u16        q13, q13, q15           @ pa <= pc
		vcle.u16        q14, q14, q15           @ pb <= pc
		vand            q12, q12, q13           @ pa <= pb && pa <= pc
		vmovn.u16       d28, q14
		vmovn.u16       $0, q12
		vbsl            d28, $2, $3
		vbsl            $0, $1, d28
.endmacro

begin_func png_read_filter_row_paeth4_neon
        ldr             r12, [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
        vmov.i8         d20, #0
1:
        vld4.32         {d4[],d5[],d6[],d7[]},    [r1,:128]
        vld4.32         {d16[],d17[],d18[],d19[]},[r2,:128]!
        paeth           d0,  d3,  d16, d20
        vadd.u8         d0,  d0,  d4
        paeth           d1,  d0,  d17, d16
        vadd.u8         d1,  d1,  d5
        paeth           d2,  d1,  d18, d17
        vadd.u8         d2,  d2,  d6
        paeth           d3,  d2,  d19, d18
        vmov            d20, d19
        vadd.u8         d3,  d3,  d7
        vst4.32         {d0[0],d1[0],d2[0],d3[0]},[r1,:128]!
        subs            r12, r12, #16
        bgt             1b

        bx              lr

begin_func png_read_filter_row_paeth3_neon
        push            {r4,lr}
        ldr             r12, [r0, #4]           @ rowbytes
        vmov.i8         d3,  #0
        vmov.i8         d4,  #0
        mov             r0,  r1
        mov             r4,  #3
        mov             lr,  #12
        vld1.8          {q11},    [r0], lr
1:
        vld1.8          {q10},    [r2], lr
        paeth           d0,  d3,  d20, d4
        vext.8          d5,  d22, d23, #3
        vadd.u8         d0,  d0,  d22
        vext.8          d17, d20, d21, #3
        paeth           d1,  d0,  d17, d20
        vst1.32         {d0[0]},  [r1,:32], r4
        vext.8          d6,  d22, d23, #6
        vadd.u8         d1,  d1,  d5
        vext.8          d18, d20, d21, #6
        paeth           d2,  d1,  d18, d17
        vext.8          d7,  d23, d23, #1
        vld1.8          {q11},    [r0], lr
        vst1.32         {d1[0]},  [r1], r4
        vadd.u8         d2,  d2,  d6
        vext.8          d19, d21, d21, #1
        paeth           d3,  d2,  d19, d18
        vst1.32         {d2[0]},  [r1], r4
        vmov            d4,  d19
        vadd.u8         d3,  d3,  d7
        vst1.32         {d3[0]},  [r1], r4
        subs            r12, r12, #12
        bgt             1b

        pop             {r4,pc}
