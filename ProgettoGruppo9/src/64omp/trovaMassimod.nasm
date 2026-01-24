global trovaMassimod
default rel

section .data
    align 32
    ; Maschera per il valore assoluto dei double
    ; 0x7FFF...FFF = bit di segno a 0, tutti gli altri a 1
    ; Serve per calcolare |x| azzerando il bit di segno
abs_mask_d:
    dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF
    dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF

section .text
trovaMassimod:
    mov edx, edx

    push rbp
    mov  rbp, rsp
    push rbx
    push r12

    ; rax usato come offset in byte all'interno dei vettori
    xor  rax, rax          ; offset iniziale = 0

    ; Converte h (numero di double) in numero di byte
    shl  rdx, 3            
    ; Calcolo del limite per il loop vettorizzato:
    ; floor(total_bytes / 32) * 32
    ; (32 byte = 4 double = 1 registro YMM)
    mov  r12, rdx
    shr  r12, 5
    shl  r12, 5            ; r12 = limite vettoriale in byte

    ; Carica la maschera per il valore assoluto
    vmovapd ymm5, [abs_mask_d]

    ; Inizializza il vettore dei massimi locali a 0.0
    vxorpd  ymm6, ymm6, ymm6

.loop_vec:
    cmp  rax, r12
    jge  .end_vec          ; esce se abbiamo processato tutti i blocchi AVX

    ; Carica 4 double da current_index_row e dQP
    vmovupd ymm0, [rdi + rax]
    vmovupd ymm1, [rsi + rax]

    ; Calcolo differenza: current_index_row - dQP
    vsubpd  ymm0, ymm0, ymm1

    ; Calcolo valore assoluto: |diff|
    vandpd  ymm0, ymm0, ymm5

    ; Aggiornamento massimo vettoriale
    vmaxpd  ymm6, ymm6, ymm0

    ; Avanza di 4 double (32 byte)
    add  rax, 32
    jmp  .loop_vec

.end_vec:
    ; Estrae la metà alta del registro YMM (elementi 2 e 3)
    vextractf128 xmm0, ymm6, 1

    ; Confronta metà bassa (d0,d1) con metà alta (d2,d3)
    vmaxpd       xmm6, xmm6, xmm0

    ; Ora in xmm6 ci sono due double:
    ; xmm6[0] = max(d0, d2)
    ; xmm6[1] = max(d1, d3)

    ; Shuffle per confrontare i due double rimasti
    vshufpd  xmm0, xmm6, xmm6, 1
    vmaxsd   xmm2, xmm6, xmm0
    ; xmm2 contiene il massimo dei 4 valori AVX

.loop_tail:
    cmp  rax, rdx
    jge  .done             ; termina se non restano elementi

    vmovsd xmm0, [rdi + rax]
    vmovsd xmm1, [rsi + rax]

    vsubsd xmm0, xmm0, xmm1

    vandpd xmm0, xmm0, [abs_mask_d]

    vmaxsd xmm2, xmm2, xmm0

    add  rax, 8
    jmp  .loop_tail

.done:
    vmovapd xmm0, xmm2

    vzeroupper

    pop  r12
    pop  rbx
    pop  rbp
    ret
