#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lofscribe.h"

/* Current function being recorded. curr->data is a Stack* */
static LLNode* curr;
FILE* out;

static int hasPrinted = 0;

static void cleanup() {
    if(out) {
        fprintf(out, "]");
        fclose(out);
    }
}

static void output_arg(FuncArg* arg, int isLast) {
    if(!arg) {
        return;
    }

    switch(arg->typeId) {
        case FloatTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %f }", arg->typeId, arg->value.fval);
            break;
        case DoubleTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %g }", arg->typeId, arg->value.dval);
            break;
        case IntegerTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %d }", arg->typeId, arg->value.ival);
            break;
        case PointerTyID:
            fprintf(out, "{ \"type\": %d, \"value\": \"%p\" }", arg->typeId, arg->value.pval);
            break;
        default:
            // TODO: Implement the rest
            fprintf(out, "{ \"type\": %d, \"value\": \"%p\" }", arg->typeId, arg->value.pval);
            break;
    }

    if(!isLast) {
        fprintf(out, ", ");
    }
}

static void free_function(LLNode* node) {
    if(!node) {
        return;
    }
    Stack* s = (Stack*)node->data;

    LLNode* curr = stack_pop(s);
    while(curr) {
        output_arg((FuncArg*)curr->data, s->size == 0);
        free(curr->data);
        free(curr);
        curr = stack_pop(s);
    }

    free(s->bottom);
    free(s);
    free(node);
}

static inline int isPointerLike(TypeID typeId) {
    return (typeId == ArrayTyID || typeId == PointerTyID);
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
        out = fopen("lof-output.json", "w+");
        if(!out) {
            fprintf(stderr, "Could not open lof-output.json! Exiting...\n");
            exit(1);
        }
        atexit(cleanup);
    }

    Stack* s = create_stack();
    s->bottom->data = funcaddr;
    curr = create_node(s);
    stack_push(functionCalls, curr);

    //printf("lof_precall called function at %p\n", funcaddr);
}

void lof_record_arg(Data parameter, TypeID typeId) {
    /*printf("lof_record_arg called with parameter = %p and is%s a pointer\n",
            parameter.pval, isPointerLike(typeId) ? "" : " NOT");*/

    Stack *s = (Stack*)curr->data;

    FuncArg *arg = create_arg(parameter, typeId);
    LLNode* node = create_node(arg);

    stack_push(s, node);
}

void lof_postcall(Data returnValue, TypeID typeId) {
    /*printf("lof_postcall called with returnValue = %p and is%s a pointer\n",
            returnValue.pval, isPointerLike(typeId) ? "" : " NOT");*/

    FuncArg retVal;
    retVal.typeId = typeId;
    retVal.value = returnValue;

    LLNode* node = stack_pop(functionCalls);
    Stack* s = (Stack*)node->data;
    if(hasPrinted) {
        fprintf(out, ",\n");
    } else {
        fprintf(out, "[");
    }

    fprintf(out,
            "{ \"function\": {"
            "\"addr\": \"%p\", "
            "\"return\": ",
            s->bottom->data);
    output_arg(&retVal, 0);
    fprintf(out, "\"args\": [");

    free_function(node);
    fprintf(out, "] } }");
    hasPrinted = 1;
}
