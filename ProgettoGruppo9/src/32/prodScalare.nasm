section .text
global prodScalare

; Firma C: float prodScalare(float *arr1, float *arr2, int n);
prodScalare:
    
    ; --- PROLOGO ---
    push    rbp
    mov     rbp, rsp

    ; --- MAPPING ARGOMENTI (System V AMD64 ABI) ---
    ; RDI = Argomento 1: arr1 (float*)
    ; RSI = Argomento 2: arr2 (float*)
    ; EDX = Argomento 3: n (int)  <-- Nota: gli int sono 32bit, usiamo la parte bassa di RDX

    ; --- CALCOLO LIMITE ---
    ; Estendiamo n (32bit) a 64bit per sicurezza matematica sui puntatori
    movsxd  rax, edx        ; Copia n in RAX con sign-extension
    shl     rax, 2          ; RAX = n * 4 (Dimensione totale in Byte)
    mov     rcx, rax        ; Spostiamo il limite in RCX per liberare RAX per l'indice

    ; --- SETUP ---
    xorps   xmm0, xmm0      ; Azzera accumulatore
    xor     rax, rax        ; i = 0 (usiamo RAX come indice a 64 bit)

    cmp     rcx, 0
    je      fine_procedura_

ciclo_:
    ; Nota: Uso MOVAPS (Aligned) come nel tuo esempio. 
    ; Funziona SOLO se i puntatori RDI e RSI sono allineati a 16 byte.
    ; Se crasha, cambia in MOVUPS.
    
    movaps  xmm1, [rdi + rax]   ; Leggi da arr1 (Base RDI + Indice RAX)
    movaps  xmm2, [rsi + rax]   ; Leggi da arr2 (Base RSI + Indice RAX)
    
    mulps   xmm1, xmm2          ; Moltiplica
    addps   xmm0, xmm1          ; Accumula

    add     rax, 16             ; Avanza indice di 16 byte
    cmp     rax, rcx            ; Confronta indice con limite
    jl      ciclo_

    ; --- RIDUZIONE ORIZZONTALE ---
    haddps  xmm0, xmm0          ; 4 -> 2 somme
    haddps  xmm0, xmm0          ; 2 -> 1 somma (Il risultato è in XMM0 basso)

fine_procedura_:
    ; --- EPILOGO ---
    ; In 64-bit, i float si ritornano in XMM0. 
    ; Non serve fare nulla con lo stack o FLD.
    
    pop     rbp
    ret
