/*
 * C& Programming Language — Native C Backend Code Generator & System Linker
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 */

#include "cand_compiler.h"
#include <windows.h>

static Module loaded_modules[MAX_MODULES];
static int loaded_module_count = 0;

/* ── Path Normalization Helper ─────────────────────────────────────────────── */

static void normalize_path(const char *in_path, char *out_path, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in_path[i] != '\0' && j < out_size - 1; i++) {
        if (in_path[i] == '\\') {
            out_path[j++] = '/';
        } else {
            out_path[j++] = in_path[i];
        }
    }
    out_path[j] = '\0';
}

/* ── Module Import System ─────────────────────────────────────────────────── */

ASTNode *module_load(const char *module_name, const char *importer_filename) {
    char norm_name[128];
    normalize_path(module_name, norm_name, sizeof(norm_name));

    for (int i = 0; i < loaded_module_count; i++) {
        if (strcmp(loaded_modules[i].module_name, norm_name) == 0) {
            return loaded_modules[i].ast; // Return cached AST (prevents circular re-compilation)
        }
    }

    char dir_prefix[256] = "";
    if (importer_filename) {
        const char *last_slash = strrchr(importer_filename, '/');
        const char *last_backslash = strrchr(importer_filename, '\\');
        const char *sep = last_slash > last_backslash ? last_slash : last_backslash;
        if (sep) {
            size_t dir_len = sep - importer_filename + 1;
            strncpy(dir_prefix, importer_filename, dir_len);
            dir_prefix[dir_len] = '\0';
        }
    }

    char candidate_paths[12][512];
    int cand_count = 0;
    snprintf(candidate_paths[cand_count++], 512, "%s%s.cand", dir_prefix, norm_name);
    snprintf(candidate_paths[cand_count++], 512, "%s.cand", norm_name);
    snprintf(candidate_paths[cand_count++], 512, "std/%s.cand", norm_name);
    snprintf(candidate_paths[cand_count++], 512, "src/%s.cand", norm_name);
    snprintf(candidate_paths[cand_count++], 512, "tests/%s.cand", norm_name);
    snprintf(candidate_paths[cand_count++], 512, "experiments/tests/%s.cand", norm_name);

    const char *cand_env = getenv("CAND_PATH");
    if (cand_env) {
        snprintf(candidate_paths[cand_count++], 512, "%s/std/%s.cand", cand_env, norm_name);
    }
    const char *home_env = getenv("HOME");
    if (home_env) {
        snprintf(candidate_paths[cand_count++], 512, "%s/.local/share/cand/std/%s.cand", home_env, norm_name);
    }
    snprintf(candidate_paths[cand_count++], 512, "/usr/local/share/cand/std/%s.cand", norm_name);
    snprintf(candidate_paths[cand_count++], 512, "/usr/share/cand/std/%s.cand", norm_name);

    FILE *f = NULL;
    char target_path[512] = "";

    for (int i = 0; i < cand_count; i++) {
        f = fopen(candidate_paths[i], "rb");
        if (f) {
            strcpy(target_path, candidate_paths[i]);
            break;
        }
    }

    if (!f) {
        fprintf(stderr, "%s:1:1: error: Cannot resolve imported module '%s'\n", importer_filename, module_name);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(sz + 1);
    size_t read_bytes = fread(buf, 1, sz, f);
    buf[read_bytes] = '\0';
    fclose(f);

    Lexer lexer;
    lexer_init(&lexer, buf, target_path);

    size_t token_cap = 2048;
    Token *tokens = (Token *)malloc(token_cap * sizeof(Token));
    size_t token_count = 0;

    while (1) {
        Token tok = lexer_next_token(&lexer);
        if (token_count >= token_cap) {
            token_cap *= 2;
            tokens = (Token *)realloc(tokens, token_cap * sizeof(Token));
        }
        tokens[token_count++] = tok;
        if (tok.kind == TOKEN_EOF) break;
    }

    Parser *parser = parser_create(tokens, token_count, target_path);
    ASTNode *module_ast = parser_parse_program(parser);

    free(buf);
    free(tokens);
    free(parser);

    if (loaded_module_count < MAX_MODULES) {
        Module *mod = &loaded_modules[loaded_module_count++];
        strncpy(mod->module_name, norm_name, sizeof(mod->module_name) - 1);
        strncpy(mod->file_path, target_path, sizeof(mod->file_path) - 1);
        mod->ast = module_ast;
        mod->is_loaded = true;
    }

    return module_ast;
}

/* ── C Code Emission Helpers ───────────────────────────────────────────────── */

static void map_type_to_c(const char *cand_type, bool is_pointer, char *out_c_type, size_t size) {
    if (strcmp(cand_type, "int") == 0) strcpy(out_c_type, "int");
    else if (strcmp(cand_type, "float") == 0) strcpy(out_c_type, "float");
    else if (strcmp(cand_type, "bool") == 0) strcpy(out_c_type, "bool");
    else if (strcmp(cand_type, "string") == 0) strcpy(out_c_type, "const char*");
    else if (strcmp(cand_type, "char") == 0) {
        if (is_pointer) strcpy(out_c_type, "const char*");
        else strcpy(out_c_type, "char");
        return;
    }
    else if (strcmp(cand_type, "void") == 0) strcpy(out_c_type, "void");
    else strncpy(out_c_type, cand_type, size - 1);

    if (is_pointer && strcmp(cand_type, "string") != 0) {
        strcat(out_c_type, "*");
    }
}

static void emit_expression(FILE *f, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_LITERAL_EXPR:
            // If the value starts with a digit or '-', it's a number literal — emit as-is.
            // Otherwise it is a string — wrap in double quotes.
            if (node->value[0] == '"') {
                // Already has quotes stored (shouldn't normally happen, but safe)
                fprintf(f, "%s", node->value);
            } else if ((node->value[0] >= '0' && node->value[0] <= '9') ||
                       node->value[0] == '-' ||
                       strcmp(node->value, "true") == 0 ||
                       strcmp(node->value, "false") == 0) {
                fprintf(f, "%s", node->value);
            } else {
                // String literal — the lexer strips the surrounding quotes, restore them
                fprintf(f, "\"%s\"", node->value);
            }
            break;

        case AST_IDENT_EXPR:
            fprintf(f, "%s", node->name);
            break;

        case AST_MEMBER_EXPR:
            if (strcmp(node->name, "self") == 0) {
                fprintf(f, "self->%s", node->value);
            } else {
                fprintf(f, "%s.%s", node->name, node->value);
            }
            break;

        case AST_BINARY_EXPR:
            fprintf(f, "(");
            if (node->child_count > 0) emit_expression(f, node->children[0]);
            fprintf(f, " %s ", node->name);
            if (node->child_count > 1) emit_expression(f, node->children[1]);
            fprintf(f, ")");
            break;

        case AST_CALL_EXPR:
            if (strcmp(node->name, "println") == 0 || strcmp(node->name, "print") == 0) {
                // Build a proper printf call
                // First arg = format string (may be a string literal or identifier)
                // Remaining args = values to print
                bool is_println = (strcmp(node->name, "println") == 0);

                if (node->child_count == 0) {
                    // println() with no args
                    fprintf(f, is_println ? "printf(\"\\n\")" : "printf(\"\")");
                } else if (node->child_count == 1) {
                    ASTNode *arg = node->children[0];
                    if (arg->kind == AST_LITERAL_EXPR &&
                        arg->value[0] != '-' &&
                        !(arg->value[0] >= '0' && arg->value[0] <= '9') &&
                        strcmp(arg->value, "true") != 0 && strcmp(arg->value, "false") != 0) {
                        // String literal
                        fprintf(f, is_println ? "printf(\"%s\\n\", \"%s\")" : "printf(\"%s\", \"%s\")",
                                arg->value, arg->value);
                    } else {
                        // Variable or number
                        fprintf(f, is_println ? "printf(\"%%g\\n\", (double)(" : "printf(\"%%g\", (double)(");
                        emit_expression(f, arg);
                        fprintf(f, "))");
                    }
                } else {
                    // Multi-arg: first is a label string, rest are values
                    ASTNode *label = node->children[0];
                    fprintf(f, "printf(\"");
                    // Label text
                    if (label->kind == AST_LITERAL_EXPR) {
                        fprintf(f, "%s", label->value);
                    }
                    // One format placeholder per remaining arg
                    for (int i = 1; i < node->child_count; i++) {
                        fprintf(f, "%%d");
                    }
                    if (is_println) fprintf(f, "\\n");
                    fprintf(f, "\"");
                    for (int i = 1; i < node->child_count; i++) {
                        fprintf(f, ", ");
                        emit_expression(f, node->children[i]);
                    }
                    fprintf(f, ")");
                }
            } else {
                // Regular function / method call
                fprintf(f, "%s(", node->name);
                for (int i = 0; i < node->child_count; i++) {
                    if (i > 0) fprintf(f, ", ");
                    emit_expression(f, node->children[i]);
                }
                fprintf(f, ")");
            }
            break;

        default:
            break;
    }
}

static void emit_statement(FILE *f, ASTNode *node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) fprintf(f, "    ");

    switch (node->kind) {
        case AST_LET_DECL: {
            char c_type[128];
            map_type_to_c(node->type_name[0] ? node->type_name : "int", node->is_pointer, c_type, sizeof(c_type));
            fprintf(f, "%s %s", c_type, node->name);
            if (node->child_count > 0) {
                fprintf(f, " = ");
                emit_expression(f, node->children[0]);
            }
            fprintf(f, ";\n");
            break;
        }

        case AST_EXPR_STMT:
            if (node->child_count > 0) {
                emit_expression(f, node->children[0]);
            }
            fprintf(f, ";\n");
            break;

        case AST_RETURN_STMT:
            fprintf(f, "return");
            if (node->child_count > 0) {
                fprintf(f, " ");
                emit_expression(f, node->children[0]);
            }
            fprintf(f, ";\n");
            break;

        case AST_WHILE_STMT:
            fprintf(f, "while (");
            if (node->child_count > 0) emit_expression(f, node->children[0]);
            fprintf(f, ") {\n");
            if (node->body) {
                for (int i = 0; i < node->body->child_count; i++) {
                    emit_statement(f, node->body->children[i], indent + 1);
                }
            }
            for (int i = 0; i < indent; i++) fprintf(f, "    ");
            fprintf(f, "}\n");
            break;

        case AST_FOR_STMT:
            fprintf(f, "for (int %s = %d; %s < %d; %s++) {\n",
                    node->name, node->range_start, node->name, node->range_end, node->name);
            if (node->body) {
                for (int i = 0; i < node->body->child_count; i++) {
                    emit_statement(f, node->body->children[i], indent + 1);
                }
            }
            for (int i = 0; i < indent; i++) fprintf(f, "    ");
            fprintf(f, "}\n");
            break;

        case AST_IF_STMT:
            fprintf(f, "if (");
            if (node->child_count > 0) emit_expression(f, node->children[0]);
            fprintf(f, ") {\n");
            if (node->body) {
                for (int i = 0; i < node->body->child_count; i++) {
                    emit_statement(f, node->body->children[i], indent + 1);
                }
            }
            for (int i = 0; i < indent; i++) fprintf(f, "    ");
            fprintf(f, "}");

            if (node->else_body) {
                fprintf(f, " else {\n");
                for (int i = 0; i < node->else_body->child_count; i++) {
                    emit_statement(f, node->else_body->children[i], indent + 1);
                }
                for (int i = 0; i < indent; i++) fprintf(f, "    ");
                fprintf(f, "}\n");
            } else {
                fprintf(f, "\n");
            }
            break;

        default:
            break;
    }
}

static void emit_node(FILE *f, ASTNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (int i = 0; i < node->child_count; i++) {
                emit_node(f, node->children[i]);
            }
            break;

        case AST_IMPORT_STMT: {
            ASTNode *mod_ast = module_load(node->name, node->filename);
            if (mod_ast) {
                for (int i = 0; i < mod_ast->child_count; i++) {
                    ASTNode *item = mod_ast->children[i];
                    // Emit public items AND impl blocks from imports
                    if (item->is_pub || item->kind == AST_IMPL_BLOCK || item->kind == AST_EXTERN_DECL) {
                        emit_node(f, item);
                    }
                }
            }
            break;
        }

        case AST_EXTERN_DECL:
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *child = node->children[i];
                if (child->kind == AST_FN_DECL) {
                    char ret_type[128];
                    map_type_to_c(child->type_name, child->is_pointer, ret_type, sizeof(ret_type));
                    fprintf(f, "extern %s %s(", ret_type, child->name);
                    for (int p = 0; p < child->child_count; p++) {
                        if (p > 0) fprintf(f, ", ");
                        ASTNode *param = child->children[p];
                        char param_type[128];
                        map_type_to_c(param->type_name, param->is_pointer, param_type, sizeof(param_type));
                        fprintf(f, "%s %s", param_type, param->name);
                    }
                    fprintf(f, ");\n");
                }
            }
            break;

        case AST_STRUCT_DECL:
            fprintf(f, "typedef struct %s {\n", node->name);
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *field = node->children[i];
                char field_type[128];
                map_type_to_c(field->type_name, field->is_pointer, field_type, sizeof(field_type));
                fprintf(f, "    %s %s;\n", field_type, field->name);
            }
            fprintf(f, "} %s;\n\n", node->name);
            break;

        case AST_IMPL_BLOCK:
            for (int i = 0; i < node->child_count; i++) {
                ASTNode *method = node->children[i];
                char ret_type[128];
                map_type_to_c(method->type_name, method->is_pointer, ret_type, sizeof(ret_type));

                fprintf(f, "%s %s_%s(", ret_type, node->name, method->name);
                for (int p = 0; p < method->child_count; p++) {
                    if (p > 0) fprintf(f, ", ");
                    ASTNode *param = method->children[p];
                    char param_type[128];
                    map_type_to_c(param->type_name, param->is_pointer, param_type, sizeof(param_type));
                    fprintf(f, "%s %s", param_type, param->name);
                }
                fprintf(f, ") {\n");

                if (method->body) {
                    for (int b = 0; b < method->body->child_count; b++) {
                        emit_statement(f, method->body->children[b], 1);
                    }
                }
                fprintf(f, "}\n\n");
            }
            break;

        case AST_FN_DECL: {
            if (node->body == NULL) {
                // Extern/Prototype declaration: already available via headers
                break;
            }
            char ret_type[128];
            if (strcmp(node->name, "main") == 0) {
                strcpy(ret_type, "int");
            } else {
                map_type_to_c(node->type_name, node->is_pointer, ret_type, sizeof(ret_type));
            }

            fprintf(f, "%s %s(", ret_type, node->name);
            for (int p = 0; p < node->child_count; p++) {
                if (p > 0) fprintf(f, ", ");
                ASTNode *param = node->children[p];
                char param_type[128];
                map_type_to_c(param->type_name, param->is_pointer, param_type, sizeof(param_type));
                fprintf(f, "%s %s", param_type, param->name);
            }
            fprintf(f, ") {\n");
            if (strcmp(node->name, "main") == 0) {
                fprintf(f, "#ifdef _WIN32\n");
                fprintf(f, "    SetConsoleOutputCP(65001);\n");
                fprintf(f, "    SetConsoleCP(65001);\n");
                fprintf(f, "#endif\n");
            }

            for (int b = 0; b < node->body->child_count; b++) {
                emit_statement(f, node->body->children[b], 1);
            }
            if (strcmp(node->name, "main") == 0) {
                fprintf(f, "    return 0;\n");
            }
            fprintf(f, "}\n\n");
            break;
        }

        default:
            break;
    }
}

bool codegen_generate_executable(ASTNode *program, const char *output_exe_path, bool keep_c_source) {
    char c_file_path[256];
    snprintf(c_file_path, sizeof(c_file_path), "%s_gen.c", output_exe_path);
    char *dot = strrchr(c_file_path, '.');
    if (dot) strcpy(dot, ".c");

    FILE *f = fopen(c_file_path, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not create generated C file: %s\n", c_file_path);
        return false;
    }

    fprintf(f, "/* Generated by C& Compiler Native Backend v%s */\n", CAND_VERSION);
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <stdbool.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <math.h>\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "#include <windows.h>\n");
    fprintf(f, "#else\n");
    fprintf(f, "#include <unistd.h>\n");
    fprintf(f, "#include <sys/time.h>\n");
    fprintf(f, "static inline void Sleep(unsigned int ms) { usleep(ms * 1000); }\n");
    fprintf(f, "static inline int MessageBoxA(void* hwnd, const char* text, const char* caption, unsigned int type) {\n");
    fprintf(f, "    (void)hwnd; (void)type;\n");
    fprintf(f, "    char cmd[2048];\n");
    fprintf(f, "    if (system(\"which zenity >/dev/null 2>&1\") == 0) {\n");
    fprintf(f, "        snprintf(cmd, sizeof(cmd), \"zenity --info --title=\\\"%%s\\\" --text=\\\"%%s\\\" 2>/dev/null\", caption ? caption : \"Notice\", text ? text : \"\");\n");
    fprintf(f, "        return system(cmd);\n");
    fprintf(f, "    } else if (system(\"which notify-send >/dev/null 2>&1\") == 0) {\n");
    fprintf(f, "        snprintf(cmd, sizeof(cmd), \"notify-send \\\"%%s\\\" \\\"%%s\\\" 2>/dev/null\", caption ? caption : \"Notice\", text ? text : \"\");\n");
    fprintf(f, "        return system(cmd);\n");
    fprintf(f, "    } else {\n");
    fprintf(f, "        printf(\"[%s] %%s\\n\", caption ? caption : \"Notice\", text ? text : \"\");\n");
    fprintf(f, "        return 0;\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");
    fprintf(f, "#endif\n");
    fprintf(f, "#include \"src/cand_airuntime.h\"\n\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "/* C& Native Windows Desktop GUI Calculator Engine */\n");
    fprintf(f, "static HWND hCalcDisplay;\n");
    fprintf(f, "static char calc_buf[64] = \"0\";\n");
    fprintf(f, "static double calc_accumulator = 0.0;\n");
    fprintf(f, "static char calc_pending_op = 0;\n");
    fprintf(f, "static bool calc_new_entry = true;\n\n");
    fprintf(f, "static void cand_calc_update_display(void) {\n");
    fprintf(f, "    if (hCalcDisplay) SetWindowTextA(hCalcDisplay, calc_buf);\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void cand_calc_append_digit(char ch) {\n");
    fprintf(f, "    if (calc_new_entry || strcmp(calc_buf, \"0\") == 0) {\n");
    fprintf(f, "        calc_buf[0] = ch;\n");
    fprintf(f, "        calc_buf[1] = '\\0';\n");
    fprintf(f, "        calc_new_entry = false;\n");
    fprintf(f, "    } else {\n");
    fprintf(f, "        size_t len = strlen(calc_buf);\n");
    fprintf(f, "        if (len < 30) { calc_buf[len] = ch; calc_buf[len + 1] = '\\0'; }\n");
    fprintf(f, "    }\n");
    fprintf(f, "    cand_calc_update_display();\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void cand_calc_set_operator(char op) {\n");
    fprintf(f, "    calc_accumulator = atof(calc_buf);\n");
    fprintf(f, "    calc_pending_op = op;\n");
    fprintf(f, "    calc_new_entry = true;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void cand_calc_evaluate(void) {\n");
    fprintf(f, "    if (calc_pending_op == 0) return;\n");
    fprintf(f, "    double second_val = atof(calc_buf);\n");
    fprintf(f, "    double res = 0.0;\n");
    fprintf(f, "    if (calc_pending_op == '+') res = calc_accumulator + second_val;\n");
    fprintf(f, "    else if (calc_pending_op == '-') res = calc_accumulator - second_val;\n");
    fprintf(f, "    else if (calc_pending_op == '*') res = calc_accumulator * second_val;\n");
    fprintf(f, "    else if (calc_pending_op == '/') {\n");
    fprintf(f, "        if (second_val == 0.0) {\n");
    fprintf(f, "            strcpy(calc_buf, \"Error: Cannot divide by zero\");\n");
    fprintf(f, "            cand_calc_update_display();\n");
    fprintf(f, "            calc_new_entry = true;\n");
    fprintf(f, "            calc_pending_op = 0;\n");
    fprintf(f, "            return;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        res = calc_accumulator / second_val;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    if (res == (long long)res) snprintf(calc_buf, sizeof(calc_buf), \"%lld\", (long long)res);\n");
    fprintf(f, "    else snprintf(calc_buf, sizeof(calc_buf), \"%.8g\", res);\n");
    fprintf(f, "    cand_calc_update_display();\n");
    fprintf(f, "    calc_accumulator = res;\n");
    fprintf(f, "    calc_pending_op = 0;\n");
    fprintf(f, "    calc_new_entry = true;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void cand_calc_clear(void) {\n");
    fprintf(f, "    strcpy(calc_buf, \"0\");\n");
    fprintf(f, "    calc_accumulator = 0.0;\n");
    fprintf(f, "    calc_pending_op = 0;\n");
    fprintf(f, "    calc_new_entry = true;\n");
    fprintf(f, "    cand_calc_update_display();\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void cand_calc_backspace(void) {\n");
    fprintf(f, "    size_t len = strlen(calc_buf);\n");
    fprintf(f, "    if (len > 1) calc_buf[len - 1] = '\\0';\n");
    fprintf(f, "    else { strcpy(calc_buf, \"0\"); calc_new_entry = true; }\n");
    fprintf(f, "    cand_calc_update_display();\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static LRESULT CALLBACK CandCalcWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {\n");
    fprintf(f, "    switch (msg) {\n");
    fprintf(f, "        case WM_CREATE: {\n");
    fprintf(f, "            hCalcDisplay = CreateWindowExA(WS_EX_CLIENTEDGE, \"EDIT\", \"0\", WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY, 15, 15, 266, 45, hwnd, (HMENU)100, NULL, NULL);\n");
    fprintf(f, "            HFONT hFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, \"Segoe UI\");\n");
    fprintf(f, "            SendMessage(hCalcDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"C\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 70, 60, 50, hwnd, (HMENU)201, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"CE\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 83, 70, 60, 50, hwnd, (HMENU)202, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"Del\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 151, 70, 60, 50, hwnd, (HMENU)203, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"/\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 219, 70, 60, 50, hwnd, (HMENU)204, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"7\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 126, 60, 50, hwnd, (HMENU)207, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"8\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 83, 126, 60, 50, hwnd, (HMENU)208, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"9\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 151, 126, 60, 50, hwnd, (HMENU)209, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"*\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 219, 126, 60, 50, hwnd, (HMENU)205, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"4\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 182, 60, 50, hwnd, (HMENU)2044, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"5\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 83, 182, 60, 50, hwnd, (HMENU)2055, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"6\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 151, 182, 60, 50, hwnd, (HMENU)2066, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"-\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 219, 182, 60, 50, hwnd, (HMENU)206, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"1\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 238, 60, 50, hwnd, (HMENU)2011, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"2\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 83, 238, 60, 50, hwnd, (HMENU)2022, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"3\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 151, 238, 60, 50, hwnd, (HMENU)2033, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"+\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 219, 238, 60, 50, hwnd, (HMENU)2070, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"0\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 15, 294, 128, 50, hwnd, (HMENU)200, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \".\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 151, 294, 60, 50, hwnd, (HMENU)210, NULL, NULL);\n");
    fprintf(f, "            CreateWindowExA(0, \"BUTTON\", \"=\", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 219, 294, 60, 50, hwnd, (HMENU)299, NULL, NULL);\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        case WM_COMMAND: {\n");
    fprintf(f, "            int id = LOWORD(wParam);\n");
    fprintf(f, "            if (id == 200) cand_calc_append_digit('0');\n");
    fprintf(f, "            else if (id == 2011) cand_calc_append_digit('1');\n");
    fprintf(f, "            else if (id == 2022) cand_calc_append_digit('2');\n");
    fprintf(f, "            else if (id == 2033) cand_calc_append_digit('3');\n");
    fprintf(f, "            else if (id == 2044) cand_calc_append_digit('4');\n");
    fprintf(f, "            else if (id == 2055) cand_calc_append_digit('5');\n");
    fprintf(f, "            else if (id == 2066) cand_calc_append_digit('6');\n");
    fprintf(f, "            else if (id == 207) cand_calc_append_digit('7');\n");
    fprintf(f, "            else if (id == 208) cand_calc_append_digit('8');\n");
    fprintf(f, "            else if (id == 209) cand_calc_append_digit('9');\n");
    fprintf(f, "            else if (id == 210) cand_calc_append_digit('.');\n");
    fprintf(f, "            else if (id == 201 || id == 202) cand_calc_clear();\n");
    fprintf(f, "            else if (id == 203) cand_calc_backspace();\n");
    fprintf(f, "            else if (id == 204) cand_calc_set_operator('/');\n");
    fprintf(f, "            else if (id == 205) cand_calc_set_operator('*');\n");
    fprintf(f, "            else if (id == 206) cand_calc_set_operator('-');\n");
    fprintf(f, "            else if (id == 2070) cand_calc_set_operator('+');\n");
    fprintf(f, "            else if (id == 299) cand_calc_evaluate();\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        case WM_DESTROY:\n");
    fprintf(f, "            PostQuitMessage(0);\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        default:\n");
    fprintf(f, "            return DefWindowProcA(hwnd, msg, wParam, lParam);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "int cand_launch_calculator_window(const char* title) {\n");
    fprintf(f, "    HINSTANCE hInstance = GetModuleHandleA(NULL);\n");
    fprintf(f, "    WNDCLASSEXA wc = {0};\n");
    fprintf(f, "    wc.cbSize = sizeof(WNDCLASSEXA);\n");
    fprintf(f, "    wc.lpfnWndProc = CandCalcWndProc;\n");
    fprintf(f, "    wc.hInstance = hInstance;\n");
    fprintf(f, "    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);\n");
    fprintf(f, "    wc.lpszClassName = \"CandWindowsCalculatorWinClass\";\n");
    fprintf(f, "    wc.hCursor = LoadCursor(NULL, IDC_ARROW);\n");
    fprintf(f, "    RegisterClassExA(&wc);\n");
    fprintf(f, "    HWND hwnd = CreateWindowExA(0, \"CandWindowsCalculatorWinClass\", title ? title : \"Windows Calculator - C& Language\", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 310, 400, NULL, NULL, hInstance, NULL);\n");
    fprintf(f, "    if (!hwnd) return 0;\n");
    fprintf(f, "    ShowWindow(hwnd, SW_SHOW);\n");
    fprintf(f, "    UpdateWindow(hwnd);\n");
    fprintf(f, "    MSG msg;\n");
    fprintf(f, "    while (GetMessageA(&msg, NULL, 0, 0)) {\n");
    fprintf(f, "        TranslateMessage(&msg);\n");
    fprintf(f, "        DispatchMessageA(&msg);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fprintf(f, "#else\n");
    fprintf(f, "/* Fallback GUI Calculator for Non-Windows Platforms */\n");
    fprintf(f, "int cand_launch_calculator_window(const char* title) {\n");
    fprintf(f, "    (void)title;\n");
    fprintf(f, "    printf(\"[C& GUI] GUI calculator is currently native to Windows (Win32). Terminal fallback active.\\n\");\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fprintf(f, "#endif\n\n");
    fprintf(f, "/* C& Python & Network Ecosystem Extensions */\n");
    fprintf(f, "int cand_py_exec(const char* code) {\n");
    fprintf(f, "    if (!code) return -1;\n");
    fprintf(f, "    char cmd[8192];\n");
    fprintf(f, "    snprintf(cmd, sizeof(cmd), \"python -c \\\"%%s\\\" 2>/dev/null || python3 -c \\\"%%s\\\"\", code, code);\n");
    fprintf(f, "    return system(cmd);\n");
    fprintf(f, "}\n\n");
    fprintf(f, "int cand_send_discord_webhook(const char* url, const char* message) {\n");
    fprintf(f, "    if (!url || !message) return -1;\n");
    fprintf(f, "    char cmd[8192];\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "    snprintf(cmd, sizeof(cmd), \"powershell -NoProfile -Command \\\"[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; $body = @{ content = '%s' } | ConvertTo-Json; Invoke-RestMethod -Uri '%s' -Method Post -ContentType 'application/json' -Body $body\\\"\", message, url);\n");
    fprintf(f, "#else\n");
    fprintf(f, "    snprintf(cmd, sizeof(cmd), \"curl -s -H \\\"Content-Type: application/json\\\" -d '{\\\"content\\\": \\\"%%s\\\"}' '%%s' >/dev/null 2>&1\", message, url);\n");
    fprintf(f, "#endif\n");
    fprintf(f, "    return system(cmd);\n");
    fprintf(f, "}\n\n");
    fprintf(f, "int cand_load_env_file(const char* path) {\n");
    fprintf(f, "    FILE* fp = fopen(path ? path : \".env\", \"r\");\n");
    fprintf(f, "    if (!fp) return -1;\n");
    fprintf(f, "    char line[512];\n");
    fprintf(f, "    while (fgets(line, sizeof(line), fp)) {\n");
    fprintf(f, "        char* eq = strchr(line, '=');\n");
    fprintf(f, "        if (eq) {\n");
    fprintf(f, "            *eq = '\\0';\n");
    fprintf(f, "            char* key = line; char* val = eq + 1;\n");
    fprintf(f, "            char* nl = strchr(val, '\\r'); if (nl) *nl = '\\0';\n");
    fprintf(f, "            nl = strchr(val, '\\n'); if (nl) *nl = '\\0';\n");
    fprintf(f, "#ifdef _WIN32\n");
    fprintf(f, "            SetEnvironmentVariableA(key, val);\n");
    fprintf(f, "#else\n");
    fprintf(f, "            setenv(key, val, 1);\n");
    fprintf(f, "#endif\n");
    fprintf(f, "        }\n");
    fprintf(f, "    }\n");
    fprintf(f, "    fclose(fp);\n");
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "/* Node.js & NPM Ecosystem Integration */\n");
    fprintf(f, "int cand_node_exec(const char* code) {\n");
    fprintf(f, "    if (!code) return -1;\n");
    fprintf(f, "    char cmd[8192];\n");
    fprintf(f, "    snprintf(cmd, sizeof(cmd), \"node -e \\\"%%s\\\"\", code);\n");
    fprintf(f, "    return system(cmd);\n");
    fprintf(f, "}\n\n");
    fprintf(f, "int cand_npm_install(const char* pkg) {\n");
    fprintf(f, "    if (!pkg) return -1;\n");
    fprintf(f, "    char cmd[4096];\n");
    fprintf(f, "    snprintf(cmd, sizeof(cmd), \"npm install %%s\", pkg);\n");
    fprintf(f, "    return system(cmd);\n");
    fprintf(f, "}\n\n");
    emit_node(f, program);

    fclose(f);

    int res = -1;
#ifndef _WIN32
    char compile_cmd[4096] = "";
    snprintf(compile_cmd, sizeof(compile_cmd),
        "clang -O2 -I. -Isrc -lm \"%s\" -o \"%s\" 2>/dev/null || "
        "gcc -O2 -I. -Isrc -lm \"%s\" -o \"%s\"",
        c_file_path, output_exe_path,
        c_file_path, output_exe_path);
    res = system(compile_cmd);
#else
    char compile_cmd[4096] = "";

    // ── Locate the best available C compiler ─────────────────────────────
    // On Windows, system() breaks when the executable path has spaces OR
    // when embedded quotes are in argument strings (SDK include/lib paths).
    // Workaround: write the command to a temp .bat file and run it via cmd /C.

    const char *known_compilers[] = {
        "C:\\Program Files\\LLVM\\bin\\clang.exe",
        "C:\\Program Files (x86)\\LLVM\\bin\\clang.exe",
        "C:\\LLVM\\bin\\clang.exe",
        "C:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\bin\\Hostx64\\x64\\cl.exe",
        NULL
    };

    char compiler_exe[512] = "";

    for (int i = 0; known_compilers[i] != NULL; i++) {
        FILE *test_f = fopen(known_compilers[i], "rb");
        if (test_f) {
            fclose(test_f);
            strncpy(compiler_exe, known_compilers[i], sizeof(compiler_exe) - 1);
            break;
        }
    }

    char inner_cmd[4096] = "";
    if (compiler_exe[0] != '\0') {
        if (strstr(compiler_exe, "cl.exe")) {
            snprintf(inner_cmd, sizeof(inner_cmd),
                "\"%s\" /nologo /O2 /I. /Isrc -D_CRT_SECURE_NO_WARNINGS "
                "-IC:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\include "
                "\"-IC:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\ucrt\" "
                "\"-IC:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\um\" "
                "\"-IC:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\shared\" "
                "\"%s\" /Fe:\"%s\" /link "
                "/LIBPATH:C:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64 "
                "\"/LIBPATH:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\ucrt\\x64\" "
                "\"/LIBPATH:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\um\\x64\" "
                "User32.lib Gdi32.lib",
                compiler_exe, c_file_path, output_exe_path);
        } else {
            const char *inc_flags =
                "-I. -Isrc -D_CRT_SECURE_NO_WARNINGS "
                "-IC:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\include "
                "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\ucrt\" "
                "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\um\" "
                "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\shared\"";

            const char *lib_flags =
                "-LC:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64 "
                "-L\"C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\ucrt\\x64\" "
                "-L\"C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\um\\x64\" "
                "-luser32 -lgdi32";

            snprintf(inner_cmd, sizeof(inner_cmd),
                "\"%s\" -O2 -Wno-everything %s %s \"%s\" -o \"%s\"",
                compiler_exe, inc_flags, lib_flags, c_file_path, output_exe_path);
        }
    } else {
        const char *inc_flags =
            "-I. -Isrc -D_CRT_SECURE_NO_WARNINGS "
            "-IC:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\include "
            "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\ucrt\" "
            "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\um\" "
            "-I\"C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.22621.0\\shared\"";

        const char *lib_flags =
            "-LC:\\BuildTools\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64 "
            "-L\"C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\ucrt\\x64\" "
            "-L\"C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.22621.0\\um\\x64\" "
            "-luser32 -lgdi32";

        snprintf(inner_cmd, sizeof(inner_cmd),
            "clang -O2 -Wno-everything %s %s \"%s\" -o \"%s\" || "
            "gcc -O2 \"%s\" -o \"%s\"",
            inc_flags, lib_flags, c_file_path, output_exe_path,
            c_file_path, output_exe_path);
    }

    // Write to a temp .bat so cmd handles quoting properly
    char bat_path[256] = "cand_build_tmp.bat";
    FILE *bat = fopen(bat_path, "w");
    if (bat) {
        fprintf(bat, "@echo off\r\n%s\r\n", inner_cmd);
        fclose(bat);
        snprintf(compile_cmd, sizeof(compile_cmd), "cmd.exe /C %s", bat_path);
        res = system(compile_cmd);
        remove(bat_path);
    } else {
        // Last resort: run directly
        snprintf(compile_cmd, sizeof(compile_cmd), "cmd.exe /C \"%s\"", inner_cmd);
        res = system(compile_cmd);
    }
#endif

    if (res != 0) {
        fprintf(stderr, "%s:1:1: error: Native compilation failed\n", c_file_path);
    }

    if (!keep_c_source) {
        remove(c_file_path);
    }

    return (res == 0);
}

