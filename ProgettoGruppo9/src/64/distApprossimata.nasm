section .text
global distApprossimata

;== distApprossimata
; si implementa la formula (v+ ◦ w+) + (v− ◦ w−) − (v+ ◦ w−) − (v− ◦ w+)
; calcolare un prodotto scalare significa fare l'and bit a bit di 
; ogni elemento e terminare con un popcnt sui risultati. 
; input: uint32_t *vPlus, uint32_t *vMinus, uint32_t *wPlus, uint32_t *wMinus, int length
; output: int output, risultato della formula per il prodotto scalare
; vPlus -> rdi; vMinus -> rsi; wPlus -> rdx; wMinus -> rcx; length -> r8

distApprossimata:
    push    rbp
    mov     rbp, rsp
    push    rbx             

    shl     r8, 3           ; n in byte, lo uso per il ciclo resto
    mov     r10, r8            
    shr     r10, 5          ; n/32 
    shl     r10, 5          ; n/32 * 32, lo uso come limite per il loop vettorizzato
    
    xor     r9, r9          ; i = 0
    xor     r11, r11        

loop_vettorizzato:
    cmp     r9, r10
    jge     loop_resto
    
    ; Carico 4 elementi da ogni vettore e faccio delle copie per evitare la sovrascrittura
    vmovupd  ymm0, [rdi + r9]      ; vPlus 
    vmovupd  ymm1, [rsi + r9]      ; vMinus
    vmovupd  ymm2, [rdx + r9]      ; wPlus
    vmovupd  ymm3, [rcx + r9]      ; wMinus 

    ; -- 1. (v+ ◦ w+) -> Somma --
    vmovaps  xmm4, xmm0            ; copia vPlus
    vpand    xmm4, xmm2            ; vPlus ◦ wPlus
    
    movq    rax,  xmm4
    movhlps xmm4, xmm4
    movq    rbx,  xmm4
    popcnt  rax, rax
    popcnt  rbx, rbx
    add     r11, rax
    add     r11, rbx              ; sommo all'accumulatore r11

    ; -- 2. (v- ◦ w-) -> Somma --
    movaps  xmm5, xmm1            ; copia vMinus
    pand    xmm5, xmm3            ; vMinus ◦ wMinus

    movq    rax,  xmm5
    movhlps xmm5, xmm5
    movq    rbx,  xmm5
    popcnt  rax, rax
    popcnt  rbx, rbx
    add     r11, rax
    add     r11, rbx

    ; -- 3. (v+ ◦ w-) -> Sottrai --
    ; Riusiamo xmm0 (vPlus) direttamente
    pand    xmm0, xmm3            ; vPlus ◦ wMinus

    movq    rax,  xmm0
    movhlps xmm0, xmm0
    movq    rbx,  xmm0
    popcnt  rax, rax
    popcnt  rbx, rbx
    sub     r11, rax              ; sottraggo dall'accumulatore
    sub     r11, rbx

    ; -- 4. (v- ◦ w+) -> Sottrai --
    ; Riusiamo xmm1 (vMinus) direttamente
    pand    xmm1, xmm2            ; vMinus ◦ wPlus

    movq    rax,  xmm1
    movhlps xmm1, xmm1
    movq    rbx,  xmm1
    popcnt  rax, rax
    popcnt  rbx, rbx
    sub     r11, rax
    sub     r11, rbx
  
    add     r9, 16
    jmp     loop_vettorizzato

loop_resto:
    cmp     r9, r8
    jge     fine
    
    ; Carico 1 elemento alla volta e uso EAX per calcoli rapidi
    
    ; 1. vPlus ◦ wPlus (Somma)
    mov     eax, [rdi + r9]       ; vPlus 
    and     eax, [rdx + r9]       ; wPlus
    popcnt  eax, eax
    add     r11, rax

    ; 2. vMinus ◦ wMinus (Somma)
    mov     eax, [rsi + r9]       ; vMinus
    and     eax, [rcx + r9]       ; wMinus
    popcnt  eax, eax
    add     r11, rax

    ; 3. vPlus ◦ wMinus (Sottrai)
    mov     eax, [rdi + r9]       ; vPlus
    and     eax, [rcx + r9]       ; wMinus
    popcnt  eax, eax
    sub     r11, rax

    ; 4. vMinus ◦ wPlus (Sottrai)
    mov     eax, [rsi + r9]       ; vMinus
    and     eax, [rdx + r9]       ; wPlus
    popcnt  eax, eax
    sub     r11, rax

    add     r9, 4
    jmp     loop_resto

fine: 
    xor     rax, rax
    mov     rax, r11       
    pop     rbx             
    pop     rbp
    ret
