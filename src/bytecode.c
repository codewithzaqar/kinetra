#include "../include/bytecode.h"
#include "../include/diagnostics.h"
#include <string.h>

// ============================================================
// Limits
// ============================================================

#define BC_CODE_CAP    65536
#define BC_MAX_CONSTS  256
#define BC_MAX_NAMES   256
#define BC_MAX_GLOBALS 256
#define BC_STACK_MAX   1024

// ============================================================
// Opcodes
// ============================================================

typedef enum {
    OP_HALT,
    OP_CONST,
    OP_PRINT,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NOT,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_FALSE_KEEP,
    OP_JUMP_IF_TRUE_KEEP,
    OP_LOOP
} OpCode;

// ============================================================
// Chunk
// ============================================================

typedef struct {
    uint8_t code[BC_CODE_CAP];
    int lines[BC_CODE_CAP];
    int count;

    KValue consts[BC_MAX_CONSTS];
    int const_count;

    char names[BC_MAX_NAMES][256];
    int name_count;
} Chunk;

static Chunk chunk;

// ============================================================
// Loop Context (break / continue patching)
// ============================================================

typedef struct LoopCtx {
    struct LoopCtx* parent;
    int* breaks;
    int break_count;
    int* conts;
    int cont_count;
} LoopCtx;

static LoopCtx* current_loop = NULL;

// ============================================================
// Errors and Value Helpers
// ============================================================

static _Noreturn void codegen_error(int line, const char* msg) {
    diag_error(DIAG_CODEGEN, line, 0, msg);
}

static _Noreturn void bc_runtime_error(int line, const char* msg) {
    diag_error(DIAG_RUNTIME, line, 0, msg);
}

static KValue bc_number(double value) {
    KValue v;
    v.type = K_VALUE_NUMBER;
    v.number = value;
    v.x = 0.0; v.y = 0.0; v.z = 0.0;
    v.boolean = false;
    for (int i = 0; i < 16; i++) v.m[i] = 0.0;
    v.elements = NULL;
    v.element_count = 0;
    v.px = 0.0; v.py = 0.0; v.pz = 0.0;
    v.vx = 0.0; v.vy = 0.0; v.vz = 0.0;
    v.fx = 0.0; v.fy = 0.0; v.fz = 0.0;
    v.mass = 0.0;
    return v;
}

static KValue bc_bool(bool value) {
    KValue v = bc_number(0.0);
    v.type = K_VALUE_BOOL;
    v.boolean = value;
    return v;
}

// ============================================================
// Emission Helpers
// ============================================================

static void emit_byte(uint8_t v, int line) {
    if (chunk.count >= BC_CODE_CAP) {
        codegen_error(line, "program too large for prototype bytecode VM");
    }

    chunk.code[chunk.count] = v;
    chunk.lines[chunk.count] = line;
    chunk.count++;
}

static void emit_op(uint8_t op, int line) {
    emit_byte(op, line);
}

static void emit_u16(uint16_t v, int line) {
    emit_byte((uint8_t)(v & 0xFF), line);
    emit_byte((uint8_t)((v >> 8) & 0xFF), line);
}

static int emit_jump(uint8_t op, int line) {
    emit_op(op, line);
    int pos = chunk.count;
    emit_u16(0, line);
    return pos;
}

static void patch_to(int pos, int target) {
    chunk.code[pos] = (uint8_t)(target & 0xFF);
    chunk.code[pos + 1] = (uint8_t)((target >> 8) & 0xFF);
}

static void patch_jump(int pos) {
    patch_to(pos, chunk.count);
}

static void emit_loop(int target, int line) {
    emit_op(OP_LOOP, line);
    emit_u16((uint16_t)target, line);
}

static int add_const(KValue v, int line) {
    if (chunk.const_count >= BC_MAX_CONSTS) {
        codegen_error(line, "too many constants for prototype bytecode VM");
    }

    chunk.consts[chunk.const_count] = v;
    return chunk.const_count++;
}

static int intern_name(const char* name, int line) {
    for (int i = 0; i < chunk.name_count; i++) {
        if (strcmp(chunk.names[i], name) == 0) {
            return i;
        }
    }

    if (chunk.name_count >= BC_MAX_NAMES) {
        codegen_error(line, "too many names for prototype bytecode VM");
    }

    strncpy(chunk.names[chunk.name_count], name, 255);
    chunk.names[chunk.name_count][255] = '\0';

    return chunk.name_count++;
}

// ============================================================
// Compiler
// ============================================================

static void compile_expression(ASTNode* node);
static void compile_statement(ASTNode* node);

static void compile_expression(ASTNode* node) {
    int line = node->token.line;

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            emit_op(OP_CONST, line);
            emit_byte((uint8_t)add_const(bc_number(node->token.value), line), line);
            break;
        }

        case NODE_BOOLEAN_LITERAL: {
            emit_op(OP_CONST, line);
            emit_byte(
                (uint8_t)add_const(bc_bool(node->token.type == TOKEN_TRUE), line),
                line
            );
            break;
        }

        case NODE_VARIABLE: {
            emit_op(OP_GET_GLOBAL, line);
            emit_byte((uint8_t)intern_name(node->token.lexeme, line), line);
            break;
        }

        case NODE_UNARY_OP: {
            if (node->token.type != TOKEN_NOT) {
                codegen_error(line, "bytecode VM does not support this unary operator yet");
            }

            compile_expression(node->left);
            emit_op(OP_NOT, line);
            break;
        }

        case NODE_BINARY_OP: {
            // Short-circuit &&
            if (node->token.type == TOKEN_AND) {
                compile_expression(node->left);
                int j = emit_jump(OP_JUMP_IF_FALSE_KEEP, line);
                compile_expression(node->right);
                patch_jump(j);
                break;
            }

            // Short-circuit ||
            if (node->token.type == TOKEN_OR) {
                compile_expression(node->left);
                int j = emit_jump(OP_JUMP_IF_TRUE_KEEP, line);
                compile_expression(node->right);
                patch_jump(j);
                break;
            }

            compile_expression(node->left);
            compile_expression(node->right);

            switch (node->token.type) {
                case TOKEN_OP_ADD: emit_op(OP_ADD, line); break;
                case TOKEN_OP_SUB: emit_op(OP_SUB, line); break;
                case TOKEN_OP_MUL: emit_op(OP_MUL, line); break;
                case TOKEN_OP_DIV: emit_op(OP_DIV, line); break;
                case TOKEN_LT:     emit_op(OP_LT, line); break;
                case TOKEN_GT:     emit_op(OP_GT, line); break;
                case TOKEN_LE:     emit_op(OP_LE, line); break;
                case TOKEN_GE:     emit_op(OP_GE, line); break;
                case TOKEN_EQ:     emit_op(OP_EQ, line); break;
                case TOKEN_NE:     emit_op(OP_NE, line); break;
                default:
                    codegen_error(line, "bytecode VM does not support this operator yet");
            }

            break;
        }

        default:
            codegen_error(line, "bytecode VM does not support this expression yet");
    }
}

static void compile_statement(ASTNode* node) {
    int line = node->token.line;

    switch (node->type) {
        case NODE_PRINT: {
            compile_expression(node->left);
            emit_op(OP_PRINT, line);
            break;
        }

        case NODE_LET: {
            compile_expression(node->left);
            emit_op(OP_DEFINE_GLOBAL, line);
            emit_byte((uint8_t)intern_name(node->token.lexeme, line), line);
            break;
        }

        case NODE_ASSIGN: {
            compile_expression(node->left);
            emit_op(OP_SET_GLOBAL, line);
            emit_byte((uint8_t)intern_name(node->token.lexeme, line), line);
            break;
        }

        case NODE_BLOCK: {
            for (int i = 0; i < node->statement_count; i++) {
                compile_statement(node->statements[i]);
            }
            break;
        }

        case NODE_IF: {
            compile_expression(node->left);
            int else_jump = emit_jump(OP_JUMP_IF_FALSE, line);

            compile_statement(node->right);

            int end_jump = -1;

            if (node->third) {
                end_jump = emit_jump(OP_JUMP, line);
                patch_jump(else_jump);
                compile_statement(node->third);
            } else {
                patch_jump(else_jump);
            }

            if (end_jump >= 0) {
                patch_jump(end_jump);
            }

            break;
        }

        case NODE_WHILE: {
            int loop_start = chunk.count;

            compile_expression(node->left);
            int exit_jump = emit_jump(OP_JUMP_IF_FALSE, line);

            LoopCtx ctx;
            ctx.parent = current_loop;
            ctx.breaks = NULL;
            ctx.break_count = 0;
            ctx.conts = NULL;
            ctx.cont_count = 0;
            current_loop = &ctx;

            compile_statement(node->right);

            emit_loop(loop_start, line);
            patch_jump(exit_jump);

            for (int i = 0; i < ctx.break_count; i++) {
                patch_jump(ctx.breaks[i]);
            }

            for (int i = 0; i < ctx.cont_count; i++) {
                patch_to(ctx.conts[i], loop_start);
            }

            free(ctx.breaks);
            free(ctx.conts);
            current_loop = ctx.parent;

            break;
        }

        case NODE_BREAK: {
            if (!current_loop) {
                codegen_error(line, "break outside of loop");
            }

            int j = emit_jump(OP_JUMP, line);
            current_loop->breaks = realloc(
                current_loop->breaks,
                sizeof(int) * (current_loop->break_count + 1)
            );
            current_loop->breaks[current_loop->break_count++] = j;
            break;
        }

        case NODE_CONTINUE: {
            if (!current_loop) {
                codegen_error(line, "continue outside of loop");
            }

            int j = emit_jump(OP_JUMP, line);
            current_loop->conts = realloc(
                current_loop->conts,
                sizeof(int) * (current_loop->cont_count + 1)
            );
            current_loop->conts[current_loop->cont_count++] = j;
            break;
        }

        default:
            codegen_error(line, "bytecode VM does not support this statement yet");
    }
}

// ============================================================
// Bytecode VM Globals
// ============================================================

typedef struct {
    char name[256];
    KValue value;
} BcGlobal;

static BcGlobal bc_globals[BC_MAX_GLOBALS];
static int bc_global_count = 0;

static KValue* bc_find(const char* name) {
    for (int i = 0; i < bc_global_count; i++) {
        if (strcmp(bc_globals[i].name, name) == 0) {
            return &bc_globals[i].value;
        }
    }

    return NULL;
}

static void bc_set(const char* name, KValue value, int line) {
    KValue* existing = bc_find(name);

    if (existing) {
        *existing = value;
        return;
    }

    if (bc_global_count >= BC_MAX_GLOBALS) {
        bc_runtime_error(line, "bytecode VM variable limit reached");
    }

    strncpy(bc_globals[bc_global_count].name, name, 255);
    bc_globals[bc_global_count].name[255] = '\0';
    bc_globals[bc_global_count].value = value;
    bc_global_count++;
}

static bool bc_truthy(KValue v, int line) {
    if (v.type == K_VALUE_BOOL) {
        return v.boolean;
    }

    bc_runtime_error(line, "condition must be a boolean");
    return false;
}

// ============================================================
// Bytecode VM Execution
// ============================================================

static void bc_exec(bool quiet) {
    KValue stack[BC_STACK_MAX];
    int sp = 0;
    int ip = 0;

    bc_global_count = 0;

    for (;;) {
        int line = chunk.lines[ip];
        uint8_t op = chunk.code[ip++];

        switch (op) {
            case OP_HALT:
                return;

            case OP_CONST: {
                uint8_t idx = chunk.code[ip++];

                if (sp >= BC_STACK_MAX) {
                    bc_runtime_error(line, "bytecode stack overflow");
                }

                stack[sp++] = chunk.consts[idx];
                break;
            }

            case OP_PRINT: {
                KValue v = stack[--sp];

                if (!quiet) {
                    if (v.type == K_VALUE_NUMBER) {
                        printf("[Kinetra] %g\n", v.number);
                    } else if (v.type == K_VALUE_BOOL) {
                        printf("[Kinetra] %s\n", v.boolean ? "true" : "false");
                    } else {
                        printf("[Kinetra] <value>\n");
                    }
                }

                break;
            }

            case OP_GET_GLOBAL: {
                uint8_t nidx = chunk.code[ip++];
                KValue* g = bc_find(chunk.names[nidx]);

                if (!g) {
                    bc_runtime_error(line, "Undefined variable");
                }

                if (sp >= BC_STACK_MAX) {
                    bc_runtime_error(line, "bytecode stack overflow");
                }

                stack[sp++] = *g;
                break;
            }

            case OP_DEFINE_GLOBAL:
            case OP_SET_GLOBAL: {
                uint8_t nidx = chunk.code[ip++];
                KValue v = stack[--sp];
                bc_set(chunk.names[nidx], v, line);
                break;
            }

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV: {
                KValue b = stack[--sp];
                KValue a = stack[--sp];

                if (a.type != K_VALUE_NUMBER || b.type != K_VALUE_NUMBER) {
                    bc_runtime_error(line, "bytecode VM arithmetic expects numbers");
                }

                double r = 0.0;

                if (op == OP_ADD) r = a.number + b.number;
                else if (op == OP_SUB) r = a.number - b.number;
                else if (op == OP_MUL) r = a.number * b.number;
                else {
                    if (b.number == 0.0) {
                        bc_runtime_error(line, "Division by zero");
                    }
                    r = a.number / b.number;
                }

                stack[sp++] = bc_number(r);
                break;
            }

            case OP_LT:
            case OP_GT:
            case OP_LE:
            case OP_GE: {
                KValue b = stack[--sp];
                KValue a = stack[--sp];

                if (a.type != K_VALUE_NUMBER || b.type != K_VALUE_NUMBER) {
                    bc_runtime_error(line, "bytecode VM comparison expects numbers");
                }

                bool r = false;

                if (op == OP_LT) r = a.number < b.number;
                else if (op == OP_GT) r = a.number > b.number;
                else if (op == OP_LE) r = a.number <= b.number;
                else r = a.number >= b.number;

                stack[sp++] = bc_bool(r);
                break;
            }

            case OP_EQ:
            case OP_NE: {
                KValue b = stack[--sp];
                KValue a = stack[--sp];

                if (a.type != b.type) {
                    bc_runtime_error(line, "bytecode VM equality expects matching types");
                }

                bool eq = false;

                if (a.type == K_VALUE_NUMBER) eq = a.number == b.number;
                else if (a.type == K_VALUE_BOOL) eq = a.boolean == b.boolean;
                else bc_runtime_error(line, "bytecode VM equality expects numbers or booleans");

                if (op == OP_NE) eq = !eq;

                stack[sp++] = bc_bool(eq);
                break;
            }

            case OP_NOT: {
                KValue v = stack[--sp];

                if (v.type != K_VALUE_BOOL) {
                    bc_runtime_error(line, "'!' expects a boolean");
                }

                stack[sp++] = bc_bool(!v.boolean);
                break;
            }

            case OP_JUMP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;
                ip = t;
                break;
            }

            case OP_JUMP_IF_FALSE: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;

                KValue v = stack[--sp];

                if (!bc_truthy(v, line)) {
                    ip = t;
                }

                break;
            }

            case OP_JUMP_IF_FALSE_KEEP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;

                KValue v = stack[sp - 1];

                if (!bc_truthy(v, line)) {
                    ip = t;      // keep the false value as the result
                } else {
                    sp--;        // discard and evaluate the right side
                }

                break;
            }

            case OP_JUMP_IF_TRUE_KEEP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;

                KValue v = stack[sp - 1];

                if (bc_truthy(v, line)) {
                    ip = t;      // keep the true value as the result
                } else {
                    sp--;        // discard and evaluate the right side
                }

                break;
            }

            case OP_LOOP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;
                ip = t;
                break;
            }

            default:
                bc_runtime_error(line, "unknown bytecode opcode");
        }
    }
}

// ============================================================
// Entry
// ============================================================

void bc_run_program(ASTNode* ast, bool quiet) {
    if (!ast || ast->type != NODE_PROGRAM) {
        return;
    }

    chunk.count = 0;
    chunk.const_count = 0;
    chunk.name_count = 0;
    current_loop = NULL;

    for (int i = 0; i < ast->statement_count; i++) {
        compile_statement(ast->statements[i]);
    }

    emit_op(OP_HALT, 0);

    bc_exec(quiet);
}