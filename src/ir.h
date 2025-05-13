#ifndef IR_H
#define IR_H

#include "ast.h"
#include "cJSON.h"

void init_ir(void);
char* gen_ir(ASTNode *n);
cJSON* get_ir_json(void);
int ir_has_error(void);

#endif // IR_H
