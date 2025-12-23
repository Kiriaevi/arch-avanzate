bits 64
default rel

section .data
    align 32
    neg_ones: dq -1.0, -1.0, -1.0, -1.0

section .text
    global get_d_k_maxd

get_d_k_maxd:
    vmovapd ymm0, [neg_ones]
    
    test esi, esi
    jle .red
    
    mov edx, esi
    shr edx, 2          
    shl edx, 2          
    jz .resto
    
    xor ecx, ecx

.loop_vett:
    mov rax, rcx
    shl rax, 4          
    
    vmovupd ymm1, [rdi + rax]       
    vmovupd ymm2, [rdi + rax + 32]  
    
    vshufpd ymm1, ymm1, ymm2, 0b1111
    vmaxpd ymm0, ymm0, ymm1
    
    add ecx, 4
    cmp ecx, edx
    jl .loop_vett

.resto:
    cmp ecx, esi
    jge .red

.loop_resto:
    mov rax, rcx
    shl rax, 4
    
    vmovsd xmm1, [rdi + rax + 8]
    vmaxsd xmm0, xmm0, xmm1
    
    inc ecx
    cmp ecx, esi
    jl .loop_resto


.red:
    vextractf128 xmm1, ymm0, 1     
    vmaxpd xmm0, xmm0, xmm1         
    vunpckhpd xmm1, xmm0, xmm0      
    vmaxsd xmm0, xmm0, xmm1         
    
    ret
