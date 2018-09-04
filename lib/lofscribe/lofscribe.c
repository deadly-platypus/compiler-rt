#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lofscribe.h"


/* Current function being recorded. curr->data is a Stack* */
static LLNode* curr;

static void free_function(LLNode* node) {
    Stack* s = (Stack*)node->data;

    LLNode* curr = stack_pop(s);
    while(curr) {
        free(curr->data);
        free(curr);
        curr = stack_pop(s);
    }

    free(s->bottom);
    free(s);
    free(node);
}

static FuncArg* create_arg(Data value, TypeID typeId) {
    FuncArg* arg = (FuncArg*)malloc(sizeof(FuncArg));
    if(!arg) {
        fprintf(stderr, "Could not allocate FuncArg! Exiting...\n");
        exit(1);
    }
    arg->value = value;
    arg->typeId = typeId;

    return arg;
}

void lof_precall(void* funcaddr) {
    if(functionCalls == NULL) {
        functionCalls = create_stack();
    }

    curr = NULL;
    printf("lof_precall called function at %p\n", funcaddr);
}

void lof_record_arg(Data parameter, TypeID typeId) {
    printf("lof_record_arg called with parameter = %p and is%s a pointer\n", 
            parameter.pval, typeId == PointerTyID ? "" : " NOT");
    if(!curr) {
        curr = create_node(create_stack());
    }

    Stack *s = (Stack*)curr->data;

    FuncArg *arg = create_arg(parameter, typeId);
    LLNode* node = create_node(arg);

    stack_push(s, node);
}

void lof_postcall(Data returnValue, TypeID typeId) {
    printf("lof_postcall called with returnValue = %p and is%s a pointer\n", 
            returnValue.pval, typeId == PointerTyID ? "" : " NOT");
    
    LLNode* node = stack_pop(functionCalls);

    free_function(node);
}
