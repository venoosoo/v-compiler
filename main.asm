global _start
_start:
   mov rax, 10
   push rax
   mov rax, 3
   push rax
   mov rax, 5
   push rax
   mov rax, qword [rsp + 8]
   mov rdx, 2
   mul rdx
   mov rdi, rax
   mov rcx, qword [rsp + 16]
   mov rdx, rdi
   add rcx, rdx
   mov rdi, rcx
   push rdi
   mov rax, qword [rsp + 8]
   xor rdx, rdx
   mov rcx, 2
   div rcx
   mov rdi, rax
   mov rcx, qword [rsp + 0]
   mov rdx, rdi
   sub rcx, rdx
   mov rdi, rcx
   push rdi
   push qword [rsp + 0]
   pop rdi
   mov rax, 60
   syscall
