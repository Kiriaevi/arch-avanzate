section .text
global prodScalared

; double prodScalare(double *arr1, double *arr2, int n)
prodScalared:
    push    rbp
    mov     rbp, rsp

    ; Input: RDI=arr1, RSI=arr2, EDX=n

    movsxd  rcx, edx        ; RCX = n (esteso a 64 bit)
    shl     rcx, 3     
    mov     rdx, rcx    
    shr     rdx, 4       
    shl     rdx, 4          
    xorpd   xmm0, xmm0     
    xor     rax, rax        ; Indice i = 0

; --- Ciclo Vettoriale (SSE) ---
ciclo_vett:
    cmp     rax, rdx        
    jge     fine_vett       ; Se abbiamo finito i blocchi da 4, esci

    movupd  xmm1, [rdi + rax]
    movupd  xmm2, [rsi + rax]
    mulpd   xmm1, xmm2
    addpd   xmm0, xmm1

    add     rax, 16
    jmp     ciclo_vett

fine_vett:
    haddpd  xmm0, xmm0

; --- Ciclo Scalare (Resto) ---
ciclo_scalare:
    cmp     rax, rcx        ; Confronta con il LIMITE TOTALE
    jge     fine

    movsd   xmm1, [rdi + rax]
    movsd   xmm2, [rsi + rax]
    mulsd   xmm1, xmm2
    addsd   xmm0, xmm1      ; Aggiunge al totale

    add     rax, 8
    jmp     ciclo_scalare

fine:
    pop     rbp
    ret
