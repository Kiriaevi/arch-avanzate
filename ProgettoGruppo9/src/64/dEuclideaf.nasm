
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
    shr     rdx, 4       
    shl     rdx, 4          
    xorps   xmm0, xmm0     
    xor     rax, rax        

ciclo_vett:
    cmp     rax, rdx       
    jge     fine_vett       

    movups  xmm1, [rdi + rax]
    movups  xmm2, [rsi + rax]
    subps   xmm1, xmm2
    mulps   xmm1, xmm1
    addps   xmm0, xmm1

    add     rax, 16
    jmp     ciclo_vett

fine_vett:
    haddps  xmm0, xmm0
    haddps  xmm0, xmm0


ciclo_resto:
    cmp     rax, rcx        
    jge     fine

    movss   xmm1, [rdi + rax]
    movss   xmm2, [rsi + rax]
    subss   xmm1, xmm2
    mulss   xmm1, xmm1
    addss   xmm0, xmm1      

    add     rax, 4
    jmp     ciclo_resto 
fine:
    sqrtss  xmm0, xmm0      ; Radice quadrata del risultato
    pop     rbp
    ret
