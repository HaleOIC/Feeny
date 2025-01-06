#include "feeny/parser.h"
#include "feeny/ast.h"
#include "feeny/lexer.h"
#include "feeny/utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG_LEXER 1
// #define DEBUG_PARSER 1

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
#ifdef DEBUG_PARSER
        print_token(peek(parser));
        printf("\n");
#endif
        advance(parser);
        return 1;
    }
    return 0;
}

static void consume(Parser *parser, TokenType type, const char *message) {
#ifdef DEBUG_PARSER
    print_token(peek(parser));
    printf("\n");
#endif
    if (check(parser, type)) {
        advance(parser);
        return;
    }
    parser_error(parser, message);
}

// expr -> assign
static Exp *parse_expression(Parser *parser) {
    return parse_assign(parser);
}

// assign -> lvalue "=" assign | compare
static Exp *parse_assign(Parser *parser) {
    Exp *expr = parse_compare(parser);

    if (is_valid_lvalue(expr) && match(parser, TOKEN_EQUAL)) {
        Exp *rhs = parse_assign(parser);
        if (expr->tag == REF_EXP) {
            return make_SetExp(((RefExp *)expr)->name, rhs);
        }
        if (expr->tag == SLOT_EXP) {
            return make_SetSlotExp(((SlotExp *)expr)->name, ((SlotExp *)expr)->exp, rhs);
        }
        if (expr->tag == CALL_SLOT_EXP) {
            CallSlotExp *call = (CallSlotExp *)expr;
            if (strcmp(call->name, "get") != 0) {
                parser_error(parser, "Invalid assignment target");
            }

            Exp **args = malloc((call->nargs + 1) * sizeof(Exp *));

            for (int i = 0; i < call->nargs; i++) {
                args[i] = call->args[i];
            }
            args[call->nargs] = rhs;

            return make_CallSlotExp("set", call->exp, call->nargs + 1, args);
        }
        parser_error(parser, "Invalid assignment target");
    }

    return expr;
}

// Left-hand side value can not be a method call
// except for array access
// lvalue -> chain ( "[" expression "]" | "." IDENTIFIER )*
static Exp *parse_lvalue(Parser *parser) {
    Exp *expr = parse_chain(parser);

    while (true) {
        if (match(parser, TOKEN_LBRAKET)) {
            // a[x, y, z] => a.get(x, y, z)
            Exp **args = NULL;
            int arg_count = 0;

            do {
                Exp *index = parse_expression(parser);
                args = realloc(args, (arg_count + 1) * sizeof(Exp *));
                args[arg_count++] = index;
                if (!match(parser, TOKEN_COMMA)) {
                    break;
                }
            } while (1);

            consume(parser, TOKEN_RBRAKET, "Expect ']' after index");
            expr = make_CallSlotExp("get", expr, arg_count, args);
        } else if (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENTIFIER, "Expect property name");
            char *name = strdup(previous(parser)->lexeme);
            expr = make_SlotExp(name, expr);
        } else {
            break;
        }
    }

    return expr;
}

// Helper function: check if the expression is a valid lvalue
static bool is_valid_lvalue(Exp *expr) {
    if (expr->tag == REF_EXP) {
        return true;
    }
    if (expr->tag == SLOT_EXP) {
        return true;
    }
    if (expr->tag == CALL_SLOT_EXP) {
        CallSlotExp *call = (CallSlotExp *)expr;
        return strcmp(call->name, "get") == 0;
    }
    return false;
}

// compare-> term (("<" | "<=" | ">" | ">=" | "==") term)*
static Exp *parse_compare(Parser *parser) {
    Exp *expr = parse_term(parser);

    while (match(parser, TOKEN_LT) || match(parser, TOKEN_LE) ||
           match(parser, TOKEN_GT) || match(parser, TOKEN_GE) ||
           match(parser, TOKEN_EQ)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_term(parser);

        char *method_name;
        if (strcmp(op, "<") == 0)
            method_name = "lt";
        else if (strcmp(op, "<=") == 0)
            method_name = "le";
        else if (strcmp(op, ">") == 0)
            method_name = "gt";
        else if (strcmp(op, ">=") == 0)
            method_name = "ge";
        else
            method_name = "eq";

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup(method_name), expr, 1, args);
    }
    return expr;
}

// term -> factor (("+" | "-") factor)*
static Exp *parse_term(Parser *parser) {
    Exp *expr = parse_factor(parser);

    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_factor(parser);

        char *method_name = (strcmp(op, "+") == 0) ? "add" : "sub";

        Exp **args = malloc(sizeof(Exp *));
        args[0] = right;
        expr = make_CallSlotExp(strdup(method_name), expr, 1, args);
    }
    return expr;
}

// factor -> unary (("*" | "/" | "%") unary)*
static Exp *parse_factor(Parser *parser) {
    Exp *expr = parse_unary(parser);

    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) ||
           match(parser, TOKEN_PERCENT)) {
        char *op = previous(parser)->lexeme;
        Exp *right = parse_unary(parser);

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

// unary -> "-"* chain
static Exp *parse_unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS)) {
        Exp *expr = parse_unary(parser);

        // -a = 0 - a
        Exp **args = malloc(sizeof(Exp *));
        args[0] = expr;
        return make_CallSlotExp(strdup("sub"), make_IntExp(0), 1, args);
    }

    return parse_chain(parser);
}

// chain -> primary (("[" expr "]" | "." IDENTIFIER ("(" args? ")")? | "(" args? ")"))*
static Exp *parse_chain(Parser *parser) {
    Exp *expr = parse_primary(parser);
    if (expr->tag == PRINTF_EXP || expr->tag == ARRAY_EXP || expr->tag == OBJECT_EXP) {
        return expr;
    }

    bool is_simple_identifier = expr->tag == REF_EXP;
    bool last_was_field_access = false;

    while (true) {
        if (match(parser, TOKEN_LBRAKET)) {
            // a[x, y, z] => a.get(x, y, z)
            Exp **args = NULL;
            int arg_count = 0;

            do {
                Exp *index = parse_expression(parser);
                args = realloc(args, (arg_count + 1) * sizeof(Exp *));
                args[arg_count++] = index;
                if (!match(parser, TOKEN_COMMA)) {
                    break;
                }
            } while (1);

            consume(parser, TOKEN_RBRAKET, "Expect ']' after index");
            expr = make_CallSlotExp(strdup("get"), expr, arg_count, args);
            is_simple_identifier = false;
            last_was_field_access = true;

        } else if (match(parser, TOKEN_DOT)) {
            consume(parser, TOKEN_IDENTIFIER, "Expect property name after '.'");
            char *name = strdup(previous(parser)->lexeme);

            // Check if the next token is a left parenthesis
            if (match(parser, TOKEN_LPAREN)) {
                // if a.b(), create CallExp
                int arg_count = 0;
                Exp **args = NULL;

                if (!check(parser, TOKEN_RPAREN)) {
                    do {
                        arg_count++;
                        args = realloc(args, sizeof(Exp *) * arg_count);
                        args[arg_count - 1] = parse_expression(parser);
                    } while (match(parser, TOKEN_COMMA));
                }

                consume(parser, TOKEN_RPAREN, "Expect ')' after arguments");
                expr = make_CallSlotExp(name, expr, arg_count, args);
                last_was_field_access = false;
            } else {
                expr = make_SlotExp(name, expr);
                last_was_field_access = true;
            }
            is_simple_identifier = false;

        } else if (match(parser, TOKEN_LPAREN)) {
            if (!is_simple_identifier && !last_was_field_access) {
                parser_error(parser, "Invalid function call syntax");
            }

            int arg_count = 0;
            Exp **args = NULL;

            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    arg_count++;
                    args = realloc(args, sizeof(Exp *) * arg_count);
                    args[arg_count - 1] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
            }

            consume(parser, TOKEN_RPAREN, "Expect ')' after arguments");

            if (is_simple_identifier) {
                char *name = ((RefExp *)expr)->name;
                expr = make_CallExp(strdup(name), arg_count, args);
            } else {
                expr = make_CallExp(((SlotExp *)expr)->name, arg_count, args);
            }
            is_simple_identifier = false;
            last_was_field_access = false;

        } else {
            break;
        }
    }

    return expr;
}

// primary -> NUMBER | NULL | IDENTIFIER | "(" expr ")" | "if" expr ":" scope_stmt ("else" ":" scope_stmt)?
//          | "while" expr ":" scope_stmt | "object" (IDENTIFIER | ":") ":" INDENT slot_stmt* DEDENT
static Exp *parse_primary(Parser *parser) {
    // Number
    if (match(parser, TOKEN_INTEGER)) {
        int value = atoi(previous(parser)->lexeme);
        return make_IntExp(value);
    }

    // Null
    if (match(parser, TOKEN_NULL)) {
        return make_NullExp();
    }

    // "(" expr ")"
    if (match(parser, TOKEN_LPAREN)) {
        Exp *expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }

    // IDENTIFIER
    if (match(parser, TOKEN_IDENTIFIER)) {
        char *name = strdup(previous(parser)->lexeme);
        return make_RefExp(name);
    }

    // If expression
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
    }

    // While expression
    if (match(parser, TOKEN_WHILE)) {
        Exp *condition = parse_expression(parser);
        consume(parser, TOKEN_COLON, "Expect ':' after while condition.");
        consume(parser, TOKEN_INDENT, "Expect indentation after while condition.");
        ScopeStmt *body = parse_scope_statement(parser);
        consume(parser, TOKEN_DEDENT, "Expect dedent after while body.");

        return make_WhileExp(condition, body);
    }

    // Object expression
    if (match(parser, TOKEN_OBJECT)) {
        Exp *parent = NULL;
        if (!check(parser, TOKEN_COLON)) {
            parent = parse_expression(parser);
        }

        consume(parser, TOKEN_COLON, "Expect ':' after object declaration.");
        Vector *slots = make_vector();

        consume(parser, TOKEN_INDENT, "Expect indentation after object declaration.");
        while (!check(parser, TOKEN_DEDENT) && !check(parser, TOKEN_EOF)) {
            vector_add(slots, parse_slot_statement(parser));
        }
        consume(parser, TOKEN_DEDENT, "Expect dedent after object declaration.");

        Exp *result = make_ObjectExp(parent, vector_size(slots), (SlotStmt **)slots->array); // 修改
        return result;
    }

    // Array expression
    if (match(parser, TOKEN_ARRAY)) {
        consume(parser, TOKEN_LPAREN, "Expect '(' after 'array'.");
        Exp *length = parse_expression(parser);
        consume(parser, TOKEN_COMMA, "Expect ',' after array length.");
        Exp *init = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after array initialization.");

        return make_ArrayExp(length, init);
    }

    // Printf expression
    if (match(parser, TOKEN_PRINTF)) {
        consume(parser, TOKEN_LPAREN, "Expect '(' after 'printf'.");

        if (!match(parser, TOKEN_STRING)) {
            parser_error(parser, "Expect string literal in printf.");
        }
        char *raw_string = previous(parser)->lexeme;

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

    parser_error(parser, "Unexpected token.");
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

        consume(parser, TOKEN_INDENT, "Expect indentation after method declaration.");
        ScopeStmt *body = parse_scope_statement(parser);
        consume(parser, TOKEN_DEDENT, "Expect dedent after method body.");

        return make_SlotMethod(name, vector_size(args), (char **)args->array, body);
    }

    parser_error(parser, "Expect variable or method declaration.");
    return NULL;
}

static ScopeStmt *parse_var_declaration(Parser *parser) {
    consume(parser, TOKEN_VAR, "Expect 'var' keyword.");
    consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
    char *name = strdup(previous(parser)->lexeme);
    if (strcmp(name, "this") == 0) {
        parser_error(parser, "'this' is a reserved keyword.");
    }

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
        return make_ScopeExp(make_NullExp());
    }
    if (vector_size(stmts) == 1) {
        ScopeStmt *result = vector_get(stmts, 0);
        return result;
    }

    // multiple statements
    ScopeStmt *result = make_ScopeSeq(
        vector_get(stmts, vector_size(stmts) - 2),
        vector_get(stmts, vector_size(stmts) - 1));
    for (int i = vector_size(stmts) - 3; i >= 0; i--) {
        result = make_ScopeSeq(vector_get(stmts, i), result);
    }
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
