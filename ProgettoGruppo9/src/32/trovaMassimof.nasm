global trovaMassimof
default rel
section .data 
  align 16
  ; maschera per il valore assoluto vettoriale SSE
  ; sarebbe 01111111 11111111 11111111 11111111 in binario
  abs_mask: dd 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF 
;== trovaMassimof
; ABS(current_index_row[j] - dQP[j]);
; si implementa | curr_index_row[j] - dQP[j] | e aggiornamento massimo locale
; float *current_index_row, float *dQP, int h
; input: float *current_index_row, float *dQP, int h ( dimensione dei vettori )
; output: float output, massimo valore assoluto trovato
; current_index_row -> rdi; dQP -> rsi; h -> rdx
section .text
trovaMassimof:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13

    xor     rax, rax          ; i = 0
    xor     ebx, ebx          ; local_best = 0

    shl     rdx, 2            ; h in byte, limite per il ciclo resto
    mov     r12, rdx          
    shr     r12, 4            
    shl     r12, 4            ; h / 4 in byte, limite per il loop vettorizzato
    movaps  xmm5, [abs_mask]
    xorps   xmm6, xmm6
    xorps   xmm2, xmm2

loop_vettorizzato:
    cmp     rax, r12
    jge     fine_loop_vettorizzato

    movups  xmm0, [rdi + rax]      ; current_index_row
    movups  xmm1, [rsi + rax]      ; dQP

    ; Calcolo current_index_row - dQP
    subps   xmm0, xmm1             

    ; Calcolo il valore assoluto usando la maschera
    andps   xmm0, xmm5             ; | curr_index_row - dQP |   
    maxps   xmm6, xmm0

    add     rax, 16                
    jmp     loop_vettorizzato
fine_loop_vettorizzato: 
    ; Riduzione del massimo vettoriale a scalare
    movaps  xmm0, xmm6
    shufps  xmm0, xmm0, 10110001b      
    maxps   xmm6, xmm0            

    movaps  xmm0, xmm6
    shufps  xmm0, xmm0, 01001110b      
    maxps   xmm6, xmm0            

    movaps  xmm2, xmm6
    jmp     loop_resto



loop_resto:

    cmp     rax, rdx
    jge     fine

    movss   xmm0, [rdi + rax]      ; current_index_row
    movss   xmm1, [rsi + rax]      ; dQP

    subss   xmm0, xmm1             

    andps   xmm0, xmm5             ; | curr_index_row - dQP |   
    maxss   xmm2, xmm0
    add     rax, 4                 
    jmp     loop_resto

fine:
    pop     r13      
    pop     r12
    pop     rbx
    pop     rbp      
    movaps  xmm0, xmm2
    ret              

