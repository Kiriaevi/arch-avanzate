global trovaMassimod
default rel
section .data 
  align 16
  ; maschera per il valore assoluto vettoriale SSE
  ; sarebbe 0111111111111111 1111111111111111 in binario
  abs_mask: dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF
;== trovaMassimod
; ABS(current_index_row[j] - dQP[j]);
; si implementa | curr_index_row[j] - dQP[j] | e aggiornamento massimo locale
; double *current_index_row, double *dQP, int h
; input: double *current_index_row, double *dQP, int h ( dimensione dei vettori )
; output: double output, massimo valore assoluto trovato
; current_index_row -> rdi; dQP -> rsi; h -> rdx
section .text
trovaMassimod:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    xor     rax, rax          ; i = 0
    xor     ebx, ebx          ; local_best = 0

    shl     rdx, 3            ; h in byte, limite per il ciclo resto
    mov     r12, rdx          
    shr     r12, 4            ; h / 16
    shl     r12, 4            ; h / 16 in byte, limite per il loop vettorizzato
    movaps  xmm5, [abs_mask]
    xorps   xmm6, xmm6
    xorps   xmm2, xmm2

loop_vettorizzato:
    cmp     rax, r12
    jge     fine_loop_vettorizzato

    movupd  xmm0, [rdi + rax]      ; current_index_row
    movupd  xmm1, [rsi + rax]      ; dQP

    ; Calcolo current_index_row - dQP
    subpd   xmm0, xmm1             

    ; Calcolo il valore assoluto usando la maschera
    andpd   xmm0, xmm5             ; | curr_index_row - dQP |   
    maxpd   xmm6, xmm0

    add     rax, 16                
    jmp     loop_vettorizzato
fine_loop_vettorizzato: 
    ; Riduzione del massimo vettoriale a scalare
    movapd  xmm0, xmm6
    shufpd  xmm0, xmm0, 1
    maxpd   xmm6, xmm0            

    movapd  xmm2, xmm6
    jmp     loop_resto



loop_resto:

    cmp     rax, rdx
    jge     fine

    movsd   xmm0, [rdi + rax]      ; current_index_row
    movsd   xmm1, [rsi + rax]      ; dQP

    subsd   xmm0, xmm1             

    andpd   xmm0, xmm5             ; | curr_index_row - dQP |   
    maxsd   xmm2, xmm0
    add     rax, 8                 
    jmp     loop_resto

fine:
    pop     r13      
    pop     r12
    pop     rbx
    pop     rbp      
    movapd  xmm0, xmm2
    ret              

