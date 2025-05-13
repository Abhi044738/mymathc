#ifndef CODEGEN_H
#define CODEGEN_H

#include "cJSON.h"

void init_codegen(void);
cJSON* get_code_json(void);
void generate_assembly(void);

#endif // CODEGEN_H