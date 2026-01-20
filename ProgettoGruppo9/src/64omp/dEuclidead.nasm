section .text
global dEuclidead

; double dEuclidead_avx(double *v, double *w, int D)
; rdi = v, rsi = w, edx = D
; risultato in xmm0

dEuclidead:
    push    rbp
    mov     rbp, rsp
    ; i = 0
    xor     eax, eax
    ; numero di elementi processabili a blocchi da 4
    mov     r8d, edx
    and     r8d, -4
    ; accumulatore vettoriale
    vxorpd  ymm0, ymm0, ymm0

.ciclo_vettoriale:
    cmp     eax, r8d
    jge     .fine_vettoriale

    ; carica v[i..i+3] e w[i..i+3]
    vmovupd ymm1, [rdi + rax*8]
    vmovupd ymm2, [rsi + rax*8]
    ; (v - w)^2
    vsubpd  ymm1, ymm1, ymm2
    vmulpd  ymm1, ymm1, ymm1
    ; accumula
    vaddpd  ymm0, ymm0, ymm1
    add     eax, 4
    jmp     .ciclo_vettoriale
.fine_vettoriale:
    ; riduzione ymm0 -> somma scalare
    vextractf128 xmm1, ymm0, 1
    vaddpd       xmm0, xmm0, xmm1
    vhaddpd      xmm0, xmm0, xmm0
.ciclo_resto:
    cmp     eax, edx
    jge     .fine
    ; elementi rimanenti
    vmovsd  xmm1, [rdi + rax*8]
    vmovsd  xmm2, [rsi + rax*8]
    vsubsd  xmm1, xmm1, xmm2
    vmulsd  xmm1, xmm1, xmm1
    vaddsd  xmm0, xmm0, xmm1
    inc     eax
    jmp     .ciclo_resto
.fine:
    ; sqrt finale
    vsqrtsd xmm0, xmm0, xmm0
    ; evita penalty AVX -> SSE
    vzeroupper
    pop     rbp
    ret
