; Rispetto all'implementazione classica, qui si utilizza il loop unrolling
; per ridurre il numero di operazioni di somma necessarie a calcolare il risultato

; L'efficacia di questa ottimizzazione è più evidente su vettori di dimensioni maggiori

section .text
global dEuclideaLU

push    rbp
    mov     rbp, rsp

    ; Input: rdi=v, rsi=w, edx=D
    
    movsxd  rcx, edx        ; RCX = D
    shl     rcx, 2          ; RCX = D * 4 (Byte totali)

    ; Calcolo limite per UNROLLING (blocchi da 64 byte = 16 float) 
    mov     rdx, rcx
    and     rdx, -64        
                           
    
    xorps   xmm0, xmm0      
    xorps   xmm1, xmm1      
    xorps   xmm2, xmm2      
    xorps   xmm3, xmm3      
    
    xor     rax, rax        


; Il ciclo processa 4 blocchi da 16 byte (4 float) per iterazione
ciclo_unrolled:
    cmp     rax, rdx
    jge     fine_unrolled

    movups  xmm4, [rdi + rax]
    movups  xmm5, [rsi + rax]
    subps   xmm4, xmm5
    mulps   xmm4, xmm4
    addps   xmm0, xmm4      

    ; Blocco 2 (Offset +16)
    movups  xmm6, [rdi + rax + 16]
    movups  xmm7, [rsi + rax + 16]
    subps   xmm6, xmm7
    mulps   xmm6, xmm6
    addps   xmm1, xmm6      

    ; Blocco 3 (Offset +32)
    movups  xmm4, [rdi + rax + 32]  
    movups  xmm5, [rsi + rax + 32]
    subps   xmm4, xmm5
    mulps   xmm4, xmm4
    addps   xmm2, xmm4      

    ; Blocco 4 (Offset +48) 
    movups  xmm6, [rdi + rax + 48]  
    movups  xmm7, [rsi + rax + 48]
    subps   xmm6, xmm7
    mulps   xmm6, xmm6
    addps   xmm3, xmm6      

    add     rax, 64         ; Avanzo di 16x4 (64) byte
    jmp     ciclo_unrolled

fine_unrolled:
    addps   xmm0, xmm1
    addps   xmm0, xmm2
    addps   xmm0, xmm3


    mov     rdx, rcx
    and     rdx, -16        ; Arrotonda ai 16 byte


ciclo_vett_rimanenti:
    cmp     rax, rdx
    jge     fine_vett_rimanenti

    movups  xmm4, [rdi + rax]
    movups  xmm5, [rsi + rax]
    subps   xmm4, xmm5
    mulps   xmm4, xmm4
    addps   xmm0, xmm4

    add     rax, 16
    jmp     ciclo_vett_rimanenti

fine_vett_rimanenti:
    haddps  xmm0, xmm0
    haddps  xmm0, xmm0

ciclo_resto:
    cmp     rax, rcx
    jge     fine_resto

    movss   xmm4, [rdi + rax]
    movss   xmm5, [rsi + rax]
    subss   xmm4, xmm5
    mulss   xmm4, xmm4
    addss   xmm0, xmm4

    add     rax, 4
    jmp     ciclo_resto

fine_resto:
    sqrtss  xmm0, xmm0
    pop     rbp
    ret
