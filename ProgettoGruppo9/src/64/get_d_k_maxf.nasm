bits 64

section .data
    align 32
    neg_ones: dd -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0

section .text
    global get_d_k_maxf

; float get_d_k_max(float* KNN, int k)
; RDI = KNN, ESI = k, ritorno in XMM0
get_d_k_maxf:
    vmovaps ymm0, [rel neg_ones]    ; max = [-1, -1, -1, -1, ...]
    
    test esi, esi
    jle .reduce
    
    mov edx, esi
    shr edx, 3                      
    shl edx, 3                      ; edx = k arrotondato a multiplo di 8
    jz .remainder
    
    xor ecx, ecx                    ; i = 0
    
.loop8:
    ; KNN: [id0, dist0, id1, dist1, id2, dist2, id3, dist3, ...]
    vmovups ymm1, [rdi + rcx*8]     ; [id0, dist0, id1, dist1, id2, dist2, id3, dist3]
    vmovups ymm2, [rdi + rcx*8 + 32]; [id4, dist4, id5, dist5, id6, dist6, id7, dist7]
    vshufps ymm1, ymm1, ymm2, 0xDD  ; estrae [dist0, dist1, dist4, dist5 | dist2, dist3, dist6, dist7]
    vmaxps ymm0, ymm0, ymm1
    
    add ecx, 8
    cmp ecx, edx
    jl .loop8
    
.remainder:
    cmp ecx, esi
    jge .reduce
    
.loop1:
    lea eax, [ecx*2 + 1]
    vmovss xmm1, [rdi + rax*4]
    vmaxss xmm0, xmm0, xmm1
    
    inc ecx
    cmp ecx, esi
    jl .loop1
    
.reduce:
    vextractf128 xmm1, ymm0, 1
    vmaxps xmm0, xmm0, xmm1

    ; Riduzione orizzontale: trova il max tra i 4 elementi di xmm0
    vmovhlps xmm1, xmm0, xmm0       ; xmm1 = [xmm0[2], xmm0[3]]
    vmaxps xmm0, xmm0, xmm1         ; xmm0 = [max(0,2), max(1,3), ...]
    vmovaps xmm1, xmm0
    vshufps xmm1, xmm1, xmm1, 0x55  ; xmm1 = [xmm0[1], ...]
    vmaxss xmm0, xmm0, xmm1         ; xmm0[0] = max finale
    
    vzeroupper
    ret
