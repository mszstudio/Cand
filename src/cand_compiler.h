/*
 * C& Programming Language — Native Standalone Compiler Architecture
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 * Distributed under the MIT License.
 */

#ifndef CAND_COMPILER_H
#define CAND_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define CAND_VERSION "1.0.0 (Independent Native Engine)"
#define MAX_TOKEN_LEN 1024
#define MAX_AST_CHILDREN 256
#define MAX_SYMBOLS 512
#define MAX_MODULES 64

/* ── Token Kinds ───────────────────────────────────────────────────────────── */

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,

    // Keywords
    TOKEN_IMPORT,
    TOKEN_FN,
    TOKEN_LET,
    TOKEN_MUT,
    TOKEN_CONST,
    TOKEN_STRUCT,
    TOKEN_IMPL,
    TOKEN_ENUM,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_IN,         // in (used in for i in 0..10)
    TOKEN_RETURN,
    TOKEN_PRINTLN,
    TOKEN_PRINT,
    TOKEN_UNSAFE,
    TOKEN_EXTERN,
    TOKEN_PUB,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_BOOL,
    TOKEN_STRING_TYPE,
    TOKEN_TRUE,
    TOKEN_FALSE,

    // Punctuation & Operators
    TOKEN_LPAREN,     // (
    TOKEN_RPAREN,     // )
    TOKEN_LBRACE,     // {
    TOKEN_RBRACE,     // }
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]
    TOKEN_COLON,      // :
    TOKEN_SEMICOLON,  // ;
    TOKEN_COMMA,      // ,
    TOKEN_DOT,        // .
    TOKEN_DOTDOT,     // ..
    TOKEN_ASSIGN,     // =
    TOKEN_ARROW,      // ->
    TOKEN_PLUS,       // +
    TOKEN_MINUS,      // -
    TOKEN_STAR,       // *
    TOKEN_SLASH,      // /
    TOKEN_EQ,         // ==
    TOKEN_NEQ,        // !=
    TOKEN_LT,         // <
    TOKEN_GT,         // >
    TOKEN_LTE,        // <=
    TOKEN_GTE,        // >=
    TOKEN_UNKNOWN
} TokenKind;

typedef struct {
    TokenKind kind;
    char text[MAX_TOKEN_LEN];
    int line;
    int col;
} Token;

typedef struct {
    const char *source;
    size_t pos;
    size_t len;
    int line;
    int col;
    const char *filename;
} Lexer;

/* ── AST Nodes ─────────────────────────────────────────────────────────────── */

typedef enum {
    AST_PROGRAM,
    AST_IMPORT_STMT,
    AST_FN_DECL,
    AST_LET_DECL,
    AST_CONST_DECL,
    AST_STRUCT_DECL,
    AST_STRUCT_FIELD,
    AST_IMPL_BLOCK,
    AST_EXTERN_DECL,
    AST_BLOCK,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_EXPR_STMT,
    AST_BINARY_EXPR,
    AST_CALL_EXPR,
    AST_MEMBER_EXPR,
    AST_IDENT_EXPR,
    AST_LITERAL_EXPR
} ASTNodeKind;

typedef struct ASTNode {
    ASTNodeKind kind;
    char name[256];
    char value[256];
    char type_name[64];
    bool is_mut;
    bool is_pub;
    bool is_pointer;
    int pointer_depth;
    
    // Source diagnostic location
    int line;
    int col;
    char filename[256];

    // For range loops (for i in start..end)
    int range_start;
    int range_end;

    struct ASTNode *children[MAX_AST_CHILDREN];
    int child_count;

    struct ASTNode *body;
    struct ASTNode *else_body;
} ASTNode;

typedef struct {
    Token *tokens;
    size_t token_count;
    size_t current;
    const char *filename;
} Parser;

/* ── Symbol Table & Semantic Model ─────────────────────────────────────────── */

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_STRUCT,
    SYMBOL_MODULE
} SymbolKind;

typedef struct {
    char name[128];
    char type_name[64];
    SymbolKind kind;
    bool is_pub;
    bool is_mut;
    bool is_pointer;
    int line;
    int col;
    char filename[256];
} Symbol;

typedef struct Scope {
    Symbol symbols[MAX_SYMBOLS];
    int symbol_count;
    struct Scope *parent;
} Scope;

typedef struct {
    Scope *current_scope;
    Scope *global_scope;
    char current_filename[256];
    bool has_error;
} SymbolTable;

typedef struct {
    char module_name[128];
    char file_path[256];
    ASTNode *ast;
    bool is_loaded;
} Module;

/* ── Subsystem Prototypes ─────────────────────────────────────────────────── */

void lexer_init(Lexer *lexer, const char *source, const char *filename);
Token lexer_next_token(Lexer *lexer);

Parser *parser_create(Token *tokens, size_t token_count, const char *filename);
ASTNode *parser_parse_program(Parser *parser);
void ast_free(ASTNode *node);

bool semantic_analyze(ASTNode *program);

bool codegen_generate_executable(ASTNode *program, const char *output_exe_path, bool keep_c_source);

// Module Loader Interface
ASTNode *module_load(const char *module_name, const char *importer_filename);

#endif /* CAND_COMPILER_H */
