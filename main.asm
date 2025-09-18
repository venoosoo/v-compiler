global _start
_start:
   mov rax, 69
   push rax
   mov rax, 5
   push rax
   push qword [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   mov rdi, 0
   syscall
