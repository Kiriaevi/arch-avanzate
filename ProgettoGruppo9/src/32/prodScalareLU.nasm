section .text
global prodScalare_opt

prodScalare_opt:
    push    rbp
    mov     rbp, rsp

    ; Input: RDI=arr1, RSI=arr2, EDX=n

    movsxd  rcx, edx        ; RCX = n
    shl     rcx, 2          ; RCX = n * 4 (Byte totali)

    ; Calcolo limite per UNROLLING (blocchi da 64 byte = 16 float) ---
    mov     rdx, rcx
    and     rdx, -64        ; Arrotonda per difetto al multiplo di 64
                            ; (64 byte = 4 registri XMM * 16 byte)

    xorps   xmm0, xmm0      
    xorps   xmm1, xmm1      
    xorps   xmm2, xmm2      
    xorps   xmm3, xmm3      

    xor     rax, rax        

ciclo_unrolled:
    cmp     rax, rdx
    jge     fine_unrolled

    movups  xmm4, [rdi + rax]       
    movups  xmm5, [rsi + rax]       
    mulps   xmm4, xmm5              
    addps   xmm0, xmm4              

    ; Blocco 2 (Offset +16) 
    movups  xmm6, [rdi + rax + 16]
    movups  xmm7, [rsi + rax + 16]
    mulps   xmm6, xmm7
    addps   xmm1, xmm6              

    ; Blocco 3 (Offset +32) 
    movups  xmm4, [rdi + rax + 32]  
    movups  xmm5, [rsi + rax + 32]
    mulps   xmm4, xmm5
    addps   xmm2, xmm4              

    ; Blocco 4 (Offset +48) 
    movups  xmm6, [rdi + rax + 48]  
    movups  xmm7, [rsi + rax + 48]
    mulps   xmm6, xmm7
    addps   xmm3, xmm6              

    add     rax, 64                 ; Avanza di 64 byte
    jmp     ciclo_unrolled

fine_unrolled:
    addps   xmm0, xmm1      
    addps   xmm2, xmm3      
    addps   xmm0, xmm2      

    mov     rdx, rcx
    and     rdx, -16        ; Arrotonda ai 16 byte per processare il resto "medio"

ciclo_vett_rimanenti:
    cmp     rax, rdx
    jge     fine_vett_clean

    movups  xmm4, [rdi + rax]
    movups  xmm5, [rsi + rax]
    mulps   xmm4, xmm5
    addps   xmm0, xmm4

    add     rax, 16
    jmp     ciclo_vett_rimanenti

fine_vett_rimanenti:
    haddps  xmm0, xmm0      
    haddps  xmm0, xmm0      


ciclo_resto:
    cmp     rax, rcx
    jge     fine_resto

    movss   xmm4, [rdi + rax]
    movss   xmm5, [rsi + rax]
    mulss   xmm4, xmm5
    addss   xmm0, xmm4

    add     rax, 4
    jmp     ciclo_resto

fine_resto:
    pop     rbp
    ret