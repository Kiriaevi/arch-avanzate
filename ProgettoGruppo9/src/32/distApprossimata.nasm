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

  shl     r8, 2             ; n in byte, lo uso per il ciclo resto
  mov     r10, r8           
  shr     r10, 4            ; n/4 
  shl     r10, 2            ; n/4 in byte, lo uso come indice per il loop vettorizzato
  xor     r9, r9            ; i = 0
  xor     rcx, rcx          ; risultato vPlus_wPlus
  xor     rdx, rdx          ; risultato vMinus_wMinus
  xor     r11, r11          ; risultato vPlus_wMinus
  xor     r12, r12          ; risultato vMinus_wPlus
  xor     r13, r13          ; j = 0
  


vPlus_wPlus_loop_vettorizzato:
  cmp     r9, r10
  jge     vPlus_wPlus_loop_resto
  movups  xmm0, [rdi + r9]      ; vPlus 
  movups  xmm1, [rdx + r9]      ; wPlus
  pand    xmm0, xmm1            ; and bit a bit
  movq    rax,  xmm0
  movhlps xmm0, xmm0
  movq    rbx,  xmm0
  popcnt  rax, rax
  popcnt  rbx, rbx
  add     rax, rbx
  add     rcx, rax              ; ora in rcx c'è il risultato parziale
  add     r9, 16
  jmp     vPlus_wPlus_loop_vettorizzato

vPlus_wPlus_loop_resto:
  cmp     r9, r8
  jge     vMinus_wMinus_loop_vettorizzato
  mov     eax, [rdi + r9]       ; vPlus
  mov     ebx, [rdx + r9]       ; wPlus
  and     eax, ebx              ; and bit a bit
  popcnt  eax, eax
  add     rcx, rax              ; sarà il risultato finale
  add     r9, 4
  jmp     vPlus_wPlus_loop_resto

vMinus_wMinus_loop_vettorizzato:
  cmp     r13, r10
  jge     vMinus_wMinus_loop_resto
  movups  xmm0, [rsi + r9]      ; vMinus 
  movups  xmm1, [rcx + r9]      ; wMinus
  pand    xmm0, xmm1            ; and bit a bit
  movq    rax,  xmm0
  movhlps xmm0, xmm0
  movq    rbx,  xmm0
  popcnt  rax, rax
  popcnt  rbx, rbx
  add     rax, rbx
  add     rdx, rax              ; ora in rdx c'è il risultato parziale
  add     r13, 16
  jmp     vMinus_wMinus_loop_vettorizzato
 

vMinus_wMinus_loop_resto:
  xor     r9, r9           ; i = 0
  cmp     r13, r8
  jge     vPlus_wMinus_loop_vettorizzato
  mov     eax, [rsi + r9]       ; vMinus
  mov     ebx, [rcx + r9]       ; wMinus
  and     eax, ebx              ; and bit a bit
  popcnt  eax, eax
  add     rdx, rax              ; sarà il risultato finale
  add     r13, 4
  jmp     vMinus_wMinus_loop_resto

vPlus_wMinus_loop_vettorizzato:
  cmp     r9, r10
  jge     vPlus_wMinus_loop_resto
  movups  xmm0, [rdi + r9]      ; vPlus 
  movups  xmm1, [rcx + r9]      ; wMinus
  pand    xmm0, xmm1            ; and bit a bit
  movq    rax,  xmm0
  movhlps xmm0, xmm0
  movq    rbx,  xmm0
  popcnt  rax, rax
  popcnt  rbx, rbx
  add     rax, rbx
  add     r11, rax              ; ora in r11 c'è il risultato parziale
  add     r9, 16
  jmp     vPlus_wMinus_loop_vettorizzato

vPlus_wMinus_loop_resto:
  xor     r13, r13         ; j = 0
  cmp     r9, r8
  jge     vMinus_wPlus_loop_vettorizzato
  mov     eax, [rdi + r9]       ; vPlus
  mov     ebx, [rcx + r9]       ; wMinus
  and     eax, ebx              ; and bit a bit
  popcnt  eax, eax
  add     r11, rax              ; sarà il risultato finale
  add     r9, 4
  jmp     vPlus_wMinus_loop_resto

vMinus_wPlus_loop_vettorizzato:

vMinus_wPlus_loop_resto:

fine: 
  pop     rbp
  ret            
