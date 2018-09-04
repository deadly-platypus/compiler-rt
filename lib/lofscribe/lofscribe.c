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

static FuncArg* create_arg(Data value, DataType dataType, int isPointer) {
    FuncArg* arg = (FuncArg*)malloc(sizeof(FuncArg));
    if(!arg) {
        fprintf(stderr, "Could not allocate FuncArg! Exiting...\n");
        exit(1);
    }
    arg->value = value;
    arg->data_type = dataType;
    arg->isPointer = isPointer;

    return arg;
}

void lof_precall(int numargs) {
    if(functionCalls == NULL) {
        functionCalls = create_stack();
    }

    curr = NULL;
    printf("lof_precall called with numargs = %d\n", numargs);
}

void lof_record_arg(Data parameter, int isPointer, DataType dataType) {
    printf("lof_record_arg called with parameter = %p and is%s a pointer\n", 
            parameter.pval, isPointer ? "" : " NOT");
    if(!curr) {
        curr = create_node(create_stack());
    }

    Stack *s = (Stack*)curr->data;

    FuncArg *arg = create_arg(parameter, dataType, isPointer);
    LLNode* node = create_node(arg);

    stack_push(s, node);
}

void lof_postcall(Data returnValue, int isPointer, DataType dataType) {
    printf("lof_postcall called with returnValue = %p and is%s a pointer\n", 
            returnValue.pval, isPointer ? "" : " NOT");
    
    LLNode* node = stack_pop(functionCalls);

    free_function(node);
}
