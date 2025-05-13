#include "opt.h"
#include "ir.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    char *temp;
    double value;
} Constant;

static Constant constants[100];
static int const_count = 0;
static cJSON *opt_errors = NULL;  // Track optimization errors

static double get_constant(const char *temp) {
    for (int i = 0; i < const_count; i++) {
        if (strcmp(constants[i].temp, temp) == 0) {
            return constants[i].value;
        }
    }
    return NAN;
}

static void fold_constants(cJSON *opt, const char *code) {
    char out_temp[16], op[5], a_temp[16], b_temp[16];
    double a_val, b_val, result;
    
    if (sscanf(code, "%15s = %15s %4s %15s", out_temp, a_temp, op, b_temp) == 4) {
        a_val = get_constant(a_temp);
        b_val = get_constant(b_temp);
        
        if (!isnan(a_val) && !isnan(b_val)) {
            if (strcmp(op, "+") == 0) result = a_val + b_val;
            else if (strcmp(op, "-") == 0) result = a_val - b_val;
            else if (strcmp(op, "*") == 0) result = a_val * b_val;
            else if (strcmp(op, "/") == 0) {
                if (b_val == 0) {
                    cJSON_AddItemToArray(opt_errors, cJSON_CreateString("Division by zero (optimized)"));
                    cJSON_AddItemToArray(opt, cJSON_CreateString(code)); // Keep original IR
                    return;
                }
                result = a_val / b_val;
            } else return;

            char folded[32];
            sprintf(folded, "%s = %g", out_temp, result);
            cJSON_AddItemToArray(opt, cJSON_CreateString(folded));
            return;
        }
    }
    
    double value;
    if (sscanf(code, "%15s = %lf", out_temp, &value) == 2) {
        constants[const_count].temp = strdup(out_temp);
        constants[const_count].value = value;
        const_count++;
    }
    
    cJSON_AddItemToArray(opt, cJSON_CreateString(code));
}

cJSON* get_opt_json() {
    cJSON *ir = get_ir_json();
    cJSON *opt = cJSON_CreateArray();
    const_count = 0;

    // Reset optimization errors
    if (opt_errors) cJSON_Delete(opt_errors);
    opt_errors = cJSON_CreateArray();

    cJSON *instr;
    cJSON_ArrayForEach(instr, ir) {
        const char *code = cJSON_GetStringValue(instr);
        fold_constants(opt, code);
    }

    // Merge optimization errors into the main IR array (optional)
    // cJSON_AddItemToArray(opt, opt_errors);
    return opt;
}

void init_opt() {
    for (int i = 0; i < const_count; i++) {
        free(constants[i].temp);
    }
    const_count = 0;
    if (opt_errors) cJSON_Delete(opt_errors);
    opt_errors = NULL;
}