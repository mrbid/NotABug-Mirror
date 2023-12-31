[section .text]
global munmap_and_exit
munmap_and_exit:
mov rax, 11
syscall

mov rax, 60
xor rdi, rdi
syscall
