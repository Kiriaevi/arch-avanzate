global popCountAnd

section .text

; int popCountAnd(void *v, void *w, int numeroBlocchi32)
; RDI = puntatore v
; RSI = puntatore w
; RDX = numeroBlocchi32 (quanti uint32_t ci sono)
; Ritorna (RAX) il numero totale di bit a 1 nell'intersezione

popCountAnd:
    xor rax, rax            ; Azzera accumulatore risultato
    test rdx, rdx           ; Controllo se size == 0
    jz .fine

    ; Calcoliamo quanti cicli da 64 bit possiamo fare
    ; RDX contiene il numero di blocchi da 32 bit.
    ; Dividiamo per 2 per lavorare a 64 bit.
    mov rcx, rdx
    shr rcx, 1              ; rcx = rdx / 2
    
    xor r8, r8              ; Indice loop a 64 bit

.loop_64:
    cmp r8, rcx
    jge .handle_remainder

    ; Carica 64 bit (2 blocchi da 32 insieme)
    mov r9, [rdi + r8*8]
    mov r10, [rsi + r8*8]

    and r9, r10             ; AND bit a bit

    popcnt r9, r9           ; Conta i bit a 1 (istruzione SSE4.2)
    add rax, r9             ; Aggiungi al totale

    inc r8
    jmp .loop_64

.handle_remainder:
    ; Gestione dell'eventuale blocco da 32 bit rimasto dispari
    test rdx, 1             ; Se il numero di blocchi 32 era dispari
    jz .fine

    ; Carica gli ultimi 32 bit
    ; Nota: l'offset è r8*8 perché r8 contava i blocchi da 64 bit (8 byte)
    mov r9d, [rdi + r8*8]   ; mov r9d carica solo 32 bit
    mov r10d, [rsi + r8*8]
    
    and r9d, r10d
    popcnt r9d, r9d
    add rax, r9

.fine:
    ret
