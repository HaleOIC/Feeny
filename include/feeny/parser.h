#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "utils.h"
#include <ctype.h>
#include <stdbool.h>

typedef struct {
    Lexer *lexer;
    Token *current;
    Token *previous;
} Parser;

void parser_error(Parser *parser, const char *message);
Parser *init_parser(const char *source);
void free_parser(Parser *parser);
ScopeStmt *parse(Parser *parser);

typedef struct {
    const char *message;
    Token *token;
} ParseError;

// Forward declarations for parsing functions
static Exp *parse_expression(Parser *parser);
static ScopeStmt *parse_scope_statement(Parser *parser);
static SlotStmt *parse_slot_statement(Parser *parser);
void print_all_tokens(const char *source);

#endif // PARSER_H