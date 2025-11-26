section .rodata
    ; Eventuali costanti read-only qui.
    ; .align 16 assicura che i dati siano allineati per SSE se usi movaps


section .text
global prodScalare


; Firma C equivalente: float dot_product_sse(float *arr1, float *arr2, int n);
prodScalare:
    
    ; --- PROLOGO (Setup dello stack frame) ---
    PUSH    EBP            ; Salva il vecchio base pointer
    MOV     EBP, ESP       ; Imposta il nuovo base pointer
    PUSH    ESI
    PUSH    EDI

    ; --- MAPPING DEGLI ARGOMENTI (System V AMD64 ABI) ---
    ; Quando chiami questa funzione, i dati sono sullo stack:
    ; [ebp + 8] = Argomento 1: Puntatore array A (float*)
    ; [ebp + 12] = Argomento 2: Puntatore array B (float*)
    ; [ebp + 16] = Argomento 3: Dimensione N (int) 

    mov esi, [ebp + 8]      ;  puntatore A
    mov edi, [ebp + 12]     ;  puntatore B
    mov ecx, [ebp + 16]     ;  n 

    mov edx, ecx            ; Copia n in EDX
    shl edx, 2              ; n * 4
    
    ; --- SETUP INIZIALE ---
    ; Suggerimento: Azzera il registro accumulatore qui.
    ; Ricorda: il risultato float finale deve essere restituito in %xmm0
    XORPS XMM0, XMM0
    XOR EAX, EAX ; i = 0
    cmp edx, 0
    je fine_procedura_
ciclo_: 
    MOVAPS XMM1, [ESI + EAX] ;A[i], A[i+1], A[i+2] e A[i+3]
    MOVAPS XMM2, [EDI + EAX] ;B[i], B[i+1], B[i+2] e B[i+3]
    MULPS XMM1, XMM2
    ADDPS XMM0, XMM1 ; prod += A[I] * B[I]
    ADD EAX, 16
    CMP EAX, EDX
    JL ciclo_
    HADDPS XMM0, XMM0
    HADDPS XMM0, XMM0
fine_procedura_: 
    sub esp, 4                  
    movss [esp], xmm0           
    fld dword [esp]         
    add esp, 4                 

    pop edi                     
    pop esi
    pop ebp                    
    ret
