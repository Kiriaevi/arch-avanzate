global trovaMassimof
default rel
section .data 
  align 32
  ; maschera per il valore assoluto vettoriale SSE
  ; sarebbe 01111111 11111111 11111111 11111111 in binario
  ; AVX richiede 8 valori da 32 bit (256 bit totali)
  abs_mask: dd 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF 

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
    shr     r12, 5            ; Adattato per AVX (32 byte alla volta)
    shl     r12, 5            ; h / 8 in byte, limite per il loop vettorizzato
    vmovaps ymm5, [abs_mask]
    vxorps  ymm6, ymm6, ymm6
    vxorps  xmm2, xmm2, xmm2

loop_vettorizzato:
    cmp     rax, r12
    jge     fine_loop_vettorizzato

    vmovups ymm0, [rdi + rax]      ; current_index_row
    vmovups ymm1, [rsi + rax]      ; dQP

    ; Calcolo current_index_row - dQP
    vsubps  ymm0, ymm0, ymm1              

    ; Calcolo il valore assoluto usando la maschera
    vandps  ymm0, ymm0, ymm5       ; | curr_index_row - dQP |    
    vmaxps  ymm6, ymm6, ymm0

    add     rax, 32                
    jmp     loop_vettorizzato
fine_loop_vettorizzato: 
    ; Riduzione del massimo vettoriale a scalare
    ; Prima riduciamo da 256 bit (YMM) a 128 bit (XMM)
    vextractf128 xmm0, ymm6, 1
    vmaxps       xmm6, xmm6, xmm0  ; Massimo tra metà alta e metà bassa

    ; Ora procediamo come nel codice SSE originale
    vmovaps xmm0, xmm6
    vshufps xmm0, xmm0, xmm0, 10110001b       
    vmaxps  xmm6, xmm6, xmm0              

    vmovaps xmm0, xmm6
    vshufps xmm0, xmm0, xmm0, 01001110b       
    vmaxps  xmm6, xmm6, xmm0              

    vmovaps xmm2, xmm6
    jmp     loop_resto



loop_resto:

    cmp     rax, rdx
    jge     fine

    vmovss  xmm0, [rdi + rax]      ; current_index_row
    vmovss  xmm1, [rsi + rax]      ; dQP

    vsubss  xmm0, xmm0, xmm1              

    vandps  xmm0, xmm0, xmm5       ; | curr_index_row - dQP |    
    vmaxss  xmm2, xmm2, xmm0
    add     rax, 4                  
    jmp     loop_resto

fine:
    pop     r13      
    pop     r12
    pop     rbx
    pop     rbp      
    vmovaps xmm0, xmm2
    vzeroupper                     ; Pulizia registri YMM
    ret
