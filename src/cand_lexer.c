/*
 * C& Programming Language — Hand-Crafted Lexical Analyzer
 * Copyright (c) 2026 MSZ Studio. All rights reserved.
 */

#include "cand_compiler.h"

void lexer_init(Lexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->len = source ? strlen(source) : 0;
    lexer->line = 1;
    lexer->col = 1;
    lexer->filename = filename ? filename : "main.cand";
}

static char peek_char(Lexer *lexer) {
    if (lexer->pos >= lexer->len) return '\0';
    return lexer->source[lexer->pos];
}

static char peek_ahead(Lexer *lexer, size_t offset) {
    if (lexer->pos + offset >= lexer->len) return '\0';
    return lexer->source[lexer->pos + offset];
}

static char advance_char(Lexer *lexer) {
    char c = peek_char(lexer);
    if (c != '\0') {
        lexer->pos++;
        if (c == '\n') {
            lexer->line++;
            lexer->col = 1;
        } else {
            lexer->col++;
        }
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    while (lexer->pos < lexer->len) {
        char c = peek_char(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lexer);
        } else if (c == '/' && peek_ahead(lexer, 1) == '/') {
            // Line comment
            while (lexer->pos < lexer->len && peek_char(lexer) != '\n') {
                advance_char(lexer);
            }
        } else if (c == '/' && peek_ahead(lexer, 1) == '*') {
            // Block comment
            advance_char(lexer);
            advance_char(lexer);
            while (lexer->pos < lexer->len) {
                if (peek_char(lexer) == '*' && peek_ahead(lexer, 1) == '/') {
                    advance_char(lexer);
                    advance_char(lexer);
                    break;
                }
                advance_char(lexer);
            }
        } else {
            break;
        }
    }
}

static Token make_token(TokenKind kind, const char *text, int line, int col) {
    Token tok;
    tok.kind = kind;
    tok.line = line;
    tok.col = col;
    strncpy(tok.text, text, MAX_TOKEN_LEN - 1);
    tok.text[MAX_TOKEN_LEN - 1] = '\0';
    return tok;
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int start_line = lexer->line;
    int start_col = lexer->col;

    if (lexer->pos >= lexer->len) {
        return make_token(TOKEN_EOF, "EOF", start_line, start_col);
    }

    char c = peek_char(lexer);

    // Identifiers & Keywords
    if (isalpha(c) || c == '_') {
        char buf[MAX_TOKEN_LEN];
        size_t len = 0;
        while ((isalnum(c) || c == '_') && len < MAX_TOKEN_LEN - 1) {
            buf[len++] = advance_char(lexer);
            c = peek_char(lexer);
        }
        buf[len] = '\0';

        if (strcmp(buf, "import") == 0) return make_token(TOKEN_IMPORT, buf, start_line, start_col);
        if (strcmp(buf, "fn") == 0) return make_token(TOKEN_FN, buf, start_line, start_col);
        if (strcmp(buf, "let") == 0) return make_token(TOKEN_LET, buf, start_line, start_col);
        if (strcmp(buf, "mut") == 0) return make_token(TOKEN_MUT, buf, start_line, start_col);
        if (strcmp(buf, "const") == 0) return make_token(TOKEN_CONST, buf, start_line, start_col);
        if (strcmp(buf, "struct") == 0) return make_token(TOKEN_STRUCT, buf, start_line, start_col);
        if (strcmp(buf, "impl") == 0) return make_token(TOKEN_IMPL, buf, start_line, start_col);
        if (strcmp(buf, "enum") == 0) return make_token(TOKEN_ENUM, buf, start_line, start_col);
        if (strcmp(buf, "if") == 0) return make_token(TOKEN_IF, buf, start_line, start_col);
        if (strcmp(buf, "else") == 0) return make_token(TOKEN_ELSE, buf, start_line, start_col);
        if (strcmp(buf, "while") == 0) return make_token(TOKEN_WHILE, buf, start_line, start_col);
        if (strcmp(buf, "for") == 0) return make_token(TOKEN_FOR, buf, start_line, start_col);
        if (strcmp(buf, "in") == 0) return make_token(TOKEN_IN, buf, start_line, start_col);
        if (strcmp(buf, "return") == 0) return make_token(TOKEN_RETURN, buf, start_line, start_col);
        if (strcmp(buf, "println") == 0) return make_token(TOKEN_PRINTLN, buf, start_line, start_col);
        if (strcmp(buf, "print") == 0) return make_token(TOKEN_PRINT, buf, start_line, start_col);
        if (strcmp(buf, "unsafe") == 0) return make_token(TOKEN_UNSAFE, buf, start_line, start_col);
        if (strcmp(buf, "extern") == 0) return make_token(TOKEN_EXTERN, buf, start_line, start_col);
        if (strcmp(buf, "pub") == 0) return make_token(TOKEN_PUB, buf, start_line, start_col);
        if (strcmp(buf, "int") == 0) return make_token(TOKEN_INT, buf, start_line, start_col);
        if (strcmp(buf, "float") == 0) return make_token(TOKEN_FLOAT, buf, start_line, start_col);
        if (strcmp(buf, "bool") == 0) return make_token(TOKEN_BOOL, buf, start_line, start_col);
        if (strcmp(buf, "string") == 0) return make_token(TOKEN_STRING_TYPE, buf, start_line, start_col);
        if (strcmp(buf, "true") == 0) return make_token(TOKEN_TRUE, buf, start_line, start_col);
        if (strcmp(buf, "false") == 0) return make_token(TOKEN_FALSE, buf, start_line, start_col);

        return make_token(TOKEN_IDENTIFIER, buf, start_line, start_col);
    }

    // Numbers
    if (isdigit(c)) {
        char buf[MAX_TOKEN_LEN];
        size_t len = 0;
        while ((isdigit(c) || c == '.') && len < MAX_TOKEN_LEN - 1) {
            // Watch out for range operator '..'
            if (c == '.' && peek_ahead(lexer, 1) == '.') break;
            buf[len++] = advance_char(lexer);
            c = peek_char(lexer);
        }
        buf[len] = '\0';
        return make_token(TOKEN_NUMBER, buf, start_line, start_col);
    }

    // String Literals
    if (c == '"') {
        char buf[MAX_TOKEN_LEN];
        size_t len = 0;
        advance_char(lexer); // Opening quote
        while (lexer->pos < lexer->len && peek_char(lexer) != '"' && len < MAX_TOKEN_LEN - 1) {
            char ch = advance_char(lexer);
            if (ch == '\\' && peek_char(lexer) == '"') {
                buf[len++] = advance_char(lexer);
            } else {
                buf[len++] = ch;
            }
        }
        if (peek_char(lexer) == '"') advance_char(lexer); // Closing quote
        buf[len] = '\0';
        return make_token(TOKEN_STRING, buf, start_line, start_col);
    }

    // Operators & Punctuation
    advance_char(lexer); // Consume first char

    if (c == '.' && peek_char(lexer) == '.') {
        advance_char(lexer);
        return make_token(TOKEN_DOTDOT, "..", start_line, start_col);
    }
    if (c == '-' && peek_char(lexer) == '>') {
        advance_char(lexer);
        return make_token(TOKEN_ARROW, "->", start_line, start_col);
    }
    if (c == '=') {
        if (peek_char(lexer) == '=') {
            advance_char(lexer);
            return make_token(TOKEN_EQ, "==", start_line, start_col);
        }
        return make_token(TOKEN_ASSIGN, "=", start_line, start_col);
    }
    if (c == '!') {
        if (peek_char(lexer) == '=') {
            advance_char(lexer);
            return make_token(TOKEN_NEQ, "!=", start_line, start_col);
        }
    }
    if (c == '<') {
        if (peek_char(lexer) == '=') {
            advance_char(lexer);
            return make_token(TOKEN_LTE, "<=", start_line, start_col);
        }
        return make_token(TOKEN_LT, "<", start_line, start_col);
    }
    if (c == '>') {
        if (peek_char(lexer) == '=') {
            advance_char(lexer);
            return make_token(TOKEN_GTE, ">=", start_line, start_col);
        }
        return make_token(TOKEN_GT, ">", start_line, start_col);
    }

    switch (c) {
        case '(': return make_token(TOKEN_LPAREN, "(", start_line, start_col);
        case ')': return make_token(TOKEN_RPAREN, ")", start_line, start_col);
        case '{': return make_token(TOKEN_LBRACE, "{", start_line, start_col);
        case '}': return make_token(TOKEN_RBRACE, "}", start_line, start_col);
        case '[': return make_token(TOKEN_LBRACKET, "[", start_line, start_col);
        case ']': return make_token(TOKEN_RBRACKET, "]", start_line, start_col);
        case ':': return make_token(TOKEN_COLON, ":", start_line, start_col);
        case ';': return make_token(TOKEN_SEMICOLON, ";", start_line, start_col);
        case ',': return make_token(TOKEN_COMMA, ",", start_line, start_col);
        case '.': return make_token(TOKEN_DOT, ".", start_line, start_col);
        case '+': return make_token(TOKEN_PLUS, "+", start_line, start_col);
        case '-': return make_token(TOKEN_MINUS, "-", start_line, start_col);
        case '*': return make_token(TOKEN_STAR, "*", start_line, start_col);
        case '/': return make_token(TOKEN_SLASH, "/", start_line, start_col);
    }

    char unknown_buf[2] = {c, '\0'};
    return make_token(TOKEN_UNKNOWN, unknown_buf, start_line, start_col);
}
