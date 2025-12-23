
; Implementazione C di riferiment
;double dEuclidea(double *v, double *w, int D)
;{
; double distanza = 0.0f;
;  for (int i = 0; i < D; i++)
;  {
;    double diff = v[i] - w[i];
;    distanza += diff * diff;
;  }
;  return sqrtf(distanza);
;}

section .text
global dEuclidead


dEuclidead:
    push    rbp
    mov     rbp, rsp

    ; Input e registri associati: rdi=v, rsi=w, edx=D

    movsxd  rcx, edx        ; rcx = D
    shl     rcx, 3          ; un double è grande 8 byte -> shl 3          
    mov     rdx, rcx        ; rdx = D * 2
    shr     rdx, 4          ; D00, concateno due zeri, ora è multiplo di 4
    shl     rdx, 4          
    xorpd   xmm0, xmm0     
    xor     rax, rax        

ciclo_vett:
    cmp     rax, rdx       
    jge     fine_vett       

    movupd  xmm1, [rdi + rax]
    movupd  xmm2, [rsi + rax]
    subpd   xmm1, xmm2
    mulpd   xmm1, xmm1
    addpd   xmm0, xmm1

    add     rax, 16
    jmp     ciclo_vett

fine_vett:
    haddpd  xmm0, xmm0

ciclo_resto:
    cmp     rax, rcx        
    jge     fine

    movsd   xmm1, [rdi + rax]
    movsd   xmm2, [rsi + rax]
    subsd   xmm1, xmm2
    mulsd   xmm1, xmm1
    addsd   xmm0, xmm1      

    add     rax, 8
    jmp     ciclo_resto 
fine:
    sqrtsd  xmm0, xmm0      ; Radice quadrata del risultato
    pop     rbp
    ret
