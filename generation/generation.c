
#include "./generation.h"
#include "../libs/sds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "./helper/helper.h"


void handle_vars(gen_data* g, const NodeStmt* stmt) {
    char* s = "";
    int int_char_check = 0;
    switch (stmt->kind) {
        case NODE_STMT_INT: 
            s = strdup(stmt->as.int_.ident.value); 
            if (stmt->as.int_.expr.kind == NODE_EXPR_CHAR) {
                printf("error: cannot assign value of type 'char' to variable of type 'int'\n");
                exit(1);
            }
            break;
        case NODE_STMT_SHORT: 
            s = strdup(stmt->as.short_.ident.value); 
            if (stmt->as.short_.expr.kind == NODE_EXPR_CHAR) {
                printf("error: cannot assign value of type 'char' to variable of type 'int'\n");
                exit(1);
            }
            break;
        case NODE_STMT_LONG: 
            s = strdup(stmt->as.long_.ident.value); 
            if (stmt->as.long_.expr.kind == NODE_EXPR_CHAR) {
                printf("error: cannot assign value of type 'char' to variable of type 'int'\n");
                exit(1);
            }
            break;
        case NODE_STMT_CHAR: s = strdup(stmt->as.char_.ident.value); break;
    }
    // pushing the var for block visibility
    if (kv_size(*g->m_block) > 0) {
        size_t last_row_index = kv_size(*g->m_block) - 1;
        StrVec *last_row = &kv_A(*g->m_block, last_row_index);
        if (!s || s == "") { perror("strdup"); exit(1); }
        kv_push(char*, *last_row, s); 
    }

    switch(stmt->kind) {
        case NODE_STMT_SHORT: {
            gen_expr_to_rax(g,&stmt->as.short_.expr, stmt);
            int slot = lookup_var_slot(g, stmt->as.short_.ident.value);
            int off = slot_to_offset(g,slot);
            emit(g, "   mov word [rbp - %d], ax\n", off);
        }
        case NODE_STMT_LONG: {
            gen_expr_to_rax(g,&stmt->as.long_.expr, stmt);
            int slot = lookup_var_slot(g, stmt->as.long_.ident.value);
            int off = slot_to_offset(g,slot);
            emit(g, "   mov qword [rbp - %d], rax\n", off);
        }
        case NODE_STMT_INT: {
            gen_expr_to_rax(g, &stmt->as.int_.expr, stmt);
            int slot = lookup_var_slot(g, stmt->as.int_.ident.value);
            int off = slot_to_offset(g,slot);
            emit(g, "   mov dword [rbp - %d], eax\n", off);
            return;
        }
        case NODE_STMT_CHAR: {
            gen_expr_to_rax(g, &stmt->as.char_.expr, stmt);
            int slot = lookup_var_slot(g, stmt->as.char_.ident.value);
            int off = slot_to_offset(g,slot);
            emit(g, "   mov byte [rbp - %d], al\n", off);
            return;
        }
    }
}

void gen_stmt(gen_data* g, const NodeStmt* stmt) {
    if (!stmt) return;


    if (stmt->kind == NODE_STMT_RETURN) {
        gen_expr_to_rax(g, &stmt->as.return_.res,stmt);
        return;
    }

    if (stmt->kind == NODE_STMT_SHORT) {
        handle_vars(g,stmt);
        return;
    } 
    if (stmt->kind == NODE_STMT_LONG) {
        handle_vars(g,stmt);
        return;
    } 

    if (stmt->kind == NODE_STMT_INT) {
        handle_vars(g,stmt);
        return;
    } 
    if (stmt->kind == NODE_STMT_CHAR) {
        handle_vars(g,stmt);
        return;
    }

    if (stmt->kind == NODE_STMT_FUNC) {
        int id = next_label();
        emit(g,"   jmp _placeholder%d\n",id);
        emit(g,"_%s:\n", stmt->as.func.name.value);
        emit(g,"   push rbp\n");
        emit(g,"   mov rbp, rsp\n");



        for (int i = 0; i < kv_size(stmt->as.func.types); i++) {
            int type_num = kv_A(stmt->as.func.types, i).pair[0].type;
            char *name = kv_A(stmt->as.func.types, i).pair[1].value;

            ensure_var_slot(g, name,type_num);

            // adding arg type so when using function we can acess it by name
            args_func arg;
            arg.name = name;
            kv_init(arg.arg_types);
            kv_push(int, arg.arg_types, type_num);
            kv_push(args_func, *g->m_func, arg);

            size_t arg_size;
            const char *reg_part = NULL;

            switch ((TokenType)type_num) {
                case token_type_char_t:  arg_size = 1; reg_part = "al"; break;
                case token_type_short:   arg_size = 2; reg_part = "ax"; break;
                case token_type_int:     arg_size = 4; reg_part = "eax"; break;
                case token_type_long:    arg_size = 8; reg_part = "rax"; break;
                default:                 arg_size = 8; reg_part = "rax"; break;
            }


            // looks bad but there no other way
            const char *src_reg = NULL;
            switch(i) {
                case 0:
                    switch(arg_size) {
                        case 1: src_reg = "dil"; break;
                        case 2: src_reg = "di";  break;
                        case 4: src_reg = "edi"; break;
                        case 8: src_reg = "rdi"; break;
                    }
                    break;
                case 1:
                    switch(arg_size) {
                        case 1: src_reg = "sil"; break;
                        case 2: src_reg = "si";  break;
                        case 4: src_reg = "esi"; break;
                        case 8: src_reg = "rsi"; break;
                    }
                    break;
                case 2:
                    switch(arg_size) {
                        case 1: src_reg = "dl"; break;
                        case 2: src_reg = "dx"; break;
                        case 4: src_reg = "edx"; break;
                        case 8: src_reg = "rdx"; break;
                    }
                    break;
                case 3:
                    switch(arg_size) {
                        case 1: src_reg = "cl"; break;
                        case 2: src_reg = "cx"; break;
                        case 4: src_reg = "ecx"; break;
                        case 8: src_reg = "rcx"; break;
                    }
                    break;
                case 4:
                    switch(arg_size) {
                        case 1: src_reg = "r8b"; break;
                        case 2: src_reg = "r8w"; break;
                        case 4: src_reg = "r8d"; break;
                        case 8: src_reg = "r8";  break;
                    }
                    break;
                case 5:
                    switch(arg_size) {
                        case 1: src_reg = "r9b"; break;
                        case 2: src_reg = "r9w"; break;
                        case 4: src_reg = "r9d"; break;
                        case 8: src_reg = "r9";  break;
                    }
                    break;
            }

            emit(g, "   mov %s, %s\n", reg_part, src_reg);

            int push_size = (arg_size + 15) & ~15;
            emit(g, "   sub rsp, %zu\n", push_size);

            if (arg_size == 1)
                emit(g, "   mov byte [rsp], %s\n", reg_part);
            else if (arg_size == 2)
                emit(g, "   mov word [rsp], %s\n", reg_part);
            else if (arg_size == 4)
                emit(g, "   mov dword [rsp], %s\n", reg_part);
            else // 8 bytes
                emit(g, "   mov qword [rsp], %s\n", reg_part);
        }
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.func.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.func.body, i));
        }
        emit(g, "   leave\n");
        emit(g, "   ret\n");
        emit(g,"_placeholder%d:\n", id);
        delete_local_var(g);
        remove_last_block(g);
        return;
    }



    if (stmt->kind == NODE_STMT_VCHANGE) {
        gen_expr_to_rax(g, &stmt->as.vchange.expr, stmt);
        int slot = lookup_var_slot(g, stmt->as.vchange.ident.value);
        int off = slot_to_offset(g,slot);
        int type = get_type_by_name(g,stmt->as.vchange.ident.value);
        emit_ident_to_move(g, off, type);
        return;
    }


    if (stmt->kind == NODE_STMT_FUNC_USE) {
        for (int i = 0; i < kv_size(*g->m_func); i++) {
            if (kv_A(*g->m_func, i).name == stmt->as.func_call.name.value) {
                for (int j = 0; j < kv_size(stmt->as.func_call.args); j++) {
                    args_func arg = kv_A(*g->m_func, i);
                    if (kv_A(arg.arg_types, j) != kv_A(stmt->as.func_call.args, j).type) {
                        printf("when calling function types are different");
                        exit(1);
                    }
                }
            }
        }
        size_t arg_size;
        const char *reg_part = NULL;
        for (int i = 0; i < kv_size(stmt->as.func_call.args); i ++) {
            switch (kv_A(stmt->as.func_call.args, i).type) {
                case token_type_char_v:  arg_size = 1; break;
                case token_type_short:   arg_size = 2; break;
                case token_type_int_lit: arg_size = 4; break;
                case token_type_long:    arg_size = 8; break;
                default:                 arg_size = 8; printf("the type is: %d",kv_A(stmt->as.func_call.args, i).type); break;
            }


            // looks bad but there no other way
            const char *src_reg = NULL;
            switch(i) {
                case 0:
                    switch(arg_size) {
                        case 1: src_reg = "dil"; break;
                        case 2: src_reg = "di";  break;
                        case 4: src_reg = "edi"; break;
                        case 8: src_reg = "rdi"; break;
                    }
                    break;
                case 1:
                    switch(arg_size) {
                        case 1: src_reg = "sil"; break;
                        case 2: src_reg = "si";  break;
                        case 4: src_reg = "esi"; break;
                        case 8: src_reg = "rsi"; break;
                    }
                    break;
                case 2:
                    switch(arg_size) {
                        case 1: src_reg = "dl"; break;
                        case 2: src_reg = "dx"; break;
                        case 4: src_reg = "edx"; break;
                        case 8: src_reg = "rdx"; break;
                    }
                    break;
                case 3:
                    switch(arg_size) {
                        case 1: src_reg = "cl"; break;
                        case 2: src_reg = "cx"; break;
                        case 4: src_reg = "ecx"; break;
                        case 8: src_reg = "rcx"; break;
                    }
                    break;
                case 4:
                    switch(arg_size) {
                        case 1: src_reg = "r8b"; break;
                        case 2: src_reg = "r8w"; break;
                        case 4: src_reg = "r8d"; break;
                        case 8: src_reg = "r8";  break;
                    }
                    break;
                case 5:
                    switch(arg_size) {
                        case 1: src_reg = "r9b"; break;
                        case 2: src_reg = "r9w"; break;
                        case 4: src_reg = "r9d"; break;
                        case 8: src_reg = "r9";  break;
                    }
                    break;
            }
            emit(g, "   mov %s, %d\n", src_reg,atoi(kv_A(stmt->as.func_call.args, i).value));
        }
        emit(g, "   sub rsp,8\n");
        emit(g, "   call _%s\n",stmt->as.func_call.name.value);
        emit(g, "   add rsp,8\n");
        return;
    }

    if (stmt->kind == NODE_STMT_EXIT) {
        gen_expr_to_rax(g, &stmt->as.exit_.expr, stmt);
        emit(g, "   mov rdi, rax\n");
        emit(g, "   mov rax, 60\n");
        emit(g, "   syscall\n");
        return;
    }

    if (stmt->kind == NODE_STMT_IF) {
        // evaluate condition -> rax ; test ; je skip
        gen_expr_to_rax(g, &stmt->as.if_.cond, stmt);
        emit(g, "   test rax, rax\n");
        int id = next_label();
        emit(g, "   je .L_if_end_%d\n", id);
        // body
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.if_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.if_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);
        emit(g, ".L_if_end_%d:\n", id);
        return;
    }
    if (stmt->kind == NODE_STMT_ELSE) {
        int id = next_label();
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.else_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.else_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);
        emit(g, ".L_else_end_%d:\n", id);
        return;
    }
    if (stmt->kind == NODE_STMT_WHILE) {
        int id = next_label();
        emit(g, ".L_While_start_%d:\n", id);
        gen_expr_to_rax(g, &stmt->as.while_.cond, stmt);
        emit(g, "   mov r10, rax\n");
        emit(g, "   test r10, r10\n");
        emit(g, "   je .L_While_end_%d\n", id);
        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.while_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.while_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);

        // Jump back to start
        emit(g, "   jmp .L_While_start_%d\n", id);

        // End label
        emit(g, ".L_While_end_%d:\n", id);

        return;
    }
    if (stmt->kind == NODE_STMT_FOR) {
        int id = next_label();

        // --- init ---
        gen_stmt(g, stmt->as.for_.cond1); // e.g., i = 0

        emit(g, ".L_For_start_%d:\n", id);

        gen_expr_to_rax(g, &stmt->as.for_.cond2, stmt);  

        emit(g, "   cmp rax, 0\n");        // compare to 0
        emit(g, "   je .L_For_end_%d\n", id);  // exit if false

        StrVec row;
        kv_init(row);
        kv_push(StrVec, *g->m_block, row);
        for (size_t i = 0; i < kv_size(stmt->as.for_.body); ++i) {
            gen_stmt(g, &kv_A(stmt->as.for_.body, i));
        }
        delete_local_var(g);
        remove_last_block(g);

    
        gen_stmt(g, stmt->as.for_.cond3);

        // --- jump back ---
        emit(g, "   jmp .L_For_start_%d\n", id);

        // --- end label ---
        emit(g, ".L_For_end_%d:\n", id);
        return;
    }

    printf("unkown stmt kind %d\n", stmt->kind);
    exit(1);
}



gen_data* generate_gen_data(const NodeProg* root) {
    if (!root) return NULL;
    gen_data* g = malloc(sizeof(gen_data));
    if (!g) { perror("malloc"); return NULL; }

    // reset label counter to avoid leaking labels between invocations
    int __label_counter = 0;

    g->m_prog = root;
    g->m_stack_pos = 0;
    g->m_output = sdsempty();
    g->m_block = malloc(sizeof(Str2DVec));
    kv_init(*g->m_block);

    g->m_vars = malloc(sizeof(StackVec));
    kv_init(*g->m_vars);
    if (!g->m_vars) { free(g); return NULL; }

    g->m_func = malloc(sizeof(func_args));
    kv_init(*g->m_func);

    // 1) collect var keys
    collect_vars(root, g);

    // 2) assign slots deterministically in source order (including nested lets)
    int next_slot = 0;
    for (size_t i = 0; i < kv_size(root->stmt); ++i) {
        assign_slots_in_stmt(&kv_A(root->stmt, i),g);
    }

    int slots = next_slot;

    // Compute bytes to reserve on stack: slots * 8, round up to 16 for alignment
    int bytes = slots * 8;
    if (bytes > 0) {
        // round up to multiple of 16
        bytes = ((bytes + 15) / 16) * 16;
    }
    // Emit prologue
    emit(g, "global _start\n");
    emit(g, "_start:\n");
    emit(g, "   push rbp\n");
    emit(g, "   mov rbp, rsp\n");
    if (bytes > 0) emit(g, "   sub rsp, %d\n", bytes);
    // Generate code for statements
    for (size_t i = 0; i < kv_size(root->stmt); ++i) {
        gen_stmt(g, &kv_A(root->stmt, i));
    }
    // Note: program usually exits via syscall in exit statements; if not, we still syscall(60) with rdi=0
    emit(g, "   mov rax, 60\n");
    emit(g, "   mov rdi, 0\n");
    emit(g, "   syscall\n");

    return g;
}

char* gen_prog(gen_data* g) {
    if (!g) return NULL;
    return g->m_output;
}

char* gen_prog_from_root(const NodeProg* root) {
    gen_data* g = generate_gen_data(root);
    if (!g) return NULL;
    return g->m_output;
}
