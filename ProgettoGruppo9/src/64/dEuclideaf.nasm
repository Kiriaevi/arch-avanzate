
; Implementazione C di riferiment
;float dEuclidea(float *v, float *w, int D)
;{
; float distanza = 0.0f;
;  for (int i = 0; i < D; i++)
;  {
;    float diff = v[i] - w[i];
;    distanza += diff * diff;
;  }
;  return sqrtf(distanza);
;}

section .text
global dEuclideaf


dEuclideaf:
    push    rbp
    mov     rbp, rsp

    ; Input e registri associati: rdi=v, rsi=w, edx=D

    movsxd  rcx, edx        
    shl     rcx, 2      
    mov     rdx, rcx    
    shr     rdx, 5          
    shl     rdx, 5          
    vxorps  ymm0, ymm0, ymm0      
    xor     rax, rax        

ciclo_vett:
    cmp     rax, rdx        
    jge     fine_vett        

    vmovups ymm1, [rdi + rax]
    vmovups ymm2, [rsi + rax]
    vsubps  ymm1, ymm1, ymm2
    vmulps  ymm1, ymm1, ymm1
    vaddps  ymm0, ymm0, ymm1

    add     rax, 32
    jmp     ciclo_vett

fine_vett:
    ; Riduzione da 256 bit a 128 bit
    vextractf128 xmm1, ymm0, 1
    vaddps       xmm0, xmm0, xmm1

    ; Riduzione finale sui 128 bit
    vhaddps xmm0, xmm0, xmm0
    vhaddps xmm0, xmm0, xmm0


ciclo_resto:
    cmp     rax, rcx        
    jge     fine

    vmovss  xmm1, [rdi + rax]
    vmovss  xmm2, [rsi + rax]
    vsubss  xmm1, xmm1, xmm2
    vmulss  xmm1, xmm1, xmm1
    vaddss  xmm0, xmm0, xmm1      

    add     rax, 4
    jmp     ciclo_resto 
fine:
    vsqrtss xmm0, xmm0, xmm0 ; Radice quadrata del risultato
    vzeroupper
    pop     rbp
    ret
