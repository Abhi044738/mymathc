extern sin
extern cos
extern tan
extern exp
extern log
extern sqrt
extern pow
section .text
global main
movsd const_0(%rip), %xmm0
movsd const_1(%rip), %xmm1
movsd const_2(%rip), %xmm2
movsd %xmm2, %xmm0
call sin
movsd %xmm0, %xmm3
movsd const_3(%rip), %xmm4
movsd const_4(%rip), %xmm5
movsd const_5(%rip), %xmm6
movsd %xmm7, %xmm8
mulsd %xmm6, %xmm8
section .rodata
const_0: dq 3.1400000000000001
const_1: dq 2
const_2: dq 1.5700000000000001
const_3: dq 2
const_4: dq 10
const_5: dq 1024
