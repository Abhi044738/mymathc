#include "codegen.h"
#include "ir.h"
#include "opt.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static cJSON *code_arr = NULL;
static cJSON *data_section = NULL;
static int const_count = 0;

typedef struct {
    char *temp;
    const char *reg;
} RegAlloc;

static RegAlloc reg_map[16];
static int reg_count = 0;

// Tracks which math functions are actually used
static int used_sin = 0;
static int used_cos = 0;
static int used_tan = 0;
static int used_exp = 0;
static int used_log = 0;
static int used_sqrt = 0;
static int used_pow = 0;

// Tracks last result register
static const char *result_reg = NULL;

static const char* allocate_xmm_register(const char *temp) {
    for (int i = 0; i < reg_count; i++) {
        if (strcmp(reg_map[i].temp, temp) == 0) {
            return reg_map[i].reg;
        }
    }

    if (reg_count >= 16) {
        fprintf(stderr, "Register spill detected for temp %s\n", temp);
        exit(EXIT_FAILURE);
    }
    
    const char *regs[] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", 
                         "xmm5", "xmm6", "xmm7", "xmm8", "xmm9",
                         "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"};
    
    reg_map[reg_count].temp = strdup(temp);
    reg_map[reg_count].reg = regs[reg_count];
    return regs[reg_count++];
}

static void add_data_label(double value) {
    char label[32];
    sprintf(label, "const_%d", const_count++);
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "label", label);
    cJSON_AddNumberToObject(obj, "value", value);
    cJSON_AddItemToArray(data_section, obj);
}

void init_codegen() {
    // Free reg_map temp strings
    for (int i = 0; i < reg_count; i++) {
        free(reg_map[i].temp);
        reg_map[i].temp = NULL;
    }
    if (code_arr) cJSON_Delete(code_arr);
    if (data_section) cJSON_Delete(data_section);
    
    code_arr = cJSON_CreateArray();
    data_section = cJSON_CreateArray();
    const_count = 0;
    reg_count = 0;
    result_reg = NULL;
    
    // Reset math function usage
    used_sin = used_cos = used_tan = used_exp = used_log = used_sqrt = used_pow = 0;

    cJSON *text_section = cJSON_CreateArray();
    cJSON_AddItemToArray(text_section, cJSON_CreateString("section .text"));
    cJSON_AddItemToArray(text_section, cJSON_CreateString("global main"));
    cJSON_AddItemToArray(code_arr, text_section);

    cJSON *rodata = cJSON_CreateArray();
    cJSON_AddItemToArray(rodata, cJSON_CreateString("section .rodata"));
    cJSON_AddItemToArray(code_arr, rodata);
}

cJSON* get_code_json() {
    return code_arr;
}

void generate_assembly() {
    cJSON *ir = get_opt_json();
    cJSON *text_section = cJSON_GetArrayItem(code_arr, 0); // Text section
    cJSON *rodata = cJSON_GetArrayItem(code_arr, 1);       // Rodata section

    // Add main label
    cJSON_AddItemToArray(text_section, cJSON_CreateString("main:"));
    
    char asm_line[256];
    cJSON *instr;
    cJSON_ArrayForEach(instr, ir) {
        const char *code = cJSON_GetStringValue(instr);
        char temp[16], a[16], op[10], b[16], func[10];
        double value;

        if (sscanf(code, "%15s = %15s %9s %15s", temp, a, op, b) == 4) {
            const char *reg_a = allocate_xmm_register(a);
            const char *reg_b = allocate_xmm_register(b);
            const char *reg_out = allocate_xmm_register(temp);
            result_reg = reg_out;

            // Direct computation without redundant moves
            if (strcmp(op, "+") == 0) {
                sprintf(asm_line, "addsd %s, %s", reg_a, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                sprintf(asm_line, "movsd %s, %s", reg_out, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            } 
            else if (strcmp(op, "-") == 0) {
                // Correct order: a - b
                sprintf(asm_line, "subsd %s, %s", reg_a, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                sprintf(asm_line, "movsd %s, %s", reg_out, reg_a);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            } 
            else if (strcmp(op, "*") == 0) {
                sprintf(asm_line, "mulsd %s, %s", reg_a, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                sprintf(asm_line, "movsd %s, %s", reg_out, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            } 
            else if (strcmp(op, "/") == 0) {
                // Correct order: a / b
                sprintf(asm_line, "divsd %s, %s", reg_a, reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                sprintf(asm_line, "movsd %s, %s", reg_out, reg_a);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            } 
            else if (strcmp(op, "^") == 0) {
                used_pow = 1;
                sprintf(asm_line, "movsd xmm0, %s", reg_a);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                sprintf(asm_line, "movsd xmm1, %s", reg_b);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
                cJSON_AddItemToArray(text_section, cJSON_CreateString("call pow"));
                sprintf(asm_line, "movsd %s, xmm0", reg_out);
                cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            }
        }
        else if (sscanf(code, "%15s = %lf", temp, &value) == 2) {
            const char *reg = allocate_xmm_register(temp);
            result_reg = reg;
            add_data_label(value);
            sprintf(asm_line, "movsd %s, [const_%d]", reg, const_count-1);
            cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
        }
        else if (sscanf(code, "%15s = %9s %15s", temp, func, a) == 3) {
            const char *reg_a = allocate_xmm_register(a);
            const char *reg_out = allocate_xmm_register(temp);
            result_reg = reg_out;

            // Track used math functions
            if (strcmp(func, "sin") == 0) used_sin = 1;
            else if (strcmp(func, "cos") == 0) used_cos = 1;
            else if (strcmp(func, "tan") == 0) used_tan = 1;
            else if (strcmp(func, "exp") == 0) used_exp = 1;
            else if (strcmp(func, "log") == 0) used_log = 1;
            else if (strcmp(func, "sqrt") == 0) used_sqrt = 1;

            // Optimized math function call
            sprintf(asm_line, "movsd xmm0, %s", reg_a);
            cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            sprintf(asm_line, "call %s", func);
            cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
            sprintf(asm_line, "movsd %s, xmm0", reg_out);
            cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
        }
    }

    // Move final result to xmm0 for return
    if (result_reg) {
        if (strcmp(result_reg, "xmm0") != 0) {
            sprintf(asm_line, "movsd xmm0, %s", result_reg);
            cJSON_AddItemToArray(text_section, cJSON_CreateString(asm_line));
        }
    }
    
    // Add return instruction
    cJSON_AddItemToArray(text_section, cJSON_CreateString("ret"));

    // Add extern declarations only for used functions
    if (used_sin || used_cos || used_tan || used_exp || used_log || used_sqrt || used_pow) {
        // Insert externs at the beginning of the text section
        int insert_index = 0;
        cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("; Extern declarations"));
        if (used_sin) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern sin"));
        if (used_cos) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern cos"));
        if (used_tan) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern tan"));
        if (used_exp) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern exp"));
        if (used_log) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern log"));
        if (used_sqrt) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern sqrt"));
        if (used_pow) cJSON_InsertItemInArray(text_section, insert_index++, cJSON_CreateString("extern pow"));
    }

    // Add constants to rodata
    if (data_section->child) {
        for (int i = 0; i < const_count; i++) {
            cJSON *entry = cJSON_GetArrayItem(data_section, i);
            char line[64];
            sprintf(line, "const_%d: dq %.17g", i, 
                   cJSON_GetObjectItem(entry, "value")->valuedouble);
            cJSON_AddItemToArray(rodata, cJSON_CreateString(line));
        }
    }

    // Write to file
    // FILE *fp = fopen("output.asm", "w");
    // if (!fp) {
    //     perror("Failed to open output file");
    //     exit(EXIT_FAILURE);
    // }

    // cJSON *section;
    // cJSON_ArrayForEach(section, code_arr) {
    //     cJSON *line;
    //     cJSON_ArrayForEach(line, section) {
    //         fprintf(fp, "%s\n", cJSON_GetStringValue(line));
    //     }
    // }

    // fclose(fp);
}