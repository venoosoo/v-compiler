global _start
_start:
   push rbp
   mov rbp, rsp
   sub rsp, 64
   mov rax, 10
   mov qword [rbp - 8], rax
   mov rax, 3
   mov qword [rbp - 16], rax
   mov rax, 5
   mov qword [rbp - 24], rax
   mov rax, qword [rbp - 16]
   push rax
   mov rax, qword [rbp - 24]
   mov rbx, rax
   pop rax
   add rax, rbx
   mov qword [rbp - 32], rax
   mov rax, qword [rbp - 8]
   push rax
   mov rax, qword [rbp - 16]
   push rax
   mov rax, 2
   mov rbx, rax
   pop rax
   imul rax, rbx
   mov rbx, rax
   pop rax
   add rax, rbx
   mov qword [rbp - 40], rax
   mov rax, qword [rbp - 32]
   push rax
   mov rax, qword [rbp - 40]
   mov rbx, rax
   pop rax
   add rax, rbx
   push rax
   mov rax, 1
   mov rbx, rax
   pop rax
   add rax, rbx
   mov qword [rbp - 48], rax
   mov rax, qword [rbp - 48]
   push rax
   mov rax, qword [rbp - 24]
   mov rbx, rax
   pop rax
   add rax, rbx
   push rax
   mov rax, 30
   mov rbx, rax
   pop rax
   cmp rax, rbx
   sete al
   movzx rax, al
   test rax, rax
   je .L_if_end_0
   mov rax, 2
   mov qword [rbp - 56], rax
.L_if_end_0:
   mov rax, qword [rbp - 32]
   push rax
   mov rax, 9
   mov rbx, rax
   pop rax
   cmp rax, rbx
   sete al
   movzx rax, al
   test rax, rax
   je .L_if_end_1
   mov rax, qword [rbp - 56]
   mov rdi, rax
   mov rax, 60
   syscall
.L_if_end_1:
   mov rax, 666
   mov rdi, rax
   mov rax, 60
   syscall
.L_else_end_2:
   mov rax, 60
   mov rdi, 0
   syscall
