section .text
global distanzaPreQ


distanzaPreQ:
    ; xmm0 = accumulatore totale
    xorps xmm0, xmm0

    ; Copie dei puntatori
    mov r9, rdi    ; vPlus
    mov r10, rsi   ; vMinus
    mov r11, rdx   ; wPlus
    mov r12, rcx   ; wMinus

    mov eax, r8d
    shr eax, 2         ; iterazioni SIMD = D/4
    jz .hsum

.loop:
    ; Caricamenti
    movaps xmm1, [r9]      ; vPlus
    movaps xmm2, [r10]     ; vMinus
    movaps xmm3, [r11]     ; wPlus
    movaps xmm4, [r12]     ; wMinus

    ; ===== DOT1: vPlus · wPlus =====
    movaps xmm5, xmm1
    mulps xmm5, xmm3
    addps xmm0, xmm5

    ; ===== DOT2: vMinus · wMinus =====
    movaps xmm5, xmm2
    mulps xmm5, xmm4
    addps xmm0, xmm5

    ; ===== DOT3: vPlus · wMinus ===== (da sottrarre)
    movaps xmm5, xmm1
    mulps xmm5, xmm4
    subps xmm0, xmm5

    ; ===== DOT4: vMinus · wPlus ===== (da sottrarre)
    movaps xmm5, xmm2
    mulps xmm5, xmm3
    subps xmm0, xmm5

    ; Avanza i puntatori
    add r9, 16
    add r10, 16
    add r11, 16
    add r12, 16

    dec eax
    jnz .loop

.hsum:
    ; Somma orizzontale (SSE3)
    haddps xmm0, xmm0
    haddps xmm0, xmm0
    ret
