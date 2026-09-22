#include "../include/kinetra.h"
#include "../include/diagnostics.h"
#include <ctype.h>

// ============================================================
// Token Constructor
// ============================================================

static int token_start_column = 1;

static Token make_token(TokenType type, const char* lexeme, double value, int line) {
    Token t;
    t.type = type;

    strncpy(t.lexeme, lexeme, 255);
    t.lexeme[255] = '\0';

    t.value = value;
    t.line = line;
    t.column = token_start_column;

    return t;
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF:  return "EOF";
        case TOKEN_NUMBER:  return "NUMBER";
        case TOKEN_IDENTIFIER:  return "IDENTIFIER";
        case TOKEN_LET:  return "LET";
        case TOKEN_PRINT:  return "PRINT";
        case TOKEN_SIM:  return "SIM";
        case TOKEN_STEP:  return "STEP";
        case TOKEN_DT:  return "DT";
        case TOKEN_IF:  return "IF";
        case TOKEN_ELSE:  return "ELSE";
        case TOKEN_TRUE:  return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_FN: return "FN";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_VEC3: return "VEC3";
        case TOKEN_MAT4: return "MAT4";
        case TOKEN_PARTICLE: return "PARTICLE";
        case TOKEN_BUILTIN: return "BUILTIN";
        case TOKEN_SIM_KEYWORD: return "SIM_KEYWORD";
        case TOKEN_MATH_KEYWORD: return "MATH_KEYWORD";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_OP_ADD: return "OP_ADD";
        case TOKEN_OP_SUB: return "OP_SUB";
        case TOKEN_OP_MUL: return "OP_MUL";
        case TOKEN_OP_DIV: return "OP_DIV";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_DOT: return "DOT";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LE: return "LE";
        case TOKEN_GE: return "GE";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// ============================================================
// Lexer
// ============================================================

Token* lex(const char* source, int* token_count) {
    Token* tokens = malloc(sizeof(Token) * MAX_TOKENS);

    if (!tokens) {
        fprintf(stderr, "[Kinetra Error] Out of memory\n");
        exit(KINETRA_EXIT_LEX);
    }

    int count = 0;
    int line = 1;
    int column = 1;
    int i = 0;
    int len = strlen(source);

    while (i < len && count < MAX_TOKENS - 1) {
        token_start_column = column;

        unsigned char c = (unsigned char)source[i];

        // Skip standard whitespace AND carriage returns (\r)
        if (isspace(c) || c == '\r') {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            i++;
            continue;
        }

        // Skip UTF-8 BOM (EF BB BF) if present
        if (
            c == 0xEF &&
            i + 2 < len &&
            (unsigned char)source[i + 1] == 0xBB &&
            (unsigned char)source[i + 2] == 0xBF
        ) {
            i += 3;
            column += 3;
            continue;
        }

        // Ignore any non-ASCII characters
        if (c > 127) {
            i++;
            column++;
            continue;
        }

        // Ignore stray ASCII control characters
        if (c < 0x20 || c == 0x7F) {
            i++;
            column++;
            continue;
        }

        // Skip // line comments
        if (c == '/' && i + 1 < len && source[i + 1] == '/') {
            while (i < len && source[i] != '\n') {
                i++;
                column++;
            }
            continue;
        }

        // Numbers
        if (isdigit(c) || (c == '.' && i + 1 < len && isdigit(source[i + 1]))) {
            char num_buf[64];
            int j = 0;

            while (i < len && (isdigit(source[i]) || source[i] == '.') && j < 63) {
                num_buf[j++] = source[i++];
                column++;
            }

            num_buf[j] = '\0';

            tokens[count++] = make_token(TOKEN_NUMBER, num_buf, atof(num_buf), line);
            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            char id_buf[256];
            int j = 0;

            while (i < len && (isalnum(source[i]) || source[i] == '_') && j < 255) {
                id_buf[j++] = source[i++];
                column++;
            }

            id_buf[j] = '\0';

            // ---- Reserved keywords ----
            if (strcmp(id_buf, "let") == 0) {
                tokens[count++] = make_token(TOKEN_LET, id_buf, 0, line);
            } else if (strcmp(id_buf, "print") == 0) {
                tokens[count++] = make_token(TOKEN_PRINT, id_buf, 0, line);
            } else if (strcmp(id_buf, "sim") == 0) {
                tokens[count++] = make_token(TOKEN_SIM, id_buf, 0, line);
            } else if (strcmp(id_buf, "step") == 0) {
                tokens[count++] = make_token(TOKEN_STEP, id_buf, 0, line);
            } else if (strcmp(id_buf, "dt") == 0) {
                tokens[count++] = make_token(TOKEN_DT, id_buf, 0, line);
            } else if (strcmp(id_buf, "if") == 0) {
                tokens[count++] = make_token(TOKEN_IF, id_buf, 0, line);
            } else if (strcmp(id_buf, "else") == 0) {
                tokens[count++] = make_token(TOKEN_ELSE, id_buf, 0, line);
            } else if (strcmp(id_buf, "true") == 0) {
                tokens[count++] = make_token(TOKEN_TRUE, id_buf, 0, line);
            } else if (strcmp(id_buf, "false") == 0) {
                tokens[count++] = make_token(TOKEN_FALSE, id_buf, 0, line);
            } else if (strcmp(id_buf, "while") == 0) {
                tokens[count++] = make_token(TOKEN_WHILE, id_buf, 0, line);
            } else if (strcmp(id_buf, "break") == 0) {
                tokens[count++] = make_token(TOKEN_BREAK, id_buf, 0, line);
            } else if (strcmp(id_buf, "continue") == 0) {
                tokens[count++] = make_token(TOKEN_CONTINUE, id_buf, 0, line);
            } else if (strcmp(id_buf, "fn") == 0) {
                tokens[count++] = make_token(TOKEN_FN, id_buf, 0, line);
            } else if (strcmp(id_buf, "parallel") == 0) {
                tokens[count++] = make_token(TOKEN_PARALLEL, id_buf, 0, line);
            } else if (strcmp(id_buf, "for") == 0) {
                tokens[count++] = make_token(TOKEN_FOR, id_buf, 0, line);
            } else if (strcmp(id_buf, "in") == 0) {
                tokens[count++] = make_token(TOKEN_IN, id_buf, 0, line);
            } else if (strcmp(id_buf, "const") == 0) {
                tokens[count++] = make_token(TOKEN_CONST, id_buf, 0, line);
            } else if (strcmp(id_buf, "return") == 0) {
                tokens[count++] = make_token(TOKEN_RETURN, id_buf, 0, line);

            // ---- Type keywords ----
            } else if (strcmp(id_buf, "vec3") == 0) {
                tokens[count++] = make_token(TOKEN_VEC3, id_buf, 0, line);
            } else if (strcmp(id_buf, "mat4") == 0) {
                tokens[count++] = make_token(TOKEN_MAT4, id_buf, 0, line);
            } else if (strcmp(id_buf, "particle") == 0) {
                tokens[count++] = make_token(TOKEN_PARTICLE, id_buf, 0, line);

            // ---- Built-in functions ----
            } else if (
                strcmp(id_buf, "dot") == 0 ||
                strcmp(id_buf, "cross") == 0 ||
                strcmp(id_buf, "length") == 0 ||
                strcmp(id_buf, "normalize") == 0 ||
                strcmp(id_buf, "sqrt") == 0 ||
                strcmp(id_buf, "abs") == 0 ||
                strcmp(id_buf, "min") == 0 ||
                strcmp(id_buf, "max") == 0 ||
                strcmp(id_buf, "sin") == 0 ||
                strcmp(id_buf, "cos") == 0 ||
                strcmp(id_buf, "tan") == 0 ||
                strcmp(id_buf, "translate") == 0 ||
                strcmp(id_buf, "rotate") == 0 ||
                strcmp(id_buf, "scale") == 0 ||
                strcmp(id_buf, "transform") == 0 ||
                strcmp(id_buf, "len") == 0 ||
                strcmp(id_buf, "integrate") == 0 ||
                strcmp(id_buf, "apply_force") == 0 ||
                strcmp(id_buf, "clear_force") == 0 ||
                strcmp(id_buf, "pow") == 0 ||
                strcmp(id_buf, "exp") == 0 ||
                strcmp(id_buf, "log") == 0 ||
                strcmp(id_buf, "floor") == 0 ||
                strcmp(id_buf, "ceil") == 0 ||
                strcmp(id_buf, "round") == 0 ||
                strcmp(id_buf, "clamp") == 0 ||
                strcmp(id_buf, "lerp") == 0 ||
                strcmp(id_buf, "reflect") == 0
            ) {
                tokens[count++] = make_token(TOKEN_BUILTIN, id_buf, 0, line);

            // ---- Plain identifier ----
            } else {
                tokens[count++] = make_token(TOKEN_IDENTIFIER, id_buf, 0, line);
            }

            continue;
        }

        // String literals: "text" with \"\\ \n \t escapes
        if (c == '\x22') {
            char buf[256];
            int j = 0;
            int start_line = line;

            i++;  //skip opening quote
            column++;

            bool closed = false;

            while (i < len) {
                char ch = source[i];

                if (ch == '\n') {
                    break; // strings may not span lines
                }

                if (ch == '\x22') {
                    closed = true;
                    i++;
                    column++;
                    break;
                }

                if (ch == '\\' && i + 1 < len) {
                    char next = source[i + 1];
                    char decoded = next;

                    if (next == 'n') decoded = '\n';
                    else if (next == 't') decoded = '\t';
                    else if (next == '\\') decoded = '\\';
                    else if (next == '\x22') decoded = '\x22';

                    if (j < 255) buf[j++] = decoded;
                    i += 2;
                    column += 2;
                    continue;
                }

                if (j < 255) buf[j++] = ch;
                i++;
                column++;
            }

            if (!closed) {
                diag_error(
                    DIAG_LEX,
                    start_line,
                    token_start_column,
                    "Unterminated string literal"
                );
            }

            buf[j] = '\0';

            tokens[count++] = make_token(TOKEN_STRING, buf, 0, start_line);
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
                if (i + 1 < len && source[i + 1] == '=') {
                    tokens[count++] = make_token(TOKEN_EQ, "==", 0, line);
                    i++;
                    column++;
                } else {
                    tokens[count++] = make_token(TOKEN_ASSIGN, "=", 0, line);
                }
                break;

            case '!':
                if (i + 1 < len && source[i + 1] == '=') {
                    tokens[count++] = make_token(TOKEN_NE, "!=", 0, line);
                    i++;
                    column++;
                } else {
                    tokens[count++] = make_token(TOKEN_NOT, "!", 0, line);
                }
                break;

            case '<':
                if (i + 1 < len && source[i + 1] == '=') {
                    tokens[count++] = make_token(TOKEN_LE, "<=", 0, line);
                    i++;
                    column++;
                } else {
                    tokens[count++] = make_token(TOKEN_LT, "<", 0, line);
                }
                break;

            case '>':
                if (i + 1 < len && source[i + 1] == '=') {
                    tokens[count++] = make_token(TOKEN_GE, ">=", 0, line);
                    i++;
                    column++;
                } else {
                    tokens[count++] = make_token(TOKEN_GT, ">", 0, line);
                }
                break;

            case '&':
                if (i + 1 < len && source[i + 1] == '&') {
                    tokens[count++] = make_token(TOKEN_AND, "&&", 0, line);
                    i++;
                    column++;
                } else {
                    diag_error(DIAG_LEX, line, column, "Single '&'; did you mean '&&'?");
                }
                break;

            case '|':
                if (i + 1 < len && source[i + 1] == '|') {
                    tokens[count++] = make_token(TOKEN_OR, "||", 0, line);
                    i++;
                    column++;
                } else {
                    diag_error(DIAG_LEX, line, column, "Single '|' did you mean '||'?");
                }
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

            case '[':
                tokens[count++] = make_token(TOKEN_LBRACKET, "[", 0, line);
                break;

            case ']':
                tokens[count++] = make_token(TOKEN_RBRACKET, "]", 0, line);
                break;

            case '.':
                tokens[count++] = make_token(TOKEN_DOT, ".", 0, line);
                break;

            case ';':
                tokens[count++] = make_token(TOKEN_SEMICOLON, ";", 0, line);
                break;

            case ',':
                tokens[count++] = make_token(TOKEN_COMMA, ",", 0, line);
                break;

            default: {
                char msg[128];
                snprintf(msg, sizeof(msg), "Unexpected character '%c' (0x%02X)", c, c);
                diag_error(DIAG_LEX, line, column, msg);
            }
        }

        i++;
        column++;
    }

    token_start_column = column;
    tokens[count++] = make_token(TOKEN_EOF, "EOF", 0, line);
    *token_count = count;

    return tokens;
}