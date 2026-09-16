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

    if (!tokens) {
        fprintf(stderr, "[Lexer Error] Out of memory\n");
        exit(1);
    }

    int count = 0;
    int line = 1;
    int i = 0;
    int len = strlen(source);

    while (i < len && count < MAX_TOKENS - 1) {
        // Cast to unsigned char to safely handle extended ASCII / UTF-8 bytes
        unsigned char c = (unsigned char)source[i];

        // Skip standard whitespace AND carriage returns (\r)
        if (isspace(c)) {
            if (c == '\n') line++;
            i++;
            continue;
        }

        // Ignore stray ASCII control characters (defensive only)
        if (c < 0x20 || c == 0x7F) {
            i++;
            continue;
        }

        // Skip UTF-8 BOM (EF BB BF) if present at start of file
        if (c == 0xEF && i + 2 < len && 
            (unsigned char)source[i+1] == 0xBB && 
            (unsigned char)source[i+2] == 0xBF) {
            i += 3;
            continue;
        }

        // Ignore any non-ASCII characters (smart quotes, zero-width spaces, etc.)
        if (c > 127) {
            i++;
            continue;
        }

        // Skip // line comments
        if (c == '/' && i + 1 < len && source[i + 1] == '/') {
            while (i < len && source[i] != '\n') {
                i++;
            }
            continue;
        }

        // Numbers
        if (isdigit(c) || (c == '.' && i + 1 < len && isdigit(source[i + 1]))) {
            char num_buf[64];
            int j = 0;

            while (i < len && (isdigit(source[i]) || source[i] == '.') && j < 63) {
                num_buf[j++] = source[i++];
            }

            num_buf[j] = '\0';

            tokens[count++] = make_token(
                TOKEN_NUMBER,
                num_buf,
                atof(num_buf),
                line
            );

            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            char id_buf[256];
            int j = 0;

            while (i < len && (isalnum(source[i]) || source[i] == '_') && j < 255) {
                id_buf[j++] = source[i++];
            }

            id_buf[j] = '\0';

            if (strcmp(id_buf, "let") == 0) {
                tokens[count++] = make_token(TOKEN_LET, id_buf, 0, line);
            } else if (strcmp(id_buf, "print") == 0) {
                tokens[count++] = make_token(TOKEN_PRINT, id_buf, 0, line);
            } else if (strcmp(id_buf, "sim") == 0) {
                tokens[count++] = make_token(TOKEN_SIM, id_buf, 0, line);
            } else if (strcmp(id_buf, "step") == 0) {
                tokens[count++] = make_token(TOKEN_STEP, id_buf, 0, line);
            } else if (strcmp(id_buf, "vec3") == 0) {
                tokens[count++] = make_token(TOKEN_VEC3, id_buf, 0, line);
            } else if (
                strcmp(id_buf, "dot") == 0 ||
                strcmp(id_buf, "cross") == 0 ||
                strcmp(id_buf, "length") == 0 ||
                strcmp(id_buf, "normalize") == 0
            ) {
                tokens[count++] = make_token(TOKEN_BUILTIN, id_buf, 0, line);
            } else if (strcmp(id_buf, "mat4") == 0) {
                tokens[count++] = make_token(TOKEN_MATH_KEYWORD, id_buf, 0, line);
            } else if (strcmp(id_buf, "particle") == 0 || strcmp(id_buf, "integrate") == 0) {
                tokens[count++] = make_token(TOKEN_SIM_KEYWORD, id_buf, 0, line);
            } else {
                tokens[count++] = make_token(TOKEN_IDENTIFIER, id_buf, 0, line);
            }

            continue;
        }

        // Operators and punctuation
        switch (c) {
            case '+':
                tokens[count++] = make_token(TOKEN_OP_ADD, "+", 0, line);
                break;

            case '-':
                tokens[count++] = make_token(TOKEN_OP_SUB, "-", 0, line);
                break;

            case '*':
                tokens[count++] = make_token(TOKEN_OP_MUL, "*", 0, line);
                break;

            case '/':
                tokens[count++] = make_token(TOKEN_OP_DIV, "/", 0, line);
                break;

            case '=':
                tokens[count++] = make_token(TOKEN_ASSIGN, "=", 0, line);
                break;

            case '(':
                tokens[count++] = make_token(TOKEN_LPAREN, "(", 0, line);
                break;

            case ')':
                tokens[count++] = make_token(TOKEN_RPAREN, ")", 0, line);
                break;

            case '{':
                tokens[count++] = make_token(TOKEN_LBRACE, "{", 0, line);
                break;

            case '}':
                tokens[count++] = make_token(TOKEN_RBRACE, "}", 0, line);
                break;

            case ';':
                tokens[count++] = make_token(TOKEN_SEMICOLON, ";", 0, line);
                break;

            case ',':
                tokens[count++] = make_token(TOKEN_COMMA, ",", 0, line);
                break;

            default:
                fprintf(
                    stderr,
                    "[Lexer Error] Unexpected character '%c' (0x%02X) at line %d\n",
                    c, 
                    c, 
                    line
                );
                
                tokens[count++] = make_token(TOKEN_ERROR, "?", 0, line);
                break;
        }

        i++;
    }

    tokens[count++] = make_token(TOKEN_EOF, "EOF", 0, line);
    *token_count = count;

    return tokens;
}