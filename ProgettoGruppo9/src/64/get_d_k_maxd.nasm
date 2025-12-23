bits 64
default rel

section .data
    align 16
    neg_ones: dq -1.0, -1.0

section .text
    global get_d_k_maxd

get_d_k_maxd:
    movapd xmm0, [neg_ones]
    
    test esi, esi
    jle .reduce
    
    mov edx, esi
    shr edx, 1
    shl edx, 1
    jz .remainder
    
    xor ecx, ecx
    
.loop_vec:
    mov rax, rcx
    shl rax, 4
    
    movupd xmm1, [rdi + rax]
    movupd xmm2, [rdi + rax + 16]
    
    shufpd xmm1, xmm2, 3
    maxpd xmm0, xmm1
    
    add ecx, 2
    cmp ecx, edx
    jl .loop_vec
    
.remainder:
    cmp ecx, esi
    jge .reduce
    
.loop_scalar:
    mov rax, rcx
    shl rax, 4
    
    movsd xmm1, [rdi + rax + 8]
    maxsd xmm0, xmm1
    
    inc ecx
    cmp ecx, esi
    jl .loop_scalar
    
.reduce:
    movapd xmm1, xmm0
    unpckhpd xmm1, xmm1
    maxsd xmm0, xmm1
    
    ret
