#include "../include/kinetra.h"
#include "../include/hpc_math.h"
#include "../include/diagnostics.h"
#include "../include/value.h"
#include "../include/gc.h"
#include <math.h>

// ============================================================
// Limits
// ============================================================

#define MAX_VARIABLES 1024
#define MAX_FRAMES 64
#define MAX_FRAME_VARS 64
#define MAX_FUNCTIONS 256
#define MAX_CALL_ARGS 64

// ============================================================
// Storage: Globals, Frames, Functions
// ============================================================

typedef struct {
    char name[256];
    KValue value;
    bool is_const;
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count = 0;

typedef struct {
    char names[MAX_FRAME_VARS][256];
    KValue values[MAX_FRAME_VARS];
    bool consts[MAX_FRAME_VARS];
    int count;
} Frame;

static Frame frames[MAX_FRAMES];
static int frame_depth = 0;

typedef struct {
    char name[256];
    ASTNode* fn_node;
} KFunction;

static KFunction functions[MAX_FUNCTIONS];
static int function_count = 0;

// ============================================================
// Flow Control Signals
// ============================================================

typedef enum {
    K_FLOW_NORMAL,
    K_FLOW_BREAK,
    K_FLOW_CONTINUE,
    K_FLOW_RETURN
} KFlowSignal;

static KFlowSignal flow_signal = K_FLOW_NORMAL;
static KValue return_value;

static void vm_mark_roots(void) {
    for (int i = 0; i < variable_count; i++) {
        gc_mark_value(&variables[i].value);
    }

    for (int f = 0; f < frame_depth; f++) {
        for (int i = 0; i < frames[f].count; i++) {
            gc_mark_value(&frames[f].values[i]);
        }
    }

    gc_mark_value(&return_value);
}

// ============================================================
// Errors
// ============================================================

static void runtime_error(const char* message, int line) {
    diag_error(DIAG_RUNTIME, line, 0, message);
}

static void runtime_error_at(const char* message, Token token) {
    diag_error(DIAG_RUNTIME, token.line, token.column, message);
}

// ============================================================
// Value Constructors and Type Checks
// ============================================================

static bool vm_quiet = false;

void vm_set_quiet(bool quiet) {
    vm_quiet = quiet;
}

void vm_reset(void) {
    variable_count = 0;
    function_count = 0;
    frame_depth = 0;
    flow_signal = K_FLOW_NORMAL;
}

static KValue make_number(double value) {
    KValue v;
    v.type = K_VALUE_NUMBER;
    v.number = value;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    v.boolean = false;

    v.px = 0.0; v.py = 0.0; v.pz = 0.0;
    v.vx = 0.0; v.vy = 0.0; v.vz = 0.0;
    v.fx = 0.0; v.fy = 0.0; v.fz = 0.0;
    v.mass = 0.0;

    for (int i = 0; i < 16; i++) v.m[i] = 0.0;

    v.heap = NULL;

    return v;
}

static KValue make_particle(Vec3 pos, Vec3 vel, double mass) {
    KValue v;
    v.type = K_VALUE_PARTICLE;
    v.number = 0.0;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    v.boolean = false;

    for (int i = 0; i < 16; i++) v.m[i] = 0.0;

    v.heap = NULL;

    v.px = pos.x; v.py = pos.y; v.pz = pos.z;
    v.vx = vel.x; v.vy = vel.y; v.vz = vel.z;
    v.fx = 0.0; v.fy = 0.0; v.fz = 0.0;
    v.mass = mass;

    return v;
}

static bool is_particle(KValue value) {
    return value.type == K_VALUE_PARTICLE;
}

static KValue make_vec3(double x, double y, double z) {
    KValue v;
    v.type = K_VALUE_VEC3;
    v.number = 0.0;
    v.x = x;
    v.y = y;
    v.z = z;
    v.boolean = false;

    for (int i = 0; i < 16; i++) v.m[i] = 0.0;

    v.heap = NULL;

    return v;
}

static KValue make_bool(bool value) {
    KValue v;
    v.type = K_VALUE_BOOL;
    v.number = 0.0;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    v.boolean = value;

    for (int i = 0; i < 16; i++) v.m[i] = 0.0;

    v.heap = NULL;
    v.heap = NULL;   

    return v;
}

static KValue make_mat4(const double* m) {
    KValue v;
    v.type = K_VALUE_MAT4;
    v.number = 0.0;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    v.boolean = false;

    for (int i = 0; i < 16; i++) v.m[i] = m[i];

    v.heap = NULL;

    return v;
}

static KValue make_array(int count) {
    KValue v = make_number(0.0);
    v.type = K_VALUE_ARRAY;
    v.heap = (Obj*)gc_alloc_array(count);
    return v;
}

static bool is_number(KValue value) {
    return value.type == K_VALUE_NUMBER;
}

static bool is_vec3(KValue value) {
    return value.type == K_VALUE_VEC3;
}

static bool is_bool(KValue value) {
    return value.type == K_VALUE_BOOL;
}

static bool is_mat4(KValue value) {
    return value.type == K_VALUE_MAT4;
}

static bool is_array(KValue value) {
    return value.type == K_VALUE_ARRAY;
}

// ============================================================
// Scoped Variable Storage
// ============================================================

static KValue* find_variable(const char* name) {
    // Innermost frame first
    for (int f = frame_depth - 1; f >= 0; f--) {
        for (int i = 0; i < frames[f].count; i++) {
            if (strcmp(frames[f].names[i], name) == 0) {
                return &frames[f].values[i];
            }
        }
    }

    // Then globals
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i].value;
        }
    }

    return NULL;
}

static void push_frame(void) {
    if (frame_depth >= MAX_FRAMES) {
        runtime_error("Scope nesting too deep", 0);
    }

    frames[frame_depth].count = 0;
    frame_depth++;
}

static void pop_frame(void) {
    frame_depth--;
}

static void define_local(const char* name, KValue value, bool is_const) {
    Frame* fr = &frames[frame_depth - 1];

    for (int i = 0; i < fr->count; i++) {
        if (strcmp(fr->names[i], name) == 0) {
            if (fr->consts[i] && !is_const) {
                char msg[300];
                snprintf(msg, sizeof(msg), "Cannot redeclare constant '%s'", name);
                runtime_error(msg, 0);
            }

            fr->values[i] = value;
            fr->consts[i] = is_const;
            return;
        }
    }

    if (fr->count >= MAX_FRAME_VARS) {
        runtime_error("Too many local variables in scope", 0);
    }

    strncpy(fr->names[fr->count], name, 255);
    fr->names[fr->count][255] = '\0';
    fr->values[fr->count] = value;
    fr->consts[fr->count] = is_const;
    fr->count++;
}

static void define_in_current_scope(const char* name, KValue value, bool is_const) {
    if (frame_depth > 0) {
        define_local(name, value, is_const);
        return;
    }

    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            if (variables[i].is_const && !is_const) {
                char msg[300];
                snprintf(msg, sizeof(msg), "Cannot redeclare constant '%s'", name);
                runtime_error(msg, 0);
            }

            variables[i].value = value;
            variables[i].is_const = is_const;
            return;
        }
    }

    if (variable_count >= MAX_VARIABLES) {
        runtime_error("Variable limit reached", 0);
    }

    strncpy(variables[variable_count].name, name, 255);
    variables[variable_count].name[255] = '\0';
    variables[variable_count].value = value;
    variables[variable_count].is_const = is_const;
    variable_count++;
}

static void set_variable(const char* name, KValue value, Token token) {
    for (int f = frame_depth - 1; f >= 0; f--) {
        for (int i = 0; i < frames[f].count; i++) {
            if (strcmp(frames[f].names[i], name) == 0) {
                if (frames[f].consts[i]) {
                    char msg[300];
                    snprintf(msg, sizeof(msg), "Cannot assign to constant '%s'", name);
                    runtime_error_at(msg, token);
                }

                frames[f].values[i] = value;
                return;
            }
        }
    }

    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            if (variables[i].is_const) {
                char msg[300];
                snprintf(msg, sizeof(msg), "Cannot assign to constant '%s'", name);
                runtime_error_at(msg, token);
            }

            variables[i].value = value;
            return;
        }
    }

    define_in_current_scope(name, value, false);
}

static void set_variable_number(const char* name, double value, Token token) {
    set_variable(name, make_number(value), token);
}

static bool variable_is_const(const char* name) {
    for (int f = frame_depth - 1; f>= 0; f--) {
        for (int i = 0; i < frames[f].count; i++) {
            if (strcmp(frames[f].names[i], name) == 0) {
                return frames[f].consts[i];
            }
        }
    }

    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].is_const;
        }
    }

    return false;
}

// ============================================================
// Function Table
// ============================================================

static KFunction* find_function(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].name, name) == 0) {
            return &functions[i];
        }
    }

    return NULL;
}

static void define_function(const char* name, ASTNode* node) {
    KFunction* existing = find_function(name);

    if (existing) {
        existing->fn_node = node;
        return;
    }

    if (function_count >= MAX_FUNCTIONS) {
        runtime_error("Function limit reached", 0);
    }

    strncpy(functions[function_count].name, name, 255);
    functions[function_count].name[255] = '\0';
    functions[function_count].fn_node = node;
    function_count++;
}

// ============================================================
// Printing (recursive for nested values)
// ============================================================

static KValue make_string(const char* text);
static bool is_string(KValue value);

static void print_value_inner(KValue value) {
    if (is_number(value)) {
        printf("%g", value.number);
    } else if (is_vec3(value)) {
        printf("vec3(%g, %g, %g)", value.x, value.y, value.z);
    } else if (is_bool(value)) {
        printf("%s", value.boolean ? "true" : "false");
    } else if (is_mat4(value)) {
        printf("mat4(");

        for (int i = 0; i < 16; i++) {
            printf("%g%s", value.m[i], i < 15 ? ", " : "");
        }

        printf(")");
    } else if (is_array(value)) {
        ObjArray* arr = (ObjArray*)value.heap;
        printf("[");
        for (int i = 0; i < arr->count; i++) {
            print_value_inner(arr->elements[i]);
            if (i + 1 < arr->count) printf(", ");
        }
        printf("]");
    } else if (is_particle(value)) {
        printf(
            "particle(position: vec3(%g, %g, %g), velocity: vec3(%g, %g, %g), force: vec3(%g, %g, %g), mass: %g",
            value.px, value.py, value.pz,
            value.vx, value.vy, value.vz,
            value.fx, value.fy, value.fz,
            value.mass
        );
    } else if (is_string(value)) {
        ObjString* str = (ObjString*)value.heap;
        printf("%s", str->chars);
    }
}

static void print_value(KValue value) {
    printf("[Kinetra] ");
    print_value_inner(value);
    printf("\n");
}

// ============================================================
// Conversions between KValue and HPC types
// ============================================================

static Vec3 kvalue_to_vec3(KValue value) {
    Vec3 v;
    v.x = value.x;
    v.y = value.y;
    v.z = value.z;
    return v;
}

static KValue vec3_to_kvalue(Vec3 v) {
    return make_vec3(v.x, v.y, v.z);
}

static Mat4 kvalue_to_mat4(KValue value) {
    Mat4 m;

    for (int i = 0; i < 16; i++) {
        m.m[i] = value.m[i];
    }

    return m;
}

static KValue mat4_to_kvalue(Mat4 m) {
    KValue v;
    v.type = K_VALUE_MAT4;
    v.number = 0.0;
    v.x = 0.0;
    v.y = 0.0;
    v.z = 0.0;
    v.boolean = false;

    for (int i = 0; i < 16; i++) {
        v.m[i] = m.m[i];
    }

    v.heap = NULL;

    return v;
}

static KValue make_string(const char* text) {
    KValue v = make_number(0.0);
    v.type = K_VALUE_STRING;
    v.heap = (Obj*)gc_alloc_string(text, strlen(text));
    return v;
}

static bool is_string(KValue value) {
    return value.type == K_VALUE_STRING;
}

// ============================================================
// Argument Validation Helpers
// ============================================================

static double require_number_arg(KValue value, const char* fn, Token token) {
    if (!is_number(value)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s() expects number arguments", fn);
        runtime_error_at(msg, token);
    }
    return value.number;
}

static bool require_bool(KValue value, const char* context, Token token) {
    if (!is_bool(value)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s expects a boolean", context);
        runtime_error_at(msg, token);
    }
    return value.boolean;
}

static int require_index(KValue value, int count, Token token) {
    if (!is_number(value)) runtime_error_at("Array index must be a number", token);
    double d = value.number;
    if (d != floor(d)) runtime_error_at("Array index must be a whole number", token);
    long idx = (long)d;
    if (idx < 0 || idx >= (long)count) runtime_error_at("Array index out of bounds", token);
    return (int)idx;
}

// ============================================================
// Forward Declarations
// ============================================================

static KValue evaluate(ASTNode* node);
static void execute_statement(ASTNode* node);
static void execute_sim(ASTNode* node);
static void execute_while(ASTNode* node);
static KValue call_user_function(ASTNode* node);

// ============================================================
// Binary Operators
// ============================================================

static KValue apply_binary_op(Token op, KValue left, KValue right) {
    switch (op.type) {
        case TOKEN_OP_ADD: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number + right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x + right.x,
                    left.y + right.y,
                    left.z + right.z
                );
            }

            if (is_string(left) && is_string(right)) {
                ObjString* l = (ObjString*)left.heap;
                ObjString* r = (ObjString*)right.heap;
                int new_len = l->length + r->length;

                char* buf = malloc(new_len + 1);
                memcpy(buf, l->chars, l->length);
                memcpy(buf + l->length, r->chars, r->length);
                buf[new_len] = '\0';

                KValue res = make_number(0.0);
                res.type = K_VALUE_STRING;
                res.heap = (Obj*)gc_alloc_string(buf, new_len);
                free(buf);
                return res;
            }

            runtime_error_at("Invalid operands to '+'", op);
            return make_number(0.0);
        }

        case TOKEN_OP_SUB: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number - right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x - right.x,
                    left.y - right.y,
                    left.z - right.z
                );
            }

            runtime_error_at("Invalid operands to '-'", op);
            return make_number(0.0);
        }

        case TOKEN_OP_MUL: {
            if (is_number(left) && is_number(right)) {
                return make_number(left.number * right.number);
            }

            if (is_vec3(left) && is_number(right)) {
                return make_vec3(
                    left.x * right.number,
                    left.y * right.number,
                    left.z * right.number
                );
            }

            if (is_number(left) && is_vec3(right)) {
                return make_vec3(
                    right.x * left.number,
                    right.y * left.number,
                    right.z * left.number
                );
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_vec3(
                    left.x * right.x,
                    left.y * right.y,
                    left.z * right.z
                );
            }

            if (is_mat4(left) && is_mat4(right)) {
                return mat4_to_kvalue(
                    mat4_mul(kvalue_to_mat4(left), kvalue_to_mat4(right))
                );
            }

            if (is_mat4(left) && is_vec3(right)) {
                return vec3_to_kvalue(
                    mat4_transform_point(kvalue_to_mat4(left), kvalue_to_vec3(right))
                );
            }

            runtime_error_at("Invalid operands to '*'", op);
            return make_number(0.0);
        }

        case TOKEN_OP_DIV: {
            if (is_number(left) && is_number(right)) {
                if (right.number == 0.0) {
                    runtime_error_at("Division by zero", op);
                }

                return make_number(left.number / right.number);
            }

            if (is_vec3(left) && is_number(right)) {
                if (right.number == 0.0) {
                    runtime_error_at("Division by zero", op);
                }

                return make_vec3(
                    left.x / right.number,
                    left.y / right.number,
                    left.z / right.number
                );
            }

            runtime_error_at("Invalid operands to '/'", op);
            return make_number(0.0);
        }

        case TOKEN_LT: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number < right.number);
            }

            runtime_error_at("Invalid operands to '<'", op);
            return make_number(0.0);
        }

        case TOKEN_GT: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number > right.number);
            }

            runtime_error_at("Invalid operands to '>'", op);
            return make_number(0.0);
        }

        case TOKEN_LE: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number <= right.number);
            }

            runtime_error_at("Invalid operands to '<='", op);
            return make_number(0.0);
        }

        case TOKEN_GE: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number >= right.number);
            }

            runtime_error_at("Invalid operands to '>='", op);
            return make_number(0.0);
        }

        case TOKEN_EQ: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number == right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_bool(
                    left.x == right.x &&
                    left.y == right.y &&
                    left.z == right.z
                );
            }

            if (is_bool(left) && is_bool(right)) {
                return make_bool(left.boolean == right.boolean);
            }

            if (is_string(left) && is_string(right)) {
                ObjString* l = (ObjString*)left.heap;
                ObjString* r = (ObjString*)right.heap;
                bool eq = (l->length == r->length) && (strcmp(l->chars, r->chars) == 0);
                if (op.type == TOKEN_NE) eq = !eq;
                return make_bool(eq);
            }

            runtime_error_at("Invalid operands to '=='", op);
            return make_number(0.0);
        }

        case TOKEN_NE: {
            if (is_number(left) && is_number(right)) {
                return make_bool(left.number != right.number);
            }

            if (is_vec3(left) && is_vec3(right)) {
                return make_bool(
                    left.x != right.x ||
                    left.y != right.y ||
                    left.z != right.z
                );
            }

            if (is_bool(left) && is_bool(right)) {
                return make_bool(left.boolean != right.boolean);
            }

            if (is_string(left) && is_string(right)) {
                ObjString* l = (ObjString*)left.heap;
                ObjString* r = (ObjString*)right.heap;
                bool eq = (l->length == r->length) && (strcmp(l->chars, r->chars) == 0);
                if (op.type == TOKEN_NE) eq = !eq;
                return make_bool(eq);
            }

            runtime_error_at("Invalid operands to '!='", op);
            return make_number(0.0);
        }

        default: {
            runtime_error_at("Unknown operator", op);
            return make_number(0.0);
        }
    }
}

// ============================================================
// User-Defined Function Calls
// ============================================================

static KValue call_user_function(ASTNode* node) {
    const char* name = node->token.lexeme;

    KFunction* fn = find_function(name);

    if (!fn) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown function '%s'", name);
        runtime_error_at(msg, node->token);
    }

    int argc = node->statement_count;
    int param_count = fn->fn_node->statement_count;

    if (argc != param_count) {
        char msg[256];
        snprintf(
            msg,
            sizeof(msg),
            "'%s' expects %d arguments, got %d",
            name,
            param_count,
            argc
        );
        runtime_error_at(msg, node->token);
    }

    if (argc > MAX_CALL_ARGS) {
        runtime_error_at("Too many arguments", node->token);
    }

    KValue arg_values[MAX_CALL_ARGS];

    for (int i = 0; i < argc; i++) {
        arg_values[i] = evaluate(node->statements[i]);
    }

    if (frame_depth >= MAX_FRAMES) {
        runtime_error_at("Call stack overflow", node->token);
    }

    // Push a new local frame and bind parameters
    frame_depth++;
    frames[frame_depth - 1].count = 0;

    for (int i = 0; i < param_count; i++) {
        define_local(
            fn->fn_node->statements[i]->token.lexeme,
            arg_values[i], false
        );
    }

    execute_statement(fn->fn_node->left);

    KValue result = make_number(0.0);

    if (flow_signal == K_FLOW_RETURN) {
        result = return_value;
        flow_signal = K_FLOW_NORMAL;
    } else if (flow_signal != K_FLOW_NORMAL) {
        runtime_error_at("break/continue outside of loop", node->token);
    }

    frame_depth--;

    return result;
}

// ============================================================
// Expression Evaluation
// ============================================================

static KValue evaluate(ASTNode* node) {
    if (!node) return make_number(0.0);

    switch (node->type) {
        case NODE_NUMBER_LITERAL: {
            return make_number(node->token.value);
        }

        case NODE_BOOLEAN_LITERAL: {
            return make_bool(node->token.type == TOKEN_TRUE);
        }

        case NODE_VARIABLE: {
            KValue* value = find_variable(node->token.lexeme);

            if (!value) {
                runtime_error_at(
                    "Undefined variable",
                    node->token
                );
            }

            return *value;
        }

        case NODE_VEC3: {
            if (!node->left || !node->right || !node->third) {
                runtime_error_at(
                    "Invalid vec3 constructor",
                    node->token
                );
            }

            KValue x = evaluate(node->left);
            KValue y = evaluate(node->right);
            KValue z = evaluate(node->third);

            if (!is_number(x) || !is_number(y) || !is_number(z)) {
                runtime_error_at(
                    "vec3 arguments must be numbers",
                    node->token
                );
            }

            return make_vec3(x.number, y.number, z.number);
        }

        case NODE_MAT4: {
            if (node->statement_count == 0) {
                return mat4_to_kvalue(mat4_identity());
            }

            double vals[16];

            for (int i = 0; i < 16; i++) {
                KValue arg = evaluate(node->statements[i]);

                if (!is_number(arg)) {
                    runtime_error_at("mat4 arguments must be numbers", node->token);
                }

                vals[i] = arg.number;
            }

            return make_mat4(vals);
        }

        case NODE_ARRAY_LITERAL: {
            int count = node->statement_count;
            KValue arr = make_array(count);
            ObjArray* obj = (ObjArray*)arr.heap;
            for (int i = 0; i < count; i++) {
                obj->elements[i] = evaluate(node->statements[i]);
            }
            return arr;
        }

        case NODE_INDEX: {
            KValue base = evaluate(node->left);
            if (is_array(base)) {
                ObjArray* arr = (ObjArray*)base.heap;
                KValue idx = evaluate(node->right);
                int i = require_index(idx, arr->count, node->token);
                return arr->elements[i];
            }
            runtime_error_at("Index operator expects an array", node->token);
            return make_number(0.0);
        }

        case NODE_BINARY_OP: {
            // Short-circuit &&
            if (node->token.type == TOKEN_AND) {
                KValue left = evaluate(node->left);
                bool lb = require_bool(left, "'&&'", node->token);

                if (!lb) {
                    return make_bool(false);
                }

                KValue right = evaluate(node->right);
                return make_bool(require_bool(right, "'&&'", node->token));
            }

            // Short-circuit ||
            if (node->token.type == TOKEN_OR) {
                KValue left = evaluate(node->left);
                bool lb = require_bool(left, "'||'", node->token);

                if (lb) {
                    return make_bool(true);
                }

                KValue right = evaluate(node->right);
                return make_bool(require_bool(right, "'||'", node->token));
            }

            KValue left = evaluate(node->left);
            KValue right = evaluate(node->right);

            return apply_binary_op(node->token, left, right);
        }

        case NODE_UNARY_OP: {
            if (node->token.type == TOKEN_NOT) {
                KValue operand = evaluate(node->left);
                return make_bool(!require_bool(operand, "'!'", node->token));
            }

            runtime_error_at("Unknown unary operator", node->token);
            return make_number(0.0);
        }

        case NODE_CALL: {
            const char* name = node->token.lexeme;

            // User-defined function call
            if (node->token.type == TOKEN_IDENTIFIER) {
                return call_user_function(node);
            }

            // ---- Built-in functions ----

            if (strcmp(name, "dot") == 0) {
                KValue a = evaluate(node->statements[0]);
                KValue b = evaluate(node->statements[1]);

                if (!is_vec3(a) || !is_vec3(b)) {
                    runtime_error_at("dot() expects vec3 arguments", node->token);
                }

                return make_number(
                    vec3_dot(kvalue_to_vec3(a), kvalue_to_vec3(b))
                );
            }

            if (strcmp(name, "cross") == 0) {
                KValue a = evaluate(node->statements[0]);
                KValue b = evaluate(node->statements[1]);

                if (!is_vec3(a) || !is_vec3(b)) {
                    runtime_error_at("cross() expects vec3 arguments", node->token);
                }

                return vec3_to_kvalue(
                    vec3_cross(kvalue_to_vec3(a), kvalue_to_vec3(b))
                );
            }

            if (strcmp(name, "length") == 0) {
                KValue a = evaluate(node->statements[0]);

                if (!is_vec3(a)) {
                    runtime_error_at("length() expects a vec3 argument", node->token);
                }

                return make_number(vec3_length(kvalue_to_vec3(a)));
            }

            if (strcmp(name, "normalize") == 0) {
                KValue a = evaluate(node->statements[0]);

                if (!is_vec3(a)) {
                    runtime_error_at("normalize() expects a vec3 argument", node->token);
                }

                Vec3 v = kvalue_to_vec3(a);
                double len = vec3_length(v);

                if (len == 0.0) {
                    runtime_error_at("normalize() of zero-length vector", node->token);
                }

                return vec3_to_kvalue(vec3_normalize(v));
            }

            if (strcmp(name, "sqrt") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                if (a < 0.0) {
                    runtime_error_at("sqrt() of negative number", node->token);
                }

                return make_number(sqrt(a));
            }

            if (strcmp(name, "abs") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(fabs(a));
            }

            if (strcmp(name, "min") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double b = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );

                return make_number(a < b ? a : b);
            }

            if (strcmp(name, "max") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double b = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );

                return make_number(a > b ? a : b);
            }

            if (strcmp(name, "sin") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(sin(a));
            }

            if (strcmp(name, "cos") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(cos(a));
            }

            if (strcmp(name, "tan") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(tan(a));
            }

            if (strcmp(name, "translate") == 0) {
                double x = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double y = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );
                double z = require_number_arg(
                    evaluate(node->statements[2]), name, node->token
                );

                return mat4_to_kvalue(mat4_translate(x, y, z));
            }

            if (strcmp(name, "scale") == 0) {
                double x = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double y = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );
                double z = require_number_arg(
                    evaluate(node->statements[2]), name, node->token
                );

                return mat4_to_kvalue(mat4_scale(x, y, z));
            }

            if (strcmp(name, "rotate") == 0) {
                KValue axis = evaluate(node->statements[0]);
                double angle = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );

                if (!is_vec3(axis)) {
                    runtime_error_at("rotate() expects a vec3 axis", node->token);
                }

                Vec3 k = kvalue_to_vec3(axis);

                if (vec3_length(k) == 0.0) {
                    runtime_error_at("rotate() axis must be non-zero", node->token);
                }

                return mat4_to_kvalue(mat4_rotate(k, angle));
            }

            if (strcmp(name, "transform") == 0) {
                KValue m = evaluate(node->statements[0]);
                KValue v = evaluate(node->statements[1]);

                if (!is_mat4(m)) {
                    runtime_error_at("transform() expects a mat4 first argument", node->token);
                }

                if (!is_vec3(v)) {
                    runtime_error_at("transform() expects a vec3 second argument", node->token);
                }

                return vec3_to_kvalue(
                    mat4_transform_point(kvalue_to_mat4(m), kvalue_to_vec3(v))
                );
            }

            if (strcmp(name, "len") == 0) {
                KValue a = evaluate(node->statements[0]);
                if (is_array(a)) {
                    ObjArray* arr = (ObjArray*)a.heap;
                    return make_number((double)arr->count);
                }

                if (is_string(a)) {
                    ObjString* str = (ObjString*)a.heap;
                    return make_number((double)str->length);
                }
                runtime_error_at("len() expects an array or string argument", node->token);

            }

            if (strcmp(name, "apply_force") == 0) {
                KValue p = evaluate(node->statements[0]);
                KValue f = evaluate(node->statements[1]);

                if (!is_particle(p)) {
                    runtime_error_at("apply_force() expects a particle", node->token);
                }

                if (!is_vec3(f)) {
                    runtime_error_at("apply_force() expects a vec3 force", node->token);
                }

                KValue r = p;
                r.fx += f.x;
                r.fy += f.y;
                r.fz += f.z;

                return r;
            }

            if (strcmp(name, "clear_force") == 0) {
                KValue p = evaluate(node->statements[0]);

                if (!is_particle(p)) {
                    runtime_error_at("clear_force() expects a particle", node->token);
                }

                KValue r = p;
                r.fx = 0.0;
                r.fy = 0.0;
                r.fz = 0.0;

                return r;
            }

            if (strcmp(name, "integrate") == 0) {
                KValue p = evaluate(node->statements[0]);
                double dt = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );

                if (!is_particle(p)) {
                    runtime_error_at("integrate() expects a particle", node->token);
                }

                if (p.mass == 0.0) {
                    runtime_error_at("integrate() on zero-mass particle", node->token);
                }

                Vec3 force = {p.fx, p.fy, p.fz};
                Vec3 accel = vec3_scale(force, 1.0 / p.mass);

                Vec3 vel = {p.vx, p.vy, p.vz};
                Vec3 new_vel = vec3_add(vel, vec3_scale(accel, dt));

                Vec3 pos = {p.px, p.py, p.pz};
                Vec3 new_pos = vec3_add(pos, vec3_scale(new_vel, dt));

                return make_particle(new_pos, new_vel, p.mass);
            }

            if (strcmp(name, "pow") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double b = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );

                double r = pow(a, b);

                if (isnan(r)) {
                    runtime_error_at("pow() result is not a number", node->token);
                }

                return make_number(r);
            }

            if (strcmp(name, "exp") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(exp(a));
            }

            if (strcmp(name, "log") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                if (a <= 0.0) {
                    runtime_error_at("log() of non-positive number", node->token);
                }

                return make_number(log(a));
            }

            if (strcmp(name, "floor") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(floor(a));
            }

            if (strcmp(name, "ceil") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(ceil(a));
            }

            if (strcmp(name, "round") == 0) {
                double a = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );

                return make_number(round(a));
            }

            if (strcmp(name, "clamp") == 0) {
                double x = require_number_arg(
                    evaluate(node->statements[0]), name, node->token
                );
                double lo = require_number_arg(
                    evaluate(node->statements[1]), name, node->token
                );
                double hi = require_number_arg(
                    evaluate(node->statements[2]), name, node->token
                );

                if (lo > hi) {
                    runtime_error_at("clamp() requires lo <= hi", node->token);
                }

                if (x < lo) return make_number(lo);
                if (x > hi) return make_number(hi);

                return make_number(x);
            }

            if (strcmp(name, "lerp") == 0) {
                KValue a = evaluate(node->statements[0]);
                KValue b = evaluate(node->statements[1]);
                double t = require_number_arg(
                    evaluate(node->statements[2]), name, node->token
                );

                if (is_number(a) && is_number(b)) {
                    return make_number(a.number + (b.number - a.number) * t);
                }

                if (is_vec3(a) && is_vec3(b)) {
                    return make_vec3(
                        a.x + (b.x - a.x) * t,
                        a.y + (b.y - a.y) * t,
                        a.z + (b.z - a.z) * t
                    );
                }

                runtime_error_at(
                    "lerp() expects matching number or vec3 arguments",
                    node->token
                );

                return make_number(0.0);
            }

            if (strcmp(name, "reflect") == 0) {
                KValue v = evaluate(node->statements[0]);
                KValue n = evaluate(node->statements[1]);

                if (!is_vec3(v) || !is_vec3(n)) {
                    runtime_error_at("reflect() expects vec3 arguments", node->token);
                }

                Vec3 vv = kvalue_to_vec3(v);
                Vec3 nn = kvalue_to_vec3(n);

                if (vec3_length(nn) == 0.0) {
                    runtime_error_at("reflect() normal must be non-zero", node->token);
                }

                double d = vec3_dot(vv, nn);

                Vec3 r = vec3_add(vv, vec3_scale(nn, -2.0 * d));

                return vec3_to_kvalue(r);
            }

            runtime_error_at("Unknown built-in function", node->token);
            return make_number(0.0);
        }

        case NODE_PRINT: {
            return evaluate(node->left);
        }

        case NODE_LET: {
            return evaluate(node->left);
        }

        case NODE_ASSIGN: {
            return evaluate(node->left);
        }

        case NODE_STEP: {
            return evaluate(node->left);
        }

        case NODE_SIMULATION_BLOCK: {
            return make_number(0.0);
        }

        case NODE_IF: {
            return make_number(0.0);
        }

        case NODE_BLOCK: {
            return make_number(0.0);
        }

        case NODE_FUNCTION: {
            return make_number(0.0);
        }

        case NODE_RETURN: {
            return make_number(0.0);
        }

        case NODE_INDEX_ASSIGN: {
            return make_number(0.0);
        }

        case NODE_PARTICLE: {
            int argc = node->statement_count;

            KValue p0 = evaluate(node->statements[0]);
            KValue v0 = evaluate(node->statements[1]);

            double mass = 1.0;

            if (argc == 3) {
                KValue m = evaluate(node->statements[2]);

                if (!is_number(m)) {
                    runtime_error_at("particle mass must be a number", node->token);
                }

                mass = m.number;
            }

            if (!is_vec3(p0) || !is_vec3(v0)) {
                runtime_error_at(
                    "particle() expects vec3 position and velocity",
                    node->token
                );
            }

            return make_particle(kvalue_to_vec3(p0), kvalue_to_vec3(v0), mass);
        }

        case NODE_MEMBER: {
            KValue base = evaluate(node->left);
            const char* member = node->token.lexeme;

            if (is_vec3(base)) {
                if (strcmp(member, "x") == 0) return make_number(base.x);
                if (strcmp(member, "y") == 0) return make_number(base.y);
                if (strcmp(member, "z") == 0) return make_number(base.z);

                runtime_error_at("Unknown vec3 member", node->token);
            }

            if (is_particle(base)) {
                if (strcmp(member, "position") == 0) {
                    return make_vec3(base.px, base.py, base.pz);
                }

                if (strcmp(member, "velocity") == 0) {
                    return make_vec3(base.vx, base.vy, base.vz);
                }

                if (strcmp(member, "force") == 0) {
                    return make_vec3(base.fx, base.fy, base.fz);
                }

                if (strcmp(member, "mass") == 0) {
                    return make_number(base.mass);
                }

                runtime_error_at("Unknown particle member", node->token);
            }

            runtime_error_at(
                "Member access expects vec3 or particle",
                node->token
            );

            return make_number(0.0);
        }

        case NODE_STRING_LITERAL: {
            return make_string(node->token.lexeme);
        }

        default: {
            return make_number(0.0);
        }
    }
}

// ============================================================
// Simulation Loops
// ============================================================

static void execute_sim(ASTNode* node) {
    double step_count = 0.0;

    // Form:
    // sim expression { ... }
    if (node->left) {
        KValue count_value = evaluate(node->left);

        if (!is_number(count_value)) {
            runtime_error_at(
                "sim step count must be a number",
                node->token
            );
        }

        step_count = count_value.number;
    }

    // Form:
    // sim { step expression; ... }
    else {
        for (int i = 0; i < node->statement_count; i++) {
            ASTNode* stmt = node->statements[i];

            if (stmt->type == NODE_STEP) {
                KValue count_value = evaluate(stmt->left);

                if (!is_number(count_value)) {
                    runtime_error_at(
                        "step count must be a number",
                        stmt->token
                    );
                }

                step_count = count_value.number;
                break;
            }
        }
    }

    long steps = (long)step_count;

    if (steps < 0) {
        steps = 0;
    }

    double dt_value = 0.0;

    // Custom dt: sim N dt X { ... }
    if (node->right) {
        KValue dt_val = evaluate(node->right);

        if (!is_number(dt_val)) {
            runtime_error_at("sim dt value must be a number", node->token);
        }

        dt_value = dt_val.number;
    }

    // Automatic dt when not specified
    else if (steps > 0) {
        dt_value = 1.0 / (double)steps;
    }

    for (long i = 0; i < steps; i++) {
        gc_try_collect();
        push_frame();

        set_variable_number("step_index", (double)i, node->token);
        set_variable_number("dt", dt_value, node->token);

        for (int j = 0; j < node->statement_count; j++) {
            ASTNode* stmt = node->statements[j];

            if (stmt->type == NODE_STEP) {
                continue;
            }

            execute_statement(stmt);

            if (flow_signal != K_FLOW_NORMAL) {
                break;
            }
        }

        pop_frame();

        if (flow_signal == K_FLOW_BREAK) {
            flow_signal = K_FLOW_NORMAL;
            break;
        }

        if (flow_signal == K_FLOW_CONTINUE) {
            flow_signal = K_FLOW_NORMAL;
            break;
        }

        if (flow_signal == K_FLOW_RETURN) {
            break;
        }
    }
}

static void execute_while(ASTNode* node) {
    for (;;) {
        gc_try_collect();
        KValue cond = evaluate(node->left);
        bool keep = require_bool(cond, "while condition", node->token);

        if (!keep) {
            break;
        }

        execute_statement(node->right);

        if (flow_signal == K_FLOW_BREAK) {
            flow_signal = K_FLOW_NORMAL;
            break;
        }

        if (flow_signal == K_FLOW_CONTINUE) {
            flow_signal = K_FLOW_NORMAL;
        }

        if (flow_signal == K_FLOW_RETURN) {
            break;
        }
    }
}

// ============================================================
// Statement Execution
// ============================================================

static void execute_statement(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PRINT: {
            KValue value = evaluate(node->left);
            
            if (!vm_quiet) {
                print_value(value);
            }

            break;
        }

        case NODE_LET: {
            KValue value = evaluate(node->left);
            define_in_current_scope(node->token.lexeme, value, false);
            break;
        }

        case NODE_ASSIGN: {
            KValue value = evaluate(node->left);
            set_variable(node->token.lexeme, value, node->token);
            break;
        }

        case NODE_INDEX_ASSIGN: {
            KValue* base = find_variable(node->token.lexeme);
            if (!base) runtime_error_at("Undefined variable", node->token);
            if (!is_array(*base)) runtime_error_at("Index assignment expects an array", node->token);
            if (variable_is_const(node->token.lexeme)) {
                char msg[300];
                snprintf(msg, sizeof(msg), "Cannot modify constant '%s'", node->token.lexeme);
                runtime_error_at(msg, node->token);
            }

            ObjArray* arr = (ObjArray*)base->heap;
            KValue idx = evaluate(node->left);
            int i = require_index(idx, arr->count, node->token);
            arr->elements[i] = evaluate(node->right);
            break;
        }

        case NODE_STEP: {
            // Step nodes are handled by execute_sim().
            break;
        }

        case NODE_SIMULATION_BLOCK: {
            execute_sim(node);
            break;
        }

        case NODE_IF: {
            KValue cond = evaluate(node->left);
            bool take_then = require_bool(cond, "if condition", node->token);

            if (take_then) {
                execute_statement(node->right);
            } else if (node->third) {
                execute_statement(node->third);
            }

            break;
        }

        case NODE_BLOCK: {
            push_frame();

            for (int i = 0; i < node->statement_count; i++) {
                execute_statement(node->statements[i]);

                if (flow_signal != K_FLOW_NORMAL) {
                    break;
                }
            }

            pop_frame();
            break;
        }

        case NODE_WHILE: {
            execute_while(node);
            break;
        }

        case NODE_BREAK: {
            flow_signal = K_FLOW_BREAK;
            break;
        }

        case NODE_CONTINUE: {
            flow_signal = K_FLOW_CONTINUE;
            break;
        }

        case NODE_RETURN: {
            return_value = node->left
                ? evaluate(node->left)
                : make_number(0.0);

            flow_signal = K_FLOW_RETURN;
            break;
        }

        case NODE_FUNCTION: {
            define_function(node->token.lexeme, node);
            break;
        }

        case NODE_PARALLEL_FOR: {
            KValue count_value = evaluate(node->left);

            if (!is_number(count_value)) {
                runtime_error_at("parallel for count must be a number", node->token);
            }

            long n = (long)count_value.number;

            if (n < 0) {
                n = 0;
            }

            const char* name = node->token.lexeme;
            ASTNode* body = node->right;

            gc_set_enabled(false);

        #ifdef _OPENMP
            #pragma omp parallel for schedule(static)
        #endif
            for (long i = 0; i< n; i++) {
                // Lane-private VM state
                int saved_depth = frame_depth;
                KFlowSignal saved_flow = flow_signal;

                frame_depth = 0;
                flow_signal = K_FLOW_NORMAL;

                frames[0].count = 0;
                frame_depth = 1;
                define_local(name, make_number((double)i), false);
                execute_sim(node);

                execute_statement(body);

                frame_depth = saved_depth;
                flow_signal = saved_flow;
            }  

            gc_set_enabled(true);
            break;
        }

        case NODE_CONST: {
            KValue value = evaluate(node->left);
            define_in_current_scope(node->token.lexeme, value, true);
            break;
        }

        default: {
            evaluate(node);
            break;
        }
    }
}

void vm_eval_and_print(ASTNode* node) {
    print_value(evaluate(node));
}

void vm_reset_flow(void) {
    flow_signal = K_FLOW_NORMAL;
}

// ============================================================
// Program Entry
// ============================================================

void execute(ASTNode* ast) {
    if (!ast || ast->type != NODE_PROGRAM) {
        return;
    }

    gc_set_root_scanner(vm_mark_roots);
    flow_signal = K_FLOW_NORMAL;

    for (int i = 0; i < ast->statement_count; i++) {
        gc_try_collect();
        execute_statement(ast->statements[i]);

        if (flow_signal == K_FLOW_RETURN) {
            runtime_error_at(
                "return outside of function",
                ast->statements[i]->token
            );
        }

        if (flow_signal != K_FLOW_NORMAL) {
            runtime_error_at(
                "break/continue outside of loop",
                ast->statements[i]->token
            );
        }
    }
}

#ifdef _OPENMP
#pragma omp threadprivate(frames, frame_depth, flow_signal, return_value)
#endif