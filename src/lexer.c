#include "../include/kinetra.h"
#include <ctype.h>

static Token make_token(TokenType type, const char* lexeme, double value, int line) {
    Token t;
    t.type = type;
    strncpy(t.lexeme, lexeme, 255);
    t.lexeme[255] = '\0';
    t.value = value;
    t.line = line;
    return t;
}

Token* lex(const char* source, int* token_count) {
    Token* tokens = malloc(sizeof(Token) * MAX_TOKENS);
    int count = 0;
    int line = 1;
    int i = 0;
    int len = strlen(source);

    while (i < len && count < MAX_TOKENS - 1) {
        char c = source[i];

        // Skip whitespace
        if (isspace(c)) {
            if (c == '\n') line++;
            i++;
            continue;
        }

        // Numbers (Support for floats/doubles for math)
        if (isdigit(c) || (c == '.' && isdigit(source[i+1]))) {
            char num_buf[64];
            int j = 0;
            while (i < len && (isdigit(source[i]) || source[i] == '.')) {
                num_buf[j++] = source[i++];
            }
            num_buf[j] = '\0';
            tokens[count++] = make_token(TOKEN_NUMBER, num_buf, atof(num_buf), line);
            continue;
        }

        // Identifiers & Keywords
        if (isalpha(c) || c == '_') {
            char id_buf[256];
            int j = 0;
            while (i < len && (isalnum(source[i]) || source[i] == '_')) {
                id_buf[j++] = source[i++];
            }
            id_buf[j] = '\0';

            // Check for Kinetra specific keywords
            if (strcmp(id_buf, "sim") == 0 || strcmp(id_buf, "step") == 0) {
                tokens[count++] = make_token(TOKEN_SIM_KEYWORD, id_buf, 0, line);
            } else if (strcmp(id_buf, "vec3") == 0 || strcmp(id_buf, "mat4") == 0) {
                tokens[count++] = make_token(TOKEN_MATH_KEYWORD, id_buf, 0, line);
            } else {
                tokens[count++] = make_token(TOKEN_IDENTIFIER, id_buf, 0, line);
            }
            continue;
        }

        // Operators and Punctuation
        switch (c) {
            case '+': tokens[count++] = make_token(TOKEN_OP_ADD, "+", 0, line); break;
            case '-': tokens[count++] = make_token(TOKEN_OP_SUB, "-", 0, line); break;
            case '*': tokens[count++] = make_token(TOKEN_OP_MUL, "*", 0, line); break;
            case '/': tokens[count++] = make_token(TOKEN_OP_DIV, "/", 0, line); break;
            case '(': tokens[count++] = make_token(TOKEN_LPAREN, "(", 0, line); break;
            case ')': tokens[count++] = make_token(TOKEN_RPAREN, ")", 0, line); break;
            case ';': tokens[count++] = make_token(TOKEN_SEMICOLON, ";", 0, line); break;
            default:
                fprintf(stderr, "[Lexer Error] Unexpected character '%c' at line %d\n", c, line);
                tokens[count++] = make_token(TOKEN_ERROR, "?", 0, line);
                break;
        }
        i++;
    }

    tokens[count++] = make_token(TOKEN_EOF, "EOF", 0, line);
    *token_count = count;
    return tokens;
}