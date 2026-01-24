bits 64

section .data
    align 16
    neg_ones: dd -1.0, -1.0, -1.0, -1.0

section .text
    global get_d_k_maxf

; float get_d_k_max(float* KNN, int k)
; RDI = KNN, ESI = k, ritorno in XMM0
get_d_k_maxf:
    movaps xmm0, [rel neg_ones]     ; max = [-1, -1, -1, -1]
    
    test esi, esi
    jle .reduce
    
    mov edx, esi
    and edx, 0xFFFFFFFC             ; edx = k arrotondato a multiplo di 4
    jz .remainder
    
    xor ecx, ecx                    ; i = 0
    
.loop4:
    movups xmm1, [rdi + rcx*8]      ; [id0, dist0, id1, dist1]
    movups xmm2, [rdi + rcx*8 + 16] ; [id2, dist2, id3, dist3]
    shufps xmm1, xmm2, 0xDD         ; estrae [dist0, dist1, dist2, dist3]
    maxps xmm0, xmm1
    
    add ecx, 4
    cmp ecx, edx
    jl .loop4
    
.remainder:
    cmp ecx, esi
    jge .reduce
    
.loop1:
    lea eax, [ecx*2 + 1]
    movss xmm1, [rdi + rax*4]
    maxss xmm0, xmm1
    
    inc ecx
    cmp ecx, esi
    jl .loop1
    
.reduce:
    ; Riduzione orizzontale: trova il max tra i 4 elementi di xmm0
    movhlps xmm1, xmm0              ; xmm1 = [xmm0[2], xmm0[3]]
    maxps xmm0, xmm1                ; xmm0 = [max(0,2), max(1,3), ...]
    movaps xmm1, xmm0
    shufps xmm1, xmm1, 0x55         ; xmm1 = [xmm0[1], ...]
    maxss xmm0, xmm1                ; xmm0[0] = max finale
    
    ret
