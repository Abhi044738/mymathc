#include "ir.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "semantic.h"

typedef struct IRInstr {
    char *text;
    struct IRInstr *next;
} IRInstr;

static IRInstr *ir_head = NULL;
static IRInstr *ir_tail = NULL;
static int temp_count = 0;
static int ir_error = 0;



void init_ir() {
    IRInstr *cur = ir_head;
    while (cur) {
        IRInstr *next = cur->next;
        free(cur->text);
        free(cur);
        cur = next;
    }
    ir_head = NULL;
    ir_tail = NULL;
    temp_count = 0;
    ir_error = 0;
}

static char* new_temp() {
    if (ir_error) return NULL;
    char *buf = malloc(16);
    if (!buf) {
        ir_error = 1;
        init_ir();
        return NULL;
    }
    sprintf(buf, "t%d", temp_count++);
    return buf;
}

static void emit(const char *fmt, ...) {
    if (ir_error || !fmt) return;

    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    IRInstr *i = malloc(sizeof(*i));
    if (!i) {
        ir_error = 1;
        init_ir();
        return;
    }
    
    i->text = strdup(buf);
    if (!i->text) {
        free(i);
        ir_error = 1;
        init_ir();
        return;
    }
    
    i->next = NULL;

    if (!ir_head) {
        ir_head = ir_tail = i;
    } else {
        ir_tail->next = i;
        ir_tail = i;
    }
}

static int is_constant(ASTNode *n) {
    return n && n->type == NODE_NUM;
}

static char* gen_ir_internal(ASTNode *n) {
    if (ir_error || !n) return NULL;

    // Try constant folding first
    if (is_constant(n)) {
        char *t = new_temp();
        if (!t) return NULL;
        emit("%s = %.15g", t, n->value);
        return t;
    }

    // Handle unary operations
    if (!n->right) {
        char *a = gen_ir_internal(n->left);
        if (!a) return NULL;
        
        char *t = new_temp();
        if (!t) {
            free(a);
            return NULL;
        }

        switch(n->type) {
            case NODE_NEG: emit("%s = -%s", t, a); break;
            case NODE_SIN: emit("%s = sin %s", t, a); break;
            case NODE_COS: emit("%s = cos %s", t, a); break;
            case NODE_TAN: emit("%s = tan %s", t, a); break;
            case NODE_LOG: emit("%s = log %s", t, a); break;
            case NODE_EXP: emit("%s = exp %s", t, a); break;
            case NODE_SQRT: emit("%s = sqrt %s", t, a); break;
            default:
                free(a);
                free(t);
                ir_error = 1;
                return NULL;
        }
        free(a);
        return t;
    }

    // Handle binary operations with constant folding
    char *a = gen_ir_internal(n->left);
    char *b = gen_ir_internal(n->right);
    if (!a || !b) {
        if (a) free(a);
        if (b) free(b);
        return NULL;
    }

    // Check if both operands are constants
    if (is_constant(n->left) && is_constant(n->right)) {
        double result = 0;
        
        int valid = 1;
        
        switch(n->type) {
            case NODE_ADD: result = n->left->value + n->right->value; break;
            case NODE_SUB: result = n->left->value - n->right->value; break;
            case NODE_MUL: result = n->left->value * n->right->value; break;
            case NODE_DIV: 
                if (n->right->value == 0) valid = 0;
                else result = n->left->value / n->right->value;
                break;
            case NODE_POW: result = pow(n->left->value, n->right->value); break;
            default: valid = 0;
        }

        if (valid) {
            char *t = new_temp();
            if (t) emit("%s = %.15g", t, result);
            free(a);
            free(b);
            return t;
        }
    }

    char *t = new_temp();
    if (!t) {
        free(a);
        free(b);
        return NULL;
    }

    const char *op = NULL;
    switch(n->type) {
        case NODE_ADD: op = "+"; break;
        case NODE_SUB: op = "-"; break;
        case NODE_MUL: op = "*"; break;
        case NODE_DIV: op = "/"; break;
        case NODE_POW: op = "^"; break;
        default:
            free(a);
            free(b);
            free(t);
            ir_error = 1;
            return NULL;
    }

    emit("%s = %s %s %s", t, a, op, b);
    
    free(a);
    free(b);
    return t;
}

char* gen_ir(ASTNode *n) {
    init_ir();  // Reset IR state for each generation
    if (cJSON_GetArraySize(get_semantic_json()) > 0) {
        ir_error = 1;
        return gen_ir_internal(n);
    }
    return gen_ir_internal(n);
}

cJSON* get_ir_json() {
    cJSON *arr = cJSON_CreateArray();
    for (IRInstr *i = ir_head; i; i = i->next) {
        cJSON_AddItemToArray(arr, cJSON_CreateString(i->text));
    }
    return arr;
}

int ir_has_error(void) {
    return ir_error;
}