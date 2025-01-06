#include "feeny/lexer.h"

static struct {
    const char *keyword;
    TokenType type;
} keywords[] = {
    {"var", TOKEN_VAR},
    {"defn", TOKEN_DEFN},
    {"method", TOKEN_METHOD},
    {"object", TOKEN_OBJECT},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"printf", TOKEN_PRINTF},
    {"array", TOKEN_ARRAY},
    {"null", TOKEN_NULL},
    {NULL, TOKEN_ERROR} // Sentinel
};

Lexer *init_lexer(const char *source) {
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;

    // Initialize indentation stack
    lexer->indent_capacity = 8;
    lexer->indent_stack = (int *)malloc(sizeof(int) * lexer->indent_capacity);
    lexer->indent_top = 0;
    lexer->indent_stack[0] = 0; // Base level
    lexer->decent_count = 0;

    return lexer;
}

// Helper functions
static int is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer *lexer) {
    lexer->column++;
    return *lexer->current++;
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer))
        return '\0';
    return lexer->current[1];
}

static int match(Lexer *lexer, char expected) {
    if (is_at_end(lexer))
        return 0;
    if (*lexer->current != expected)
        return 0;

    lexer->current++;
    lexer->column++;
    return 1;
}

// Create a token
static Token *make_token(Lexer *lexer, TokenType type) {
    if (type == TOKEN_ERROR) {
        fprintf(stderr, "Lexical error at line %d, column %d\n",
                lexer->line, lexer->column);
        exit(1);
    }
    Token *token = (Token *)malloc(sizeof(Token));
    token->type = type;
    token->line = lexer->line;

    // Calculate lexeme length
    int length = (int)(lexer->current - lexer->source);
    token->lexeme = (char *)malloc(length + 1);
    strncpy(token->lexeme, lexer->source, length);
    token->lexeme[length] = '\0';
    token->column = lexer->column - length;

    return token;
}

// Skip whitespace and comments
static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance(lexer);
            break;
        case ';': // Comment
            while (peek(lexer) != '\n' && !is_at_end(lexer))
                advance(lexer);
            break;

        default:
            return;
        }
    }
}

// Calculate indentation level
static int calc_indent_level(Lexer *lexer) {
    int indent = 0;
    while (peek(lexer) == ' ' || peek(lexer) == '\t') {
        if (peek(lexer) == ' ')
            indent++;
        else
            indent += 4; // Tab = 4 spaces
        advance(lexer);
    }
    if (peek(lexer) == '\n') {
        return -1;
    }
    if (indent % 4 != 0) {
        fprintf(stderr, "Error at line %d: Indentation must be a multiple of 4 spaces\n",
                lexer->line);
        exit(1);
    }
    return indent;
}

// Handle indentation
static Token *handle_indent(Lexer *lexer, int indent_level) {
    if (indent_level % 4 != 0) {
        fprintf(stderr, "Error at line %d: Indentation must be a multiple of 4 spaces\n",
                lexer->line);
        exit(1);
    }

    int current_indent = lexer->indent_stack[lexer->indent_top];

    if (indent_level > current_indent) {
        if (indent_level != current_indent + 4) {
            fprintf(stderr, "Error at line %d: Indentation must be exactly 4 spaces greater than the previous level\n",
                    lexer->line);
            exit(1);
        }
        // INDENT
        lexer->indent_top++;
        if (lexer->indent_top >= lexer->indent_capacity) {
            lexer->indent_capacity *= 2;
            lexer->indent_stack = realloc(lexer->indent_stack,
                                          sizeof(int) * lexer->indent_capacity);
        }
        lexer->indent_stack[lexer->indent_top] = indent_level;
        return make_token(lexer, TOKEN_INDENT);
    } else if (indent_level < current_indent) {
        // DEDENT
        lexer->decent_count = (current_indent - indent_level) / 4;

        while (lexer->indent_top > 0 &&
               lexer->indent_stack[lexer->indent_top] > indent_level) {
            lexer->indent_top--;
        }

        if (lexer->indent_stack[lexer->indent_top] != indent_level) {
            fprintf(stderr, "Error at line %d: Invalid dedentation level\n",
                    lexer->line);
            exit(1);
        }
        lexer->decent_count--;
        return make_token(lexer, TOKEN_DEDENT);
    }

    return NULL;
}

// Scan identifier
static Token *scan_identifier(Lexer *lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_' || peek(lexer) == '-') {
        advance(lexer);
    }
    int length = (int)(lexer->current - lexer->source);

    // Check if it's a keyword
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strlen(keywords[i].keyword) != length)
            continue;
        if (strncmp(lexer->source, keywords[i].keyword,
                    lexer->current - lexer->source) == 0) {
            return make_token(lexer, keywords[i].type);
        }
    }

    return make_token(lexer, TOKEN_IDENTIFIER);
}

// Scan number
static Token *scan_number(Lexer *lexer) {
    while (isdigit(peek(lexer)))
        advance(lexer);
    return make_token(lexer, TOKEN_INTEGER);
}

// Scan string
static Token *scan_string(Lexer *lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n')
            lexer->line++;
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        // Unterminated string
        return make_token(lexer, TOKEN_ERROR);
    }

    // The closing quote
    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}

// Get next token
Token *get_token(Lexer *lexer) {
    // Maybe still have some DEDENT tokens to return
    if (lexer->decent_count > 0) {
        lexer->decent_count--;
        return make_token(lexer, TOKEN_DEDENT);
    }

    skip_whitespace(lexer);

    lexer->source = lexer->current;

    if (is_at_end(lexer))
        return make_token(lexer, TOKEN_EOF);

    char c = advance(lexer);

    // Handle newlines and indentation
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;

        // Calculate indentation of next line
        int indent_level = calc_indent_level(lexer);
        if (indent_level == -1) {
            // Empty line
            return get_token(lexer);
        }

        Token *indent_token = handle_indent(lexer, indent_level);
        if (indent_token != NULL) {
            return indent_token;
        }

        lexer->source = lexer->current;
        return get_token(lexer);
    }

    // Identifiers
    if (isalpha(c) || c == '_')
        return scan_identifier(lexer);

    // Numbers
    if (isdigit(c))
        return scan_number(lexer);

    // Other tokens
    switch (c) {
    case ',':
        return make_token(lexer, TOKEN_COMMA);
    case '(':
        return make_token(lexer, TOKEN_LPAREN);
    case ')':
        return make_token(lexer, TOKEN_RPAREN);
    case '[':
        return make_token(lexer, TOKNE_LBRAKET);
    case ']':
        return make_token(lexer, TOKEN_RBRAKET);
    case '.':
        return make_token(lexer, TOKEN_DOT);
    case ':':
        while (peek(lexer) == ' ' || peek(lexer) == '\t')
            advance(lexer);
        if (peek(lexer) != '\n' && !is_at_end(lexer)) {
            fprintf(stderr, "Error at line %d: Colon must be followed by newline\n",
                    lexer->line);
            exit(1);
        }
        return make_token(lexer, TOKEN_COLON);
    case '=':
        if (match(lexer, '='))
            return make_token(lexer, TOKEN_EQ);
        return make_token(lexer, TOKEN_EQUAL);
    case '<':
        if (match(lexer, '='))
            return make_token(lexer, TOKEN_LE);
        return make_token(lexer, TOKEN_LT);
    case '>':
        if (match(lexer, '='))
            return make_token(lexer, TOKEN_GE);
        return make_token(lexer, TOKEN_GT);
    case '+':
        return make_token(lexer, TOKEN_PLUS);
    case '-':
        return make_token(lexer, TOKEN_MINUS);
    case '*':
        return make_token(lexer, TOKEN_STAR);
    case '/':
        return make_token(lexer, TOKEN_SLASH);
    case '%':
        return make_token(lexer, TOKEN_PERCENT);
    case '"':
        return scan_string(lexer);
    default:
        fprintf(stderr, "Error at line %d, column %d: Unexpected character '%c'\n",
                lexer->line, lexer->column - 1, c);
        exit(1);
    }

    return make_token(lexer, TOKEN_ERROR);
}

// Clean up
void free_lexer(Lexer *lexer) {
    free(lexer->indent_stack);
    free(lexer);
}

void free_token(Token *token) {
    free(token->lexeme);
    free(token);
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
    case TOKEN_VAR:
        return "VAR";
    case TOKEN_DEFN:
        return "DEFN";
    case TOKEN_METHOD:
        return "METHOD";
    case TOKEN_OBJECT:
        return "OBJECT";
    case TOKEN_IF:
        return "IF";
    case TOKEN_ELSE:
        return "ELSE";
    case TOKEN_WHILE:
        return "WHILE";
    case TOKEN_PRINTF:
        return "PRINTF";
    case TOKEN_ARRAY:
        return "ARRAY";
    case TOKEN_NULL:
        return "NULL";
    case TOKEN_INTEGER:
        return "INTEGER";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKNE_LBRAKET:
        return "LBRAKET";
    case TOKEN_RBRAKET:
        return "RBRAKET";
    case TOKEN_DOT:
        return "DOT";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_EQUAL:
        return "EQUAL";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_PERCENT:
        return "PERCENT";
    case TOKEN_LT:
        return "LT";
    case TOKEN_GT:
        return "GT";
    case TOKEN_LE:
        return "LE";
    case TOKEN_GE:
        return "GE";
    case TOKEN_EQ:
        return "EQ";
    case TOKEN_INDENT:
        return "INDENT";
    case TOKEN_DEDENT:
        return "DEDENT";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void print_token(Token *token) {
    if (token == NULL) {
        printf("Token: NULL\n");
        return;
    }

    printf("Token {");
    printf("  type: %s", token_type_to_string(token->type));
    printf("  lexeme: '%s'", token->lexeme ? token->lexeme : "NULL");
    printf("}");
}