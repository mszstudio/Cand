/*
 * C& Programming Language — Native Recursive Descent Parser
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 */

#include "cand_compiler.h"

static ASTNode *create_node(ASTNodeKind kind, Token tok, const char *filename) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Memory allocation failed for ASTNode.\n");
        exit(1);
    }
    memset(node, 0, sizeof(ASTNode));
    node->kind = kind;
    node->line = tok.line > 0 ? tok.line : 1;
    node->col = tok.col > 0 ? tok.col : 1;
    strncpy(node->filename, filename ? filename : "main.cand", sizeof(node->filename) - 1);
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    if (node->body) ast_free(node->body);
    if (node->else_body) ast_free(node->else_body);
    free(node);
}

Parser *parser_create(Token *tokens, size_t token_count, const char *filename) {
    Parser *parser = (Parser *)malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->filename = filename ? filename : "main.cand";
    return parser;
}

static Token peek_token(Parser *parser) {
    if (parser->current >= parser->token_count) {
        Token eof_tok;
        memset(&eof_tok, 0, sizeof(Token));
        eof_tok.kind = TOKEN_EOF;
        strcpy(eof_tok.text, "EOF");
        eof_tok.line = parser->tokens ? parser->tokens[parser->token_count - 1].line : 1;
        eof_tok.col = parser->tokens ? parser->tokens[parser->token_count - 1].col : 1;
        return eof_tok;
    }
    return parser->tokens[parser->current];
}

static Token peek_token_ahead(Parser *parser, size_t offset) {
    if (parser->current + offset >= parser->token_count) {
        Token eof_tok;
        memset(&eof_tok, 0, sizeof(Token));
        eof_tok.kind = TOKEN_EOF;
        strcpy(eof_tok.text, "EOF");
        return eof_tok;
    }
    return parser->tokens[parser->current + offset];
}

static Token advance_token(Parser *parser) {
    Token tok = peek_token(parser);
    if (parser->current < parser->token_count) {
        parser->current++;
    }
    return tok;
}

static bool match_token(Parser *parser, TokenKind kind) {
    if (peek_token(parser).kind == kind) {
        advance_token(parser);
        return true;
    }
    return false;
}

static void expect_token(Parser *parser, TokenKind kind, const char *msg) {
    Token tok = peek_token(parser);
    if (tok.kind != kind) {
        fprintf(stderr, "%s:%d:%d: error: %s (Got '%s')\n",
                parser->filename, tok.line, tok.col, msg, tok.text);
    } else {
        advance_token(parser);
    }
}

/* ── Forward Declarations ─────────────────────────────────────────────────── */

static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_block(Parser *parser);

/* ── Type Parsing Helper ─────────────────────────────────────────────────── */

static void parse_type(Parser *parser, char *out_type_name, bool *out_is_pointer, int *out_pointer_depth) {
    *out_is_pointer = false;
    *out_pointer_depth = 0;

    while (peek_token(parser).kind == TOKEN_STAR) {
        advance_token(parser);
        (*out_is_pointer) = true;
        (*out_pointer_depth)++;
    }

    Token type_tok = advance_token(parser);
    strcpy(out_type_name, type_tok.text);
}

/* ── Statement Parsers ───────────────────────────────────────────────────── */

static ASTNode *parse_import(Parser *parser) {
    Token import_tok = advance_token(parser); // Consume 'import'
    ASTNode *node = create_node(AST_IMPORT_STMT, import_tok, parser->filename);

    Token name_tok = peek_token(parser);
    if (name_tok.kind == TOKEN_IDENTIFIER || name_tok.kind == TOKEN_STRING) {
        advance_token(parser);
        strcpy(node->name, name_tok.text);
    } else {
        fprintf(stderr, "%s:%d:%d: error: Expected module name after import\n", parser->filename, name_tok.line, name_tok.col);
    }

    match_token(parser, TOKEN_SEMICOLON);
    return node;
}

static ASTNode *parse_extern(Parser *parser) {
    Token extern_tok = advance_token(parser); // Consume 'extern'
    ASTNode *node = create_node(AST_EXTERN_DECL, extern_tok, parser->filename);

    if (match_token(parser, TOKEN_STRING)) {
        // e.g. extern "C"
    }

    if (peek_token(parser).kind == TOKEN_FN) {
        ASTNode *fn_node = parse_statement(parser);
        if (fn_node) {
            node->children[node->child_count++] = fn_node;
        }
    } else {
        fprintf(stderr, "%s:%d:%d: error: Expected fn declaration after extern\n", parser->filename, extern_tok.line, extern_tok.col);
    }
    return node;
}

static ASTNode *parse_struct(Parser *parser, bool is_pub) {
    Token struct_tok = advance_token(parser); // Consume 'struct'
    ASTNode *node = create_node(AST_STRUCT_DECL, struct_tok, parser->filename);
    node->is_pub = is_pub;

    Token name_tok = advance_token(parser);
    strcpy(node->name, name_tok.text);

    expect_token(parser, TOKEN_LBRACE, "Expected '{' after struct name");

    while (peek_token(parser).kind != TOKEN_RBRACE && peek_token(parser).kind != TOKEN_EOF) {
        bool field_pub = false;
        if (match_token(parser, TOKEN_PUB)) {
            field_pub = true;
        }

        Token field_name = advance_token(parser);
        expect_token(parser, TOKEN_COLON, "Expected ':' after field name");

        ASTNode *field_node = create_node(AST_STRUCT_FIELD, field_name, parser->filename);
        field_node->is_pub = field_pub;
        strcpy(field_node->name, field_name.text);

        parse_type(parser, field_node->type_name, &field_node->is_pointer, &field_node->pointer_depth);

        node->children[node->child_count++] = field_node;

        if (!match_token(parser, TOKEN_COMMA)) {
            match_token(parser, TOKEN_SEMICOLON);
        }
    }

    expect_token(parser, TOKEN_RBRACE, "Expected '}' closing struct body");
    return node;
}

static ASTNode *parse_impl(Parser *parser) {
    Token impl_tok = advance_token(parser); // Consume 'impl'
    ASTNode *node = create_node(AST_IMPL_BLOCK, impl_tok, parser->filename);

    Token name_tok = advance_token(parser);
    strcpy(node->name, name_tok.text);

    expect_token(parser, TOKEN_LBRACE, "Expected '{' in impl block");

    while (peek_token(parser).kind != TOKEN_RBRACE && peek_token(parser).kind != TOKEN_EOF) {
        ASTNode *fn_node = parse_statement(parser);
        if (fn_node) {
            node->children[node->child_count++] = fn_node;
        }
    }

    expect_token(parser, TOKEN_RBRACE, "Expected '}' closing impl block");
    return node;
}

static ASTNode *parse_function(Parser *parser, bool is_pub) {
    Token fn_tok = advance_token(parser); // Consume 'fn'
    ASTNode *node = create_node(AST_FN_DECL, fn_tok, parser->filename);
    node->is_pub = is_pub;

    Token name_tok = advance_token(parser);
    strcpy(node->name, name_tok.text);

    expect_token(parser, TOKEN_LPAREN, "Expected '(' after function name");

    while (peek_token(parser).kind != TOKEN_RPAREN && peek_token(parser).kind != TOKEN_EOF) {
        Token param_name = advance_token(parser);
        expect_token(parser, TOKEN_COLON, "Expected ':' after parameter name");

        ASTNode *param = create_node(AST_LET_DECL, param_name, parser->filename);
        strcpy(param->name, param_name.text);

        parse_type(parser, param->type_name, &param->is_pointer, &param->pointer_depth);

        node->children[node->child_count++] = param;

        if (!match_token(parser, TOKEN_COMMA)) break;
    }

    expect_token(parser, TOKEN_RPAREN, "Expected ')' after parameters");

    if (match_token(parser, TOKEN_ARROW)) {
        parse_type(parser, node->type_name, &node->is_pointer, &node->pointer_depth);
    } else {
        strcpy(node->type_name, "void");
    }

    if (peek_token(parser).kind == TOKEN_SEMICOLON) {
        advance_token(parser);
        return node;
    }

    if (peek_token(parser).kind != TOKEN_LBRACE) {
        // Prototype / extern function declaration without body
        return node;
    }

    node->body = parse_block(parser);
    return node;
}

static ASTNode *parse_block(Parser *parser) {
    Token brace_tok = peek_token(parser);
    expect_token(parser, TOKEN_LBRACE, "Expected '{' to start block");
    ASTNode *block = create_node(AST_BLOCK, brace_tok, parser->filename);

    while (peek_token(parser).kind != TOKEN_RBRACE && peek_token(parser).kind != TOKEN_EOF) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            block->children[block->child_count++] = stmt;
        }
    }

    expect_token(parser, TOKEN_RBRACE, "Expected '}' to end block");
    return block;
}

static ASTNode *parse_let(Parser *parser, bool is_pub) {
    Token let_tok = advance_token(parser); // Consume 'let' or 'const'
    ASTNode *node = create_node(AST_LET_DECL, let_tok, parser->filename);
    node->is_pub = is_pub;

    if (peek_token(parser).kind == TOKEN_MUT) {
        node->is_mut = true;
        advance_token(parser);
    }

    Token name_tok = advance_token(parser);
    strcpy(node->name, name_tok.text);

    if (match_token(parser, TOKEN_COLON)) {
        parse_type(parser, node->type_name, &node->is_pointer, &node->pointer_depth);
    }

    if (match_token(parser, TOKEN_ASSIGN)) {
        node->children[node->child_count++] = parse_expression(parser);
    }

    match_token(parser, TOKEN_SEMICOLON);
    return node;
}

static ASTNode *parse_while(Parser *parser) {
    Token while_tok = advance_token(parser); // Consume 'while'
    ASTNode *node = create_node(AST_WHILE_STMT, while_tok, parser->filename);
    node->children[node->child_count++] = parse_expression(parser);
    node->body = parse_block(parser);
    return node;
}

static ASTNode *parse_for(Parser *parser) {
    Token for_tok = advance_token(parser); // Consume 'for'
    ASTNode *node = create_node(AST_FOR_STMT, for_tok, parser->filename);

    Token var_tok = advance_token(parser);
    strcpy(node->name, var_tok.text);

    expect_token(parser, TOKEN_IN, "Expected 'in' after for loop variable");

    Token start_tok = advance_token(parser);
    node->range_start = atoi(start_tok.text);

    expect_token(parser, TOKEN_DOTDOT, "Expected '..' range operator in for loop");

    Token end_tok = advance_token(parser);
    node->range_end = atoi(end_tok.text);

    node->body = parse_block(parser);
    return node;
}

static ASTNode *parse_if(Parser *parser) {
    Token if_tok = advance_token(parser); // Consume 'if'
    ASTNode *node = create_node(AST_IF_STMT, if_tok, parser->filename);
    node->children[node->child_count++] = parse_expression(parser);
    node->body = parse_block(parser);

    if (match_token(parser, TOKEN_ELSE)) {
        if (peek_token(parser).kind == TOKEN_IF) {
            node->else_body = parse_if(parser);
        } else {
            node->else_body = parse_block(parser);
        }
    }
    return node;
}

static ASTNode *parse_return(Parser *parser) {
    Token ret_tok = advance_token(parser); // Consume 'return'
    ASTNode *node = create_node(AST_RETURN_STMT, ret_tok, parser->filename);
    if (peek_token(parser).kind != TOKEN_SEMICOLON) {
        node->children[node->child_count++] = parse_expression(parser);
    }
    match_token(parser, TOKEN_SEMICOLON);
    return node;
}

static ASTNode *parse_primary_expression(Parser *parser) {
    Token tok = peek_token(parser);

    if (tok.kind == TOKEN_NUMBER || tok.kind == TOKEN_STRING || tok.kind == TOKEN_TRUE || tok.kind == TOKEN_FALSE) {
        advance_token(parser);
        ASTNode *node = create_node(AST_LITERAL_EXPR, tok, parser->filename);
        strcpy(node->value, tok.text);
        return node;
    }

    if (tok.kind == TOKEN_IDENTIFIER || tok.kind == TOKEN_PRINTLN || tok.kind == TOKEN_PRINT) {
        advance_token(parser);
        ASTNode *node = create_node(AST_IDENT_EXPR, tok, parser->filename);
        strcpy(node->name, tok.text);

        // Function call
        if (match_token(parser, TOKEN_LPAREN)) {
            ASTNode *call_node = create_node(AST_CALL_EXPR, tok, parser->filename);
            strcpy(call_node->name, tok.text);

            while (peek_token(parser).kind != TOKEN_RPAREN && peek_token(parser).kind != TOKEN_EOF) {
                call_node->children[call_node->child_count++] = parse_expression(parser);
                if (!match_token(parser, TOKEN_COMMA)) break;
            }
            expect_token(parser, TOKEN_RPAREN, "Expected ')' after call arguments");
            return call_node;
        }

        // Member access (dot) e.g. rect.width or rect.area()
        if (match_token(parser, TOKEN_DOT)) {
            Token field_tok = advance_token(parser);
            
            // Check if member access is a method call e.g. p.area()
            if (match_token(parser, TOKEN_LPAREN)) {
                ASTNode *call_node = create_node(AST_CALL_EXPR, tok, parser->filename);
                strcpy(call_node->name, field_tok.text);
                strcpy(call_node->value, tok.text);
                
                // Add implicit receiver argument 'self' (tok.text)
                ASTNode *recv_node = create_node(AST_IDENT_EXPR, tok, parser->filename);
                strcpy(recv_node->name, tok.text);
                call_node->children[call_node->child_count++] = recv_node;

                while (peek_token(parser).kind != TOKEN_RPAREN && peek_token(parser).kind != TOKEN_EOF) {
                    call_node->children[call_node->child_count++] = parse_expression(parser);
                    if (!match_token(parser, TOKEN_COMMA)) break;
                }
                expect_token(parser, TOKEN_RPAREN, "Expected ')' after call arguments");
                return call_node;
            }

            ASTNode *member = create_node(AST_MEMBER_EXPR, tok, parser->filename);
            strcpy(member->name, tok.text);
            strcpy(member->value, field_tok.text);
            return member;
        }

        return node;
    }

    if (match_token(parser, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        expect_token(parser, TOKEN_RPAREN, "Expected ')' after parenthesized expression");
        return expr;
    }

    advance_token(parser);
    return create_node(AST_LITERAL_EXPR, tok, parser->filename);
}

static ASTNode *parse_expression(Parser *parser) {
    ASTNode *left = parse_primary_expression(parser);

    Token tok = peek_token(parser);
    if (tok.kind == TOKEN_PLUS || tok.kind == TOKEN_MINUS || tok.kind == TOKEN_STAR ||
        tok.kind == TOKEN_SLASH || tok.kind == TOKEN_EQ || tok.kind == TOKEN_NEQ ||
        tok.kind == TOKEN_LT || tok.kind == TOKEN_GT || tok.kind == TOKEN_LTE || tok.kind == TOKEN_GTE) {
        
        Token op_tok = advance_token(parser);
        ASTNode *bin_node = create_node(AST_BINARY_EXPR, op_tok, parser->filename);
        strcpy(bin_node->name, op_tok.text);
        bin_node->children[bin_node->child_count++] = left;
        bin_node->children[bin_node->child_count++] = parse_expression(parser);
        return bin_node;
    }

    return left;
}

static ASTNode *parse_statement(Parser *parser) {
    bool is_pub = false;
    Token tok = peek_token(parser);

    if (tok.kind == TOKEN_PUB) {
        is_pub = true;
        advance_token(parser);
        tok = peek_token(parser);
    }

    if (tok.kind == TOKEN_IMPORT) return parse_import(parser);
    if (tok.kind == TOKEN_EXTERN) return parse_extern(parser);
    if (tok.kind == TOKEN_STRUCT) return parse_struct(parser, is_pub);
    if (tok.kind == TOKEN_IMPL) return parse_impl(parser);
    if (tok.kind == TOKEN_FN) return parse_function(parser, is_pub);
    if (tok.kind == TOKEN_LET || tok.kind == TOKEN_CONST) return parse_let(parser, is_pub);
    if (tok.kind == TOKEN_IF) return parse_if(parser);
    if (tok.kind == TOKEN_WHILE) return parse_while(parser);
    if (tok.kind == TOKEN_FOR) return parse_for(parser);
    if (tok.kind == TOKEN_RETURN) return parse_return(parser);

    // Member assignment e.g. self.width = 10;
    if (tok.kind == TOKEN_IDENTIFIER && peek_token_ahead(parser, 1).kind == TOKEN_DOT && peek_token_ahead(parser, 3).kind == TOKEN_ASSIGN) {
        Token var_tok = advance_token(parser); // var name
        advance_token(parser); // dot
        Token field_tok = advance_token(parser); // field name
        advance_token(parser); // =

        ASTNode *assign_node = create_node(AST_BINARY_EXPR, tok, parser->filename);
        strcpy(assign_node->name, "=");

        ASTNode *mem_node = create_node(AST_MEMBER_EXPR, tok, parser->filename);
        strcpy(mem_node->name, var_tok.text);
        strcpy(mem_node->value, field_tok.text);

        assign_node->children[assign_node->child_count++] = mem_node;
        assign_node->children[assign_node->child_count++] = parse_expression(parser);
        match_token(parser, TOKEN_SEMICOLON);

        ASTNode *stmt = create_node(AST_EXPR_STMT, tok, parser->filename);
        stmt->children[stmt->child_count++] = assign_node;
        return stmt;
    }

    // Variable re-assignment statement: x = expr;
    if (tok.kind == TOKEN_IDENTIFIER && peek_token_ahead(parser, 1).kind == TOKEN_ASSIGN) {
        Token var_tok = advance_token(parser);
        advance_token(parser); // Consume '='
        ASTNode *assign_node = create_node(AST_BINARY_EXPR, tok, parser->filename);
        strcpy(assign_node->name, "=");
        
        ASTNode *var_node = create_node(AST_IDENT_EXPR, tok, parser->filename);
        strcpy(var_node->name, var_tok.text);

        assign_node->children[assign_node->child_count++] = var_node;
        assign_node->children[assign_node->child_count++] = parse_expression(parser);
        match_token(parser, TOKEN_SEMICOLON);

        ASTNode *stmt = create_node(AST_EXPR_STMT, tok, parser->filename);
        stmt->children[stmt->child_count++] = assign_node;
        return stmt;
    }

    ASTNode *expr = parse_expression(parser);
    match_token(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = create_node(AST_EXPR_STMT, tok, parser->filename);
    stmt->children[stmt->child_count++] = expr;
    return stmt;
}

ASTNode *parser_parse_program(Parser *parser) {
    Token prog_tok = peek_token(parser);
    ASTNode *program = create_node(AST_PROGRAM, prog_tok, parser->filename);
    while (peek_token(parser).kind != TOKEN_EOF) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            program->children[program->child_count++] = stmt;
        }
    }
    return program;
}
