#include "../include/bytecode.h"
#include "../include/diagnostics.h"
#include "../include/value.h"
#include "../include/gc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// Limits
// ============================================================

#define BC_CODE_CAP        65536
#define BC_MAX_CONSTS      256
#define BC_MAX_NAMES       256
#define BC_MAX_GLOBALS     256
#define BC_STACK_MAX       1024
#define BC_MAX_FUNCS       64
#define BC_MAX_PARAMS      16
#define BC_MAX_FRAMES      128
#define BC_MAX_FRAME_VARS  128

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
    OP_DEFINE_CONST,
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
    OP_LOOP,
    OP_INDEX_STR,
    OP_SLICE_STR,
    OP_LEN,
    OP_CALL,
    OP_RETURN
} OpCode;

// ============================================================
// Chunk
// ============================================================

typedef struct {
    char name[256];
    int entry;
    int arity;
    char params[BC_MAX_PARAMS][256];
} BcFunction;

typedef struct {
    uint8_t code[BC_CODE_CAP];
    int lines[BC_CODE_CAP];
    int cols[BC_CODE_CAP];
    int count;

    KValue consts[BC_MAX_CONSTS];
    int const_count;

    char names[BC_MAX_NAMES][256];
    int name_count;

    BcFunction functions[BC_MAX_FUNCS];
    int function_count;
} Chunk;

static Chunk chunk;

// ============================================================
// Loop Context
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
// Runtime State
// ============================================================

typedef struct {
    char name[256];
    KValue value;
    bool is_const;
} BcGlobal;

static BcGlobal bc_globals[BC_MAX_GLOBALS];
static int bc_global_count = 0;

static KValue bc_stack[BC_STACK_MAX];
static int bc_sp = 0;

typedef struct {
    char names[BC_MAX_FRAME_VARS][256];
    KValue values[BC_MAX_FRAME_VARS];
    bool consts[BC_MAX_FRAME_VARS];
    int count;
    int return_ip;
    int sp_before;
} BcFrame;

static BcFrame bc_frames[BC_MAX_FRAMES];
static int bc_frame_count = 0;

// ============================================================
// Errors and Value Helpers
// ============================================================

static _Noreturn void codegen_error(Token token, const char* msg) {
    diag_error(DIAG_CODEGEN, token.line, token.column, msg);
}

static _Noreturn void bc_runtime_error(int ip, const char* msg) {
    diag_error(DIAG_RUNTIME, chunk.lines[ip], chunk.cols[ip], msg);
}

static KValue bc_number(double value) {
    KValue v;
    v.type = K_VALUE_NUMBER;
    v.number = value;
    v.x = 0.0; v.y = 0.0; v.z = 0.0;
    v.boolean = false;
    for (int i = 0; i < 16; i++) v.m[i] = 0.0;
    v.px = 0.0; v.py = 0.0; v.pz = 0.0;
    v.vx = 0.0; v.vy = 0.0; v.vz = 0.0;
    v.fx = 0.0; v.fy = 0.0; v.fz = 0.0;
    v.mass = 0.0;
    v.heap = NULL;
    return v;
}

static KValue bc_bool(bool value) {
    KValue v = bc_number(0.0);
    v.type = K_VALUE_BOOL;
    v.boolean = value;
    return v;
}

static KValue bc_string(const char* text, int length) {
    KValue v = bc_number(0.0);
    v.type = K_VALUE_STRING;
    v.heap = (Obj*)gc_alloc_string(text, length);
    return v;
}

// ============================================================
// Emission
// ============================================================

static void emit_byte(uint8_t v, Token token) {
    if (chunk.count >= BC_CODE_CAP) {
        codegen_error(token, "program too large for prototype bytecode VM");
    }

    chunk.code[chunk.count] = v;
    chunk.lines[chunk.count] = token.line;
    chunk.cols[chunk.count] = token.column;
    chunk.count++;
}

static void emit_op(uint8_t op, Token token) {
    emit_byte(op, token);
}

static void emit_u16(uint16_t v, Token token) {
    emit_byte((uint8_t)(v & 0xFF), token);
    emit_byte((uint8_t)((v >> 8) & 0xFF), token);
}

static int emit_jump(uint8_t op, Token token) {
    emit_op(op, token);
    int pos = chunk.count;
    emit_u16(0, token);
    return pos;
}

static void patch_to(int pos, int target) {
    chunk.code[pos] = (uint8_t)(target & 0xFF);
    chunk.code[pos + 1] = (uint8_t)((target >> 8) & 0xFF);
}

static void patch_jump(int pos) {
    patch_to(pos, chunk.count);
}

static void emit_loop(int target, Token token) {
    emit_op(OP_LOOP, token);
    emit_u16((uint16_t)target, token);
}

static int add_const(KValue v, Token token) {
    if (chunk.const_count >= BC_MAX_CONSTS) {
        codegen_error(token, "too many constants for prototype bytecode VM");
    }

    chunk.consts[chunk.const_count] = v;
    return chunk.const_count++;
}

static int intern_name(const char* name, Token token) {
    for (int i = 0; i < chunk.name_count; i++) {
        if (strcmp(chunk.names[i], name) == 0) {
            return i;
        }
    }

    if (chunk.name_count >= BC_MAX_NAMES) {
        codegen_error(token, "too many names for prototype bytecode VM");
    }

    strncpy(chunk.names[chunk.name_count], name, 255);
    chunk.names[chunk.name_count][255] = '\0';
    return chunk.name_count++;
}

static int add_function(const char* name, int arity, Token token) {
    for (int i = 0; i < chunk.function_count; i++) {
        if (strcmp(chunk.functions[i].name, name) == 0) {
            chunk.functions[i].arity = arity;
            return i;
        }
    }

    if (chunk.function_count >= BC_MAX_FUNCS) {
        codegen_error(token, "too many functions for prototype bytecode VM");
    }

    int idx = chunk.function_count++;
    strncpy(chunk.functions[idx].name, name, 255);
    chunk.functions[idx].name[255] = '\0';
    chunk.functions[idx].entry = -1;
    chunk.functions[idx].arity = arity;
    return idx;
}

static int find_function(const char* name) {
    for (int i = 0; i < chunk.function_count; i++) {
        if (strcmp(chunk.functions[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

// ============================================================
// Scope Lookup / Definition
// ============================================================

static KValue* bc_lookup(const char* name) {
    for (int f = bc_frame_count - 1; f >= 0; f--) {
        for (int i = 0; i < bc_frames[f].count; i++) {
            if (strcmp(bc_frames[f].names[i], name) == 0) {
                return &bc_frames[f].values[i];
            }
        }
    }

    for (int i = 0; i < bc_global_count; i++) {
        if (strcmp(bc_globals[i].name, name) == 0) {
            return &bc_globals[i].value;
        }
    }

    return NULL;
}

static bool bc_is_const(const char* name) {
    for (int f = bc_frame_count - 1; f >= 0; f--) {
        for (int i = 0; i < bc_frames[f].count; i++) {
            if (strcmp(bc_frames[f].names[i], name) == 0) {
                return bc_frames[f].consts[i];
            }
        }
    }

    for (int i = 0; i < bc_global_count; i++) {
        if (strcmp(bc_globals[i].name, name) == 0) {
            return bc_globals[i].is_const;
        }
    }

    return false;
}

static void bc_define(const char* name, KValue value, bool is_const_decl, int ip) {
    if (bc_frame_count > 0) {
        BcFrame* fr = &bc_frames[bc_frame_count - 1];

        for (int i = 0; i < fr->count; i++) {
            if (strcmp(fr->names[i], name) == 0) {
                if (fr->consts[i] && !is_const_decl) {
                    char msg[300];
                    snprintf(msg, sizeof(msg), "Cannot redeclare constant '%s'", name);
                    bc_runtime_error(ip, msg);
                }

                fr->values[i] = value;
                fr->consts[i] = is_const_decl;
                return;
            }
        }

        if (fr->count >= BC_MAX_FRAME_VARS) {
            bc_runtime_error(ip, "Too many local variables in scope");
        }

        strncpy(fr->names[fr->count], name, 255);
        fr->names[fr->count][255] = '\0';
        fr->values[fr->count] = value;
        fr->consts[fr->count] = is_const_decl;
        fr->count++;
        return;
    }

    KValue* existing = bc_lookup(name);

    if (existing) {
        if (bc_is_const(name)) {
            char msg[300];
            snprintf(
                msg,
                sizeof(msg),
                is_const_decl
                    ? "Cannot redeclare constant '%s'"
                    : "Cannot assign to constant '%s'",
                name
            );
            bc_runtime_error(ip, msg);
        }

        *existing = value;
        return;
    }

    if (bc_global_count >= BC_MAX_GLOBALS) {
        bc_runtime_error(ip, "bytecode VM variable limit reached");
    }

    strncpy(bc_globals[bc_global_count].name, name, 255);
    bc_globals[bc_global_count].name[255] = '\0';
    bc_globals[bc_global_count].value = value;
    bc_globals[bc_global_count].is_const = is_const_decl;
    bc_global_count++;
}

// ============================================================
// GC Roots
// ============================================================

static void bc_mark_roots(void) {
    for (int i = 0; i < chunk.const_count; i++) {
        gc_mark_value(&chunk.consts[i]);
    }

    for (int i = 0; i < bc_global_count; i++) {
        gc_mark_value(&bc_globals[i].value);
    }

    for (int f = 0; f < bc_frame_count; f++) {
        for (int i = 0; i < bc_frames[f].count; i++) {
            gc_mark_value(&bc_frames[f].values[i]);
        }
    }

    for (int i = 0; i < bc_sp; i++) {
        gc_mark_value(&bc_stack[i]);
    }
}

static bool bc_truthy(KValue v, int ip) {
    if (v.type == K_VALUE_BOOL) {
        return v.boolean;
    }

    bc_runtime_error(ip, "condition must be a boolean");
    return false;
}

// ============================================================
// Compiler
// ============================================================

static void compile_expression(ASTNode* node);
static void compile_statement(ASTNode* node);

static void compile_expression(ASTNode* node) {
    Token line = node->token;

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            emit_op(OP_CONST, line);
            emit_byte((uint8_t)add_const(bc_number(node->token.value), line), line);
            break;
        }

        case NODE_STRING_LITERAL: {
            emit_op(OP_CONST, line);
            emit_byte(
                (uint8_t)add_const(
                    bc_string(node->token.lexeme, (int)strlen(node->token.lexeme)),
                    line
                ),
                line
            );
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
            if (node->token.type == TOKEN_AND) {
                compile_expression(node->left);
                int j = emit_jump(OP_JUMP_IF_FALSE_KEEP, line);
                compile_expression(node->right);
                patch_jump(j);
                break;
            }

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

        case NODE_INDEX: {
            compile_expression(node->left);
            compile_expression(node->right);
            emit_op(OP_INDEX_STR, line);
            break;
        }

        case NODE_SLICE: {
            compile_expression(node->left);

            if (node->right) {
                compile_expression(node->right);
            } else {
                emit_op(OP_CONST, line);
                emit_byte((uint8_t)add_const(bc_number(-1.0), line), line);
            }

            if (node->third) {
                compile_expression(node->third);
            } else {
                emit_op(OP_CONST, line);
                emit_byte((uint8_t)add_const(bc_number(-1.0), line), line);
            }

            emit_op(OP_SLICE_STR, line);
            break;
        }

        case NODE_CALL: {
            int fidx = find_function(node->token.lexeme);

            if (fidx < 0) {
                codegen_error(line, "bytecode VM: unknown function");
            }

            if (node->statement_count != chunk.functions[fidx].arity) {
                codegen_error(line, "bytecode VM: argument count mismatch");
            }

            for (int i = 0; i < node->statement_count; i++) {
                compile_expression(node->statements[i]);
            }

            emit_op(OP_CALL, line);
            emit_byte((uint8_t)fidx, line);
            break;
        }

        default:
            codegen_error(line, "bytecode VM does not support this expression yet");
    }
}

static void compile_statement(ASTNode* node) {
    Token line = node->token;

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

        case NODE_CONST: {
            compile_expression(node->left);
            emit_op(OP_DEFINE_CONST, line);
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

        case NODE_FUNCTION: {
            int arity = node->statement_count;

            if (arity > BC_MAX_PARAMS) {
                codegen_error(line, "too many parameters for prototype bytecode VM");
            }

            int fidx = add_function(node->token.lexeme, arity, line);

            for (int p = 0; p < arity; p++) {
                strncpy(
                    chunk.functions[fidx].params[p],
                    node->statements[p]->token.lexeme,
                    255
                );
                chunk.functions[fidx].params[p][255] = '\0';
            }

            int skip = emit_jump(OP_JUMP, line);

            chunk.functions[fidx].entry = chunk.count;

            compile_statement(node->left);

            emit_op(OP_CONST, line);
            emit_byte((uint8_t)add_const(bc_number(0.0), line), line);
            emit_op(OP_RETURN, line);

            patch_jump(skip);
            break;
        }

        case NODE_RETURN: {
            if (node->left) {
                compile_expression(node->left);
            } else {
                emit_op(OP_CONST, line);
                emit_byte((uint8_t)add_const(bc_number(0.0), line), line);
            }

            emit_op(OP_RETURN, line);
            break;
        }

        default:
            codegen_error(line, "bytecode VM does not support this statement yet");
    }
}

// ============================================================
// Execution
// ============================================================

static void bc_exec(bool quiet) {
    bc_sp = 0;
    bc_global_count = 0;
    bc_frame_count = 0;

    int ip = 0;

    for (;;) {
        uint8_t op = chunk.code[ip++];

        switch (op) {
            case OP_HALT:
                return;

            case OP_CONST: {
                uint8_t idx = chunk.code[ip++];

                if (bc_sp >= BC_STACK_MAX) {
                    bc_runtime_error(ip, "bytecode stack overflow");
                }

                bc_stack[bc_sp++] = chunk.consts[idx];
                break;
            }

            case OP_PRINT: {
                KValue v = bc_stack[--bc_sp];

                if (!quiet) {
                    if (v.type == K_VALUE_NUMBER) {
                        printf("%g\n", v.number);
                    } else if (v.type == K_VALUE_BOOL) {
                        printf("%s\n", v.boolean ? "true" : "false");
                    } else if (v.type == K_VALUE_STRING) {
                        printf("%s\n", ((ObjString*)v.heap)->chars);
                    } else {
                        printf("<value>\n");
                    }
                }

                break;
            }

            case OP_GET_GLOBAL: {
                uint8_t nidx = chunk.code[ip++];
                KValue* g = bc_lookup(chunk.names[nidx]);

                if (!g) {
                    bc_runtime_error(ip, "Undefined variable");
                }

                if (bc_sp >= BC_STACK_MAX) {
                    bc_runtime_error(ip, "bytecode stack overflow");
                }

                bc_stack[bc_sp++] = *g;
                break;
            }

            case OP_DEFINE_GLOBAL:
            case OP_SET_GLOBAL: {
                uint8_t nidx = chunk.code[ip++];
                KValue v = bc_stack[--bc_sp];
                bc_define(chunk.names[nidx], v, false, ip);
                break;
            }

            case OP_DEFINE_CONST: {
                uint8_t nidx = chunk.code[ip++];
                KValue v = bc_stack[--bc_sp];
                bc_define(chunk.names[nidx], v, true, ip);
                break;
            }

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV: {
                KValue b = bc_stack[--bc_sp];
                KValue a = bc_stack[--bc_sp];

                if (op == OP_ADD && a.type == K_VALUE_STRING && b.type == K_VALUE_STRING) {
                    ObjString* l = (ObjString*)a.heap;
                    ObjString* r = (ObjString*)b.heap;
                    int n = l->length + r->length;

                    char* buf = malloc((size_t)n + 1);

                    if (!buf) {
                        bc_runtime_error(ip, "Out of memory concatenating strings");
                    }

                    memcpy(buf, l->chars, (size_t)l->length);
                    memcpy(buf + l->length, r->chars, (size_t)r->length);
                    buf[n] = '\0';

                    bc_stack[bc_sp++] = bc_string(buf, n);
                    free(buf);
                    break;
                }

                if (a.type != K_VALUE_NUMBER || b.type != K_VALUE_NUMBER) {
                    bc_runtime_error(ip, "bytecode VM arithmetic expects numbers");
                }

                double r = 0.0;

                if (op == OP_ADD) r = a.number + b.number;
                else if (op == OP_SUB) r = a.number - b.number;
                else if (op == OP_MUL) r = a.number * b.number;
                else {
                    if (b.number == 0.0) {
                        bc_runtime_error(ip, "Division by zero");
                    }
                    r = a.number / b.number;
                }

                bc_stack[bc_sp++] = bc_number(r);
                break;
            }

            case OP_LT:
            case OP_GT:
            case OP_LE:
            case OP_GE: {
                KValue b = bc_stack[--bc_sp];
                KValue a = bc_stack[--bc_sp];

                if (a.type != K_VALUE_NUMBER || b.type != K_VALUE_NUMBER) {
                    bc_runtime_error(ip, "bytecode VM comparison expects numbers");
                }

                bool r = false;

                if (op == OP_LT) r = a.number < b.number;
                else if (op == OP_GT) r = a.number > b.number;
                else if (op == OP_LE) r = a.number <= b.number;
                else r = a.number >= b.number;

                bc_stack[bc_sp++] = bc_bool(r);
                break;
            }

            case OP_EQ:
            case OP_NE: {
                KValue b = bc_stack[--bc_sp];
                KValue a = bc_stack[--bc_sp];

                if (a.type == K_VALUE_STRING && b.type == K_VALUE_STRING) {
                    ObjString* l = (ObjString*)a.heap;
                    ObjString* r = (ObjString*)b.heap;

                    bool eq = l->length == r->length &&
                              memcmp(l->chars, r->chars, (size_t)l->length) == 0;

                    if (op == OP_NE) eq = !eq;

                    bc_stack[bc_sp++] = bc_bool(eq);
                    break;
                }

                if (a.type != b.type) {
                    bc_runtime_error(ip, "bytecode VM equality expects matching types");
                }

                bool eq = false;

                if (a.type == K_VALUE_NUMBER) eq = a.number == b.number;
                else if (a.type == K_VALUE_BOOL) eq = a.boolean == b.boolean;
                else bc_runtime_error(ip, "bytecode VM equality expects numbers or booleans");

                if (op == OP_NE) eq = !eq;

                bc_stack[bc_sp++] = bc_bool(eq);
                break;
            }

            case OP_NOT: {
                KValue v = bc_stack[--bc_sp];

                if (v.type != K_VALUE_BOOL) {
                    bc_runtime_error(ip, "'!' expects a boolean");
                }

                bc_stack[bc_sp++] = bc_bool(!v.boolean);
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

                KValue v = bc_stack[--bc_sp];

                if (!bc_truthy(v, ip)) {
                    ip = t;
                }

                break;
            }

            case OP_JUMP_IF_FALSE_KEEP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;

                KValue v = bc_stack[bc_sp - 1];

                if (!bc_truthy(v, ip)) {
                    ip = t;
                } else {
                    bc_sp--;
                }

                break;
            }

            case OP_JUMP_IF_TRUE_KEEP: {
                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;

                KValue v = bc_stack[bc_sp - 1];

                if (bc_truthy(v, ip)) {
                    ip = t;
                } else {
                    bc_sp--;
                }

                break;
            }

            case OP_LOOP: {
                gc_try_collect();

                uint16_t t = (uint16_t)(chunk.code[ip] | ((uint16_t)chunk.code[ip + 1] << 8));
                ip += 2;
                ip = t;
                break;
            }

            case OP_INDEX_STR: {
                KValue idx = bc_stack[--bc_sp];
                KValue base = bc_stack[--bc_sp];

                if (base.type != K_VALUE_STRING) {
                    bc_runtime_error(ip, "bytecode VM index expects a string");
                }

                if (idx.type != K_VALUE_NUMBER) {
                    bc_runtime_error(ip, "bytecode VM index expects a number");
                }

                ObjString* str = (ObjString*)base.heap;
                long i = (long)idx.number;

                if ((double)i != idx.number || i < 0 || i >= str->length) {
                    bc_runtime_error(ip, "String index out of bounds");
                }

                bc_stack[bc_sp++] = bc_string(str->chars + i, 1);
                break;
            }

            case OP_SLICE_STR: {
                KValue endv = bc_stack[--bc_sp];
                KValue startv = bc_stack[--bc_sp];
                KValue base = bc_stack[--bc_sp];

                if (base.type != K_VALUE_STRING) {
                    bc_runtime_error(ip, "bytecode VM slice expects a string");
                }

                ObjString* str = (ObjString*)base.heap;

                long start = 0;
                long end = str->length;

                if (startv.number >= 0.0) start = (long)startv.number;
                if (endv.number >= 0.0) end = (long)endv.number;

                if (start < 0 || end > str->length || start > end) {
                    bc_runtime_error(ip, "String slice out of bounds");
                }

                bc_stack[bc_sp++] = bc_string(str->chars + start, (int)(end - start));
                break;
            }

            case OP_LEN: {
                KValue v = bc_stack[--bc_sp];

                if (v.type != K_VALUE_STRING) {
                    bc_runtime_error(ip, "bytecode VM len expects a string");
                }

                bc_stack[bc_sp++] = bc_number((double)((ObjString*)v.heap)->length);
                break;
            }

            case OP_CALL: {
                uint8_t fidx = chunk.code[ip++];
                BcFunction* fn = &chunk.functions[fidx];

                if (fn->entry < 0) {
                    bc_runtime_error(ip, "bytecode VM: function body missing");
                }

                if (bc_frame_count >= BC_MAX_FRAMES) {
                    bc_runtime_error(ip, "Call stack overflow");
                }

                BcFrame* fr = &bc_frames[bc_frame_count++];

                fr->count = 0;
                fr->sp_before = bc_sp - fn->arity;
                fr->return_ip = ip;

                for (int i = 0; i < fn->arity; i++) {
                    strncpy(fr->names[i], fn->params[i], 255);
                    fr->names[i][255] = '\0';
                    fr->values[i] = bc_stack[fr->sp_before + i];
                    fr->consts[i] = false;
                    fr->count++;
                }

                bc_sp = fr->sp_before;
                ip = fn->entry;
                break;
            }

            case OP_RETURN: {
                if (bc_frame_count == 0) {
                    bc_runtime_error(ip, "return outside of function");
                }

                KValue rv = bc_stack[--bc_sp];

                bc_frame_count--;
                BcFrame* fr = &bc_frames[bc_frame_count];

                bc_sp = fr->sp_before;
                bc_stack[bc_sp++] = rv;
                ip = fr->return_ip;
                break;
            }

            default:
                bc_runtime_error(ip, "unknown bytecode opcode");
        }
    }
}

// ============================================================
// Entry
// ============================================================

static void prescan_functions(ASTNode* program) {
    for (int i = 0; i < program->statement_count; i++) {
        ASTNode* s = program->statements[i];

        if (s->type == NODE_FUNCTION) {
            int arity = s->statement_count;
            int idx = add_function(s->token.lexeme, arity, s->token);

            for (int p = 0; p < arity && p < BC_MAX_PARAMS; p++) {
                strncpy(
                    chunk.functions[idx].params[p],
                    s->statements[p]->token.lexeme,
                    255
                );
                chunk.functions[idx].params[p][255] = '\0';
            }
        }
    }
}

bool bc_compile_program(ASTNode* ast) {
    if (!ast || ast->type != NODE_PROGRAM) {
        return false;
    }

    chunk.count = 0;
    chunk.const_count = 0;
    chunk.name_count = 0;
    chunk.function_count = 0;
    current_loop = NULL;

    prescan_functions(ast);

    for (int i = 0; i < ast->statement_count; i++) {
        compile_statement(ast->statements[i]);
    }

    Token halt_tok = { TOKEN_EOF, "", 0.0, 0, 0 };
    emit_op(OP_HALT, halt_tok);

    return true;
}

int bc_instruction_count(void) {
    return chunk.count;
}

void bc_run_program(ASTNode* ast, bool quiet) {
    if (!bc_compile_program(ast)) {
        return;
    }

    gc_set_root_scanner(bc_mark_roots);
    bc_exec(quiet);
}