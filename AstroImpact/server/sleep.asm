[section .text]

global usleep2
global usleep2_start
global usleep2_end

usleep2:
usleep2_start: ; rdi: ptr to original microtime
lfence ; don't speculate this load please
mov rax, [rdi]
sub rax, rsi
jc usleep2_end

mov rcx, 1000
mul rcx
mov rcx, 1000000000
div rcx

mov rdi, input
mov [rdi], rax
mov [rdi+8], rdx

mov rsi, remaining

syscall:
mov rax, 35 ; sys_nanosleep
syscall

cmp rax, 0
je usleep2_end ; EINTR is the only one relevant

loop:
mov rax, [rsi]
mov [rdi], rax
mov rax, [rsi+8]
mov [rdi+8], rax
jmp syscall

usleep2_end:
ret

[section .bss]
input:
resq 2
remaining:
resq 2
