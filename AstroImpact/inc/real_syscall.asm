[section .text]
global real_syscall
real_syscall:
mov rax, [rsp+8]
mov r10, rcx
syscall
ret
