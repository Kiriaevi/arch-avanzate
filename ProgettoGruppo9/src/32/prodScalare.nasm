section .text
global prodScalare

; float prodScalare(float *arr1, float *arr2, int n)
prodScalare:
    push    rbp
    mov     rbp, rsp

    ; Input: RDI=arr1, RSI=arr2, EDX=n

    movsxd  rcx, edx        ; RCX = n (esteso a 64 bit)
    shl     rcx, 2     
    mov     rdx, rcx    
    shr     rdx, 4       
    shl     rdx, 4          
    xorps   xmm0, xmm0     
    xor     rax, rax        ; Indice i = 0

; --- Ciclo Vettoriale (SSE) ---
ciclo_vett:
    cmp     rax, rdx        ; Confronta con il LIMITE VETTORIALE (sicuro)
    jge     fine_vett       ; Se abbiamo finito i blocchi da 4, esci

    movups  xmm1, [rdi + rax]
    movups  xmm2, [rsi + rax]
    mulps   xmm1, xmm2
    addps   xmm0, xmm1

    add     rax, 16
    jmp     ciclo_vett

fine_vett:
    ; Riduciamo i 4 risultati parziali in 1 PRIMA di aggiungere il resto
    haddps  xmm0, xmm0
    haddps  xmm0, xmm0

; --- Ciclo Scalare (Resto) ---
; RAX riparte esattamente da dove si è fermato il ciclo vettoriale
ciclo_scalare:
    cmp     rax, rcx        ; Confronta con il LIMITE TOTALE
    jge     fine

    movss   xmm1, [rdi + rax]
    movss   xmm2, [rsi + rax]
    mulss   xmm1, xmm2
    addss   xmm0, xmm1      ; Aggiunge al totale

    add     rax, 4
    jmp     ciclo_scalare

fine:
    pop     rbp
    ret
