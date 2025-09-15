global _start
_start:
   mov rax, 69
   push rax
   push qword [rsp + 4]
   push qword [rsp + 8]
   mov rax, 60
   pop rdi
   syscall
   mov rdi, 0
   syscall
