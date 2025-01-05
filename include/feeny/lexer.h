#ifndef __LEXER_H__
#define __LEXER_H__
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token types
typedef enum {
    // Keywords
    TOKEN_VAR,
    TOKEN_DEFN,
    TOKEN_METHOD,
    TOKEN_OBJECT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_PRINTF,
    TOKEN_ARRAY,
    TOKEN_NULL,
    TOKEN_THIS,

    // Literals
    TOKEN_INTEGER,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_COMMA,

    // Symbols
    TOKEN_LPAREN,  // (
    TOKEN_RPAREN,  // )
    TOKNE_LBRAKET, // [
    TOKEN_RBRAKET, // ]
    TOKEN_DOT,     // .
    TOKEN_COLON,   // :
    TOKEN_EQUAL,   // =
    TOKEN_PLUS,    // +
    TOKEN_MINUS,   // -
    TOKEN_STAR,    // *
    TOKEN_SLASH,   // /
    TOKEN_PERCENT, // %
    TOKEN_LT,      // <
    TOKEN_GT,      // >
    TOKEN_LE,      // <=
    TOKEN_GE,      // >=
    TOKEN_EQ,      // ==

    // Special
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int column;
} Token;

typedef struct {
    const char *source;
    const char *current;
    int line;
    int column;
    int *indent_stack; // Stack to track indentation levels
    int indent_top;    // Top of indent stack
    int indent_capacity;
    int decent_count;
} Lexer;

Token *get_token(Lexer *lexer);
void free_lexer(Lexer *lexer);
void free_token(Token *token);
Lexer *init_lexer(const char *source);
void print_token(Token *token);
const char *token_type_to_string(TokenType type);

#endif // __LEXER_H__