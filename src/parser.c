#include "feeny/parser.h"
#include "feeny/ast.h"
#include "feeny/lexer.h"
#include "feeny/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_LEXER 1

static Exp *parse_expression(Parser *parser);
static ScopeStmt *parse_scope_statement(Parser *parser);
static SlotStmt *parse_slot_statement(Parser *parser);

void parser_error(Parser *parser, const char *message) {
    fprintf(stderr, "[line %d] Error at '%s': %s\n",
            parser->current->line,
            parser->current->lexeme,
            message);
    exit(1);
}

static Token *peek(Parser *parser) {
    return parser->current;
}

static Token *previous(Parser *parser) {
    return parser->previous;
}

static Token *advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = get_token(parser->lexer);
    return parser->previous;
}

static int check(Parser *parser, TokenType type) {
    return peek(parser)->type == type;
}

static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

static void consume(Parser *parser, TokenType type, const char *message) {
    print_token(peek(parser));
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    parser_error(parser, message);
}

// Parse Expression
// Primary Expression -> Integer | Null | Identifier | '(' Expression ')'
static Exp *parse_primary(Parser *parser) {
    if (match(parser, TOKEN_INTEGER)) {
        int value = atoi(previous(parser)->lexeme);
        return make_IntExp(value);
    }

    if (match(parser, TOKEN_NULL)) {
        return make_NullExp();
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        return make_RefExp(strdup(previous(parser)->lexeme));
    }

    if (match(parser, TOKEN_LPAREN)) {
        Exp *expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }

    parser_error(parser, "Expect expression.");
    return NULL;
}

static Exp *parse_call_or_slot(Parser *parser) {
    Exp *expr = parse_primary(parser);

    while (true) {
        if (match(parser, TOKEN_LPAREN)) {
            // Function call
            Vector *args = make_vector();

            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    vector_add(args, parse_expression(parser));
                } while (match(parser, TOKEN_COMMA));
            }

            consume(parser, TOKEN_RPAREN, "Expect ')' after arguments.");

            if (expr->tag == REF_EXP) {
                RefExp *ref = (RefExp *)expr;
                expr = make_CallExp(ref->name, vector_size(args), (Exp **)args->array);
            } else if (expr->tag == SLOT_EXP) {
                SlotExp *slot = (SlotExp *)expr;
                expr = make_CallSlotExp(slot->name, slot->exp, vector_size(args), (Exp **)args->array);
            } else {
                parser_error(parser, "Can only call functions and methods.");
            }
            vector_free(args);

        } else if (match(parser, TOKEN_DOT)) {
            // Slot access
            consume(parser, TOKEN_IDENTIFIER, "Expect property name after '.'.");
            char *name = strdup(previous(parser)->lexeme);

            if (match(parser, TOKEN_EQUAL)) {
                // Slot assignment
                Exp *value = parse_expression(parser);
                expr = make_SetSlotExp(name, expr, value);
            } else {
                expr = make_SlotExp(name, expr);
            }
        } else {
            break;
        }
    }

    return expr;
}

static Exp *parse_unary(Parser *parser) {
    // Currently we don't have unary operators in Feeny
    return parse_call_or_slot(parser);
}

static Exp *parse_multiplicative(Parser *parser) {
    Exp *expr = parse_unary(parser);

    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_unary(parser);

        // Convert to method call
        char *method_name;
        if (strcmp(op, "*") == 0)
            method_name = "mul";
        else if (strcmp(op, "/") == 0)
            method_name = "div";
        else
            method_name = "mod";

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup(method_name), expr, 1, args);
    }

    return expr;
}

static Exp *parse_additive(Parser *parser) {
    Exp *expr = parse_multiplicative(parser);

    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_multiplicative(parser);

        // Convert to method call
        char *method_name = (strcmp(op, "+") == 0) ? "add" : "sub";

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup(method_name), expr, 1, args);
    }

    return expr;
}

static Exp *parse_comparison(Parser *parser) {
    Exp *expr = parse_additive(parser);

    while (match(parser, TOKEN_LT) || match(parser, TOKEN_GT) ||
           match(parser, TOKEN_LE) || match(parser, TOKEN_GE)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_additive(parser);

        // Convert to method call
        char *method_name;
        if (strcmp(op, "<") == 0)
            method_name = "lt";
        else if (strcmp(op, ">") == 0)
            method_name = "gt";
        else if (strcmp(op, "<=") == 0)
            method_name = "le";
        else
            method_name = "ge";

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup(method_name), expr, 1, args);
    }

    return expr;
}

static Exp *parse_equality(Parser *parser) {
    Exp *expr = parse_comparison(parser);

    while (match(parser, TOKEN_EQ)) {
        Exp *right = parse_comparison(parser);

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup("eq"), expr, 1, args);
    }

    return expr;
}

static Exp *parse_assignment(Parser *parser) {
    Exp *expr = parse_equality(parser);

    if (match(parser, TOKEN_EQUAL)) {
        Exp *value = parse_assignment(parser);

        if (expr->tag == REF_EXP) {
            RefExp *ref = (RefExp *)expr;
            return make_SetExp(ref->name, value);
        } else if (expr->tag == SLOT_EXP) {
            SlotExp *slot = (SlotExp *)expr;
            return make_SetSlotExp(slot->name, slot->exp, value);
        }

        parser_error(parser, "Invalid assignment target.");
    }

    return expr;
}

static Exp *parse_expression(Parser *parser) {
    if (match(parser, TOKEN_IF)) {
        Exp *condition = parse_expression(parser);
        consume(parser, TOKEN_COLON, "Expect ':' after if condition.");

        consume(parser, TOKEN_INDENT, "Expect indentation after if condition.");
        ScopeStmt *then_branch = parse_scope_statement(parser);
        consume(parser, TOKEN_DEDENT, "Expect dedent after if branch.");

        ScopeStmt *else_branch = NULL;
        if (match(parser, TOKEN_ELSE)) {
            consume(parser, TOKEN_COLON, "Expect ':' after 'else'.");
            consume(parser, TOKEN_INDENT, "Expect indentation after else branch.");
            else_branch = parse_scope_statement(parser);
            consume(parser, TOKEN_DEDENT, "Expect dedent after else branch.");
        } else {
            else_branch = make_ScopeExp(make_NullExp());
        }

        return make_IfExp(condition, then_branch, else_branch);

    } else if (match(parser, TOKEN_WHILE)) {
        Exp *condition = parse_expression(parser);
        consume(parser, TOKEN_COLON, "Expect ':' after while condition.");
        ScopeStmt *body = parse_scope_statement(parser);

        return make_WhileExp(condition, body);

    } else if (match(parser, TOKEN_OBJECT)) {
        Exp *parent = NULL;
        if (!check(parser, TOKEN_COLON)) {
            parent = parse_expression(parser);
        }

        consume(parser, TOKEN_COLON, "Expect ':' after object declaration.");
        Vector *slots = make_vector();

        consume(parser, TOKEN_INDENT, "Expect indentation after object declaration.");
        while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RPAREN)) {
            vector_add(slots, parse_slot_statement(parser));
        }
        consume(parser, TOKEN_DEDENT, "Expect dedent after object declaration.");

        Exp *result = make_ObjectExp(parent, vector_size(slots), (SlotStmt **)slots->array); // 修改
        return result;
    } else if (match(parser, TOKEN_ARRAY)) {
        consume(parser, TOKEN_LPAREN, "Expect '(' after 'array'.");
        Exp *length = parse_expression(parser);
        consume(parser, TOKEN_COMMA, "Expect ',' after array length.");
        Exp *init = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after array initialization.");

        return make_ArrayExp(length, init);

    } else if (match(parser, TOKEN_PRINTF)) {
        consume(parser, TOKEN_LPAREN, "Expect '(' after 'printf'.");

        if (!check(parser, TOKEN_STRING)) {
            parser_error(parser, "Expect string literal in printf.");
        }
        const char *raw_string = advance(parser)->lexeme;

        size_t len = strlen(raw_string);
        if (len < 2) {
            parser_error(parser, "Invalid string literal.");
            return NULL;
        }

        char *format = strndup(raw_string + 1, len - 2);

        Vector *args = make_vector();
        while (match(parser, TOKEN_COMMA)) {
            vector_add(args, parse_expression(parser));
        }

        consume(parser, TOKEN_RPAREN, "Expect ')' after printf arguments.");

        Exp *result = make_PrintfExp(format, vector_size(args), (Exp **)args->array);
        return result;
    }

    return parse_assignment(parser);
}

// Parse statement
static SlotStmt *parse_slot_statement(Parser *parser) {
    if (match(parser, TOKEN_VAR)) {
        consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
        char *name = strdup(previous(parser)->lexeme);

        consume(parser, TOKEN_EQUAL, "Expect '=' after variable name.");
        Exp *initializer = parse_expression(parser);

        return make_SlotVar(name, initializer);

    } else if (match(parser, TOKEN_METHOD)) {
        consume(parser, TOKEN_IDENTIFIER, "Expect method name.");
        char *name = strdup(previous(parser)->lexeme);

        consume(parser, TOKEN_LPAREN, "Expect '(' after method name.");

        Vector *args = make_vector();

        if (!check(parser, TOKEN_RPAREN)) {
            do {
                consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
                vector_add(args, strdup(previous(parser)->lexeme));
            } while (match(parser, TOKEN_COMMA));
        }

        consume(parser, TOKEN_RPAREN, "Expect ')' after parameters.");
        consume(parser, TOKEN_COLON, "Expect ':' after method declaration.");

        ScopeStmt *body = parse_scope_statement(parser);

        return make_SlotMethod(name, vector_size(args), (char **)args->array, body);
    }

    parser_error(parser, "Expect variable or method declaration.");
    return NULL;
}

static ScopeStmt *parse_var_declaration(Parser *parser) {
    consume(parser, TOKEN_VAR, "Expect 'var' keyword.");
    consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
    char *name = strdup(previous(parser)->lexeme);

    consume(parser, TOKEN_EQUAL, "Expect '=' after variable name.");
    Exp *initializer = parse_expression(parser);

    return make_ScopeVar(name, initializer);
}

static ScopeStmt *parse_function_declaration(Parser *parser) {
    consume(parser, TOKEN_DEFN, "Expect 'defn' keyword.");
    consume(parser, TOKEN_IDENTIFIER, "Expect function name.");
    char *name = strdup(previous(parser)->lexeme);

    consume(parser, TOKEN_LPAREN, "Expect '(' after function name.");

    Vector *args = make_vector();

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
            vector_add(args, strdup(previous(parser)->lexeme));
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RPAREN, "Expect ')' after parameters.");
    consume(parser, TOKEN_COLON, "Expect ':' after function declaration.");
    consume(parser, TOKEN_INDENT, "Expect indentation after function declaration.");

    ScopeStmt *body = parse_scope_statement(parser);
    consume(parser, TOKEN_DEDENT, "Expect dedent after function body.");
    ScopeStmt *result = make_ScopeFn(name, vector_size(args), (char **)args->array, body);

    return result;
}

static ScopeStmt *parse_scope_statement(Parser *parser) {
    Vector *stmts = make_vector();
    while (1) {
        if (check(parser, TOKEN_EOF) || check(parser, TOKEN_DEDENT)) {
            break;
        }
        switch (parser->current->type) {
        case TOKEN_VAR: {
            ScopeStmt *stmt = parse_var_declaration(parser);
            stmt->tag = VAR_STMT;
            vector_add(stmts, stmt);
            break;
        }
        case TOKEN_DEFN: {
            ScopeStmt *stmt = parse_function_declaration(parser);
            stmt->tag = FN_STMT;
            vector_add(stmts, stmt);
            break;
        }
        case TOKEN_DEDENT: {
            consume(parser, TOKEN_DEDENT, "Expect dedent.");
            break;
        }
        default: {
            Exp *expr = parse_expression(parser);
            ScopeStmt *stmt = make_ScopeExp(expr);
            vector_add(stmts, stmt);
            break;
        }
        }
    }

    // zero or one statement
    if (vector_size(stmts) == 0) {
        vector_free(stmts);
        return make_ScopeExp(make_NullExp());
    }
    if (vector_size(stmts) == 1) {
        ScopeStmt *result = vector_get(stmts, 0);
        vector_free(stmts);
        return result;
    }

    // multiple statements
    ScopeStmt *result = make_ScopeSeq(
        vector_get(stmts, vector_size(stmts) - 2),
        vector_get(stmts, vector_size(stmts) - 1));
    for (int i = vector_size(stmts) - 3; i >= 0; i--) {
        result = make_ScopeSeq(vector_get(stmts, i), result);
    }
    vector_free(stmts);
    return result;
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\"\n", filename);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\"\n", filename);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\"\n", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

Parser *init_parser(const char *file_name) {
    Parser *parser = malloc(sizeof(Parser));

    char *source = read_file(file_name);
    if (source == NULL) {
        return NULL;
    }

#ifdef DEBUG_LEXER
    print_all_tokens(source);
#endif
    parser->lexer = init_lexer(source);
    parser->current = get_token(parser->lexer);
    parser->previous = NULL;
    return parser;
}

void free_parser(Parser *parser) {
    free_lexer(parser->lexer);
    free(parser);
}
ScopeStmt *parse(Parser *parser) {

    ScopeStmt *program = parse_scope_statement(parser);

    if (!check(parser, TOKEN_EOF)) {
        parser_error(parser, "Expect end of expression.");
    }

    return program;
}

void print_all_tokens(const char *source) {
    Lexer *lexer = init_lexer(source);
    Token *token;
    int token_count = 0;

    printf("\n=== Lexical Analysis Results ===\n\n");

    while (1) {
        token = get_token(lexer);
        token_count++;

        print_token(token);
        printf("\n");

        if (token->type == TOKEN_EOF) {
            free_token(token);
            break;
        }
        free_token(token);
    }

    printf("=== Total Tokens: %d ===\n\n", token_count);
    free_lexer(lexer);
}
