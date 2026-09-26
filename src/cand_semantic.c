/*
 * C& Programming Language — Native Semantic Analyzer & Type Verification Engine
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 */

#include "cand_compiler.h"

static Scope *scope_create(Scope *parent) {
    Scope *s = (Scope *)malloc(sizeof(Scope));
    if (!s) return NULL;
    s->symbol_count = 0;
    s->parent = parent;
    return s;
}

static void scope_free(Scope *scope) {
    if (!scope) return;
    free(scope);
}

static bool symbol_table_add(SymbolTable *st, const char *name, const char *type_name, SymbolKind kind, bool is_pub, bool is_mut, bool is_pointer, int line, int col) {
    Scope *cur = st->current_scope;
    for (int i = 0; i < cur->symbol_count; i++) {
        if (strcmp(cur->symbols[i].name, name) == 0) {
            fprintf(stderr, "%s:%d:%d: error: Redeclaration of symbol '%s'\n",
                    st->current_filename, line, col, name);
            st->has_error = true;
            return false;
        }
    }

    if (cur->symbol_count >= MAX_SYMBOLS) {
        fprintf(stderr, "%s:%d:%d: error: Maximum symbol limit reached in scope\n", st->current_filename, line, col);
        st->has_error = true;
        return false;
    }

    Symbol *sym = &cur->symbols[cur->symbol_count++];
    strncpy(sym->name, name, sizeof(sym->name) - 1);
    strncpy(sym->type_name, type_name ? type_name : "void", sizeof(sym->type_name) - 1);
    sym->kind = kind;
    sym->is_pub = is_pub;
    sym->is_mut = is_mut;
    sym->is_pointer = is_pointer;
    sym->line = line;
    sym->col = col;
    strncpy(sym->filename, st->current_filename, sizeof(sym->filename) - 1);

    return true;
}

static Symbol *symbol_table_lookup(SymbolTable *st, const char *name) {
    Scope *cur = st->current_scope;
    while (cur) {
        for (int i = 0; i < cur->symbol_count; i++) {
            if (strcmp(cur->symbols[i].name, name) == 0) {
                return &cur->symbols[i];
            }
        }
        cur = cur->parent;
    }
    return NULL;
}

static bool is_valid_type(SymbolTable *st, const char *type_name, bool is_pointer) {
    if (!type_name || strlen(type_name) == 0) return true;
    if (is_pointer) return true; // Pointer types (*char, *void, *int, *Struct) are valid C pointers
    if (strcmp(type_name, "int") == 0 || strcmp(type_name, "float") == 0 ||
        strcmp(type_name, "bool") == 0 || strcmp(type_name, "string") == 0 ||
        strcmp(type_name, "char") == 0 || strcmp(type_name, "void") == 0) {
        return true;
    }
    // Check if type is a declared struct in symbol table
    Symbol *sym = symbol_table_lookup(st, type_name);
    if (sym && sym->kind == SYMBOL_STRUCT) {
        return true;
    }
    return false;
}

static void analyze_node(SymbolTable *st, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }
            break;

        case AST_IMPORT_STMT: {
            symbol_table_add(st, node->name, "module", SYMBOL_MODULE, true, false, false, node->line, node->col);
            ASTNode *mod_ast = module_load(node->name, node->filename);
            if (mod_ast) {
                for (int i = 0; i < mod_ast->child_count; i++) {
                    ASTNode *child = mod_ast->children[i];
                    if (child->is_pub || child->kind == AST_IMPL_BLOCK || child->kind == AST_EXTERN_DECL) {
                        analyze_node(st, child);
                    }
                }
            }
            break;
        }

        case AST_STRUCT_DECL: {
            if (!symbol_table_add(st, node->name, "struct", SYMBOL_STRUCT, node->is_pub, false, false, node->line, node->col)) {
                break;
            }

            // Check struct fields for duplicates and valid types
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *field = node->children[i];
                for (int j = 0; j < i; j++) {
                    if (strcmp(node->children[j]->name, field->name) == 0) {
                        fprintf(stderr, "%s:%d:%d: error: Duplicate field '%s' in struct '%s'\n",
                                node->filename, field->line, field->col, field->name, node->name);
                        st->has_error = true;
                    }
                }

                if (!is_valid_type(st, field->type_name, field->is_pointer)) {
                    fprintf(stderr, "%s:%d:%d: error: Unknown field type '%s' for field '%s'\n",
                            node->filename, field->line, field->col, field->type_name, field->name);
                    st->has_error = true;
                }
            }
            break;
        }

        case AST_IMPL_BLOCK: {
            Symbol *struct_sym = symbol_table_lookup(st, node->name);
            if (!struct_sym || struct_sym->kind != SYMBOL_STRUCT) {
                fprintf(stderr, "%s:%d:%d: error: Cannot implement methods for unknown struct '%s'\n",
                        node->filename, node->line, node->col, node->name);
                st->has_error = true;
            }

            for (int i = 0; i < node->child_count; i++) {
                ASTNode *method = node->children[i];
                char method_full_name[256];
                snprintf(method_full_name, sizeof(method_full_name), "%s_%s", node->name, method->name);
                symbol_table_add(st, method_full_name, method->type_name, SYMBOL_FUNCTION, method->is_pub, false, false, method->line, method->col);
                analyze_node(st, method);
            }
            break;
        }

        case AST_EXTERN_DECL:
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *fn_node = node->children[i];
                symbol_table_add(st, fn_node->name, fn_node->type_name, SYMBOL_FUNCTION, true, false, fn_node->is_pointer, fn_node->line, fn_node->col);
            }
            break;

        case AST_FN_DECL: {
            symbol_table_add(st, node->name, node->type_name, SYMBOL_FUNCTION, node->is_pub, false, node->is_pointer, node->line, node->col);

            // Push function scope
            Scope *fn_scope = scope_create(st->current_scope);
            st->current_scope = fn_scope;

            // Register function parameters in scope
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *param = node->children[i];
                symbol_table_add(st, param->name, param->type_name, SYMBOL_VARIABLE, false, false, param->is_pointer, param->line, param->col);
            }

            if (node->body) {
                analyze_node(st, node->body);
            }

            // Pop scope
            st->current_scope = fn_scope->parent;
            scope_free(fn_scope);
            break;
        }

        case AST_BLOCK: {
            Scope *block_scope = scope_create(st->current_scope);
            st->current_scope = block_scope;

            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }

            st->current_scope = block_scope->parent;
            scope_free(block_scope);
            break;
        }

        case AST_LET_DECL: {
            if (node->child_count > 0 && (node->type_name[0] == '\0' || strcmp(node->type_name, "int") == 0 && !node->is_pointer)) {
                ASTNode *init_expr = node->children[0];
                if (init_expr->kind == AST_CALL_EXPR) {
                    Symbol *fn_sym = symbol_table_lookup(st, init_expr->name);
                    if (fn_sym && fn_sym->kind == SYMBOL_FUNCTION && fn_sym->type_name[0] != '\0') {
                        strcpy(node->type_name, fn_sym->type_name);
                        node->is_pointer = fn_sym->is_pointer;
                    }
                } else if (init_expr->kind == AST_LITERAL_EXPR) {
                    if (init_expr->value[0] == '"') {
                        strcpy(node->type_name, "string");
                        node->is_pointer = true;
                    } else if (strchr(init_expr->value, '.')) {
                        strcpy(node->type_name, "float");
                        node->is_pointer = false;
                    } else {
                        strcpy(node->type_name, "int");
                        node->is_pointer = false;
                    }
                }
            }
            if (!is_valid_type(st, node->type_name, node->is_pointer)) {
                fprintf(stderr, "%s:%d:%d: error: Unknown type '%s' in declaration of '%s'\n",
                        node->filename, node->line, node->col, node->type_name, node->name);
                st->has_error = true;
            }
            symbol_table_add(st, node->name, node->type_name, SYMBOL_VARIABLE, node->is_pub, node->is_mut, node->is_pointer, node->line, node->col);
            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }
            break;
        }

        case AST_IDENT_EXPR: {
            // Check built-in functions
            if (strcmp(node->name, "println") == 0 || strcmp(node->name, "print") == 0) break;

            Symbol *sym = symbol_table_lookup(st, node->name);
            if (!sym) {
                fprintf(stderr, "%s:%d:%d: error: Use of undeclared identifier '%s'\n",
                        node->filename, node->line, node->col, node->name);
                st->has_error = true;
            }
            break;
        }

        case AST_CALL_EXPR: {
            if (strcmp(node->name, "println") == 0 || strcmp(node->name, "print") == 0) break;

            if (node->value[0] != '\0') {
                // Method call on object node->value (e.g. "p")
                Symbol *var_sym = symbol_table_lookup(st, node->value);
                if (var_sym) {
                    char mangled_name[256];
                    snprintf(mangled_name, sizeof(mangled_name), "%s_%s", var_sym->type_name, node->name);
                    strcpy(node->name, mangled_name);
                    node->value[0] = '\0';
                }
            }

            Symbol *sym = symbol_table_lookup(st, node->name);
            if (!sym) {
                fprintf(stderr, "%s:%d:%d: error: Call to undeclared function or method '%s'\n",
                        node->filename, node->line, node->col, node->name);
                st->has_error = true;
            }
            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }
            break;
        }

        case AST_WHILE_STMT:
            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }
            if (node->body) analyze_node(st, node->body);
            break;

        case AST_FOR_STMT: {
            Scope *for_scope = scope_create(st->current_scope);
            st->current_scope = for_scope;
            symbol_table_add(st, node->name, "int", SYMBOL_VARIABLE, false, false, false, node->line, node->col);
            if (node->body) analyze_node(st, node->body);
            st->current_scope = for_scope->parent;
            scope_free(for_scope);
            break;
        }

        default:
            for (int i = 0; i < node->child_count; i++) {
                analyze_node(st, node->children[i]);
            }
            break;
    }
}

bool semantic_analyze(ASTNode *program) {
    if (!program) return false;

    SymbolTable st;
    memset(&st, 0, sizeof(SymbolTable));
    st.global_scope = scope_create(NULL);
    st.current_scope = st.global_scope;
    st.has_error = false;
    strncpy(st.current_filename, program->filename, sizeof(st.current_filename) - 1);

    analyze_node(&st, program);

    bool result = !st.has_error;
    scope_free(st.global_scope);
    return result;
}
