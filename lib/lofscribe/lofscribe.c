#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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

static void output_hex(void* ptr, u_int64_t size) {
    if(!ptr) {
        return;
    }
    char* c = (char*)ptr;
    for(u_int64_t i = 0; i < size; i++) {
        char curr = c[i];
        if(curr == '"') {
            fprintf("\\\\%c", curr);
        } else if(curr >= '!' && curr <= '~') {
            fprintf(out, "%c", curr);
        } else {
            fprintf(out, "\\\\%02x", curr);
        }
    }
}

static void output_ptrval(FuncArg* arg) {
    PtrVal* ptrval = (PtrVal*)arg->value.pval;
    fprintf(out,
            "{ \"type\": %d, \"size\": %lu, \"precall\": \"",
            arg->typeId, arg->size);
    output_hex(ptrval->value, arg->size);
    if(ptrval->loc) {
        fprintf(out,
                "\", \"postcall\": \"");
        output_hex(ptrval->loc, arg->size);
    }
    fprintf(out,
            "\" }");
}

static void output_arg(FuncArg* arg, int isLast) {
    if(!arg) {
        return;
    }

    switch(arg->typeId) {
        case FloatTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %f, \"size\": %lu }", arg->typeId, arg->value.fval, arg->size);
            break;
        case DoubleTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %g, \"size\": %lu }", arg->typeId, arg->value.dval, arg->size);
            break;
        case IntegerTyID:
            fprintf(out, "{ \"type\": %d, \"value\": %d, \"size\": %lu }", arg->typeId, arg->value.ival, arg->size);
            break;
        case PointerTyID:
            output_ptrval(arg);
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

static PtrVal* create_pointer_val(Data value, u_int64_t size) {
    PtrVal* result = (PtrVal*)malloc(sizeof(PtrVal));
    if(!result) {
        fprintf(stderr, "Could not allocate PtrVal! Exiting...\n");
        exit(1);
    }
    result->value = (char*)malloc(size);
    if(!result->value) {
        fprintf(stderr, "Could not allocate PtrVal.value! Exiting\n");
        exit(1);
    }
    result->loc = value.pval;
    memcpy(result->value, value.pval, size);

    return result;
}

static FuncArg* create_arg(Data value, TypeID typeId, u_int64_t size) {
    FuncArg* arg = (FuncArg*)malloc(sizeof(FuncArg));
    if(!arg) {
        fprintf(stderr, "Could not allocate FuncArg! Exiting...\n");
        exit(1);
    }
    
    arg->size = size;
    arg->typeId = typeId;
    
    /* Read pointer data */
    if(isPointerLike(typeId)) {
        if(size == sizeof(char)) {
            /* This is a char* -- +1 to size for the null terminator */
            arg->size = strlen((char*)value.pval) + 1;
            arg->value.pval = create_pointer_val(value, arg->size);
        } else {
            arg->value.pval = create_pointer_val(value, size);
        }
    } else {
        arg->value = value;
    }

    return arg;
}

void lof_precall(void* funcaddr) {
    if(functionCalls == NULL) {
        functionCalls = create_stack();
        char name[128];
        int counter = 0;
        sprintf(name, "lof-output-%lu-%d.json", time(NULL), counter);
        while(access(name, F_OK) != -1) {
            sprintf(name, "lof-output-%lu-%d.json", time(NULL), ++counter);
        }

        out = fopen(name, "w+");
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

void lof_double_record_arg(TypeID typeId, size_t size, double parameter) {
    Data d;
    d.dval = parameter;
    lof_record_arg(typeId, size, d);
}

void lof_record_arg(TypeID typeId, size_t size, Data parameter) {
    /*printf("lof_record_arg called with parameter = %p and is%s a pointer\n",
            parameter.pval, isPointerLike(typeId) ? "" : " NOT");*/

    Stack *s = (Stack*)curr->data;

    FuncArg *arg = create_arg(parameter, typeId, size / 8);
    LLNode* node = create_node(arg);

    stack_push(s, node);
}

void lof_double_postcall(TypeID typeId, size_t size, double parameter) {
    Data d;
    d.dval = parameter;
    lof_postcall(typeId, size, d);
}

void lof_postcall(TypeID typeId, size_t size, Data returnValue) {
    /*printf("lof_postcall called with returnValue = %p and is%s a pointer\n",
            returnValue.pval, isPointerLike(typeId) ? "" : " NOT");*/

    FuncArg* retVal = create_arg(returnValue, typeId, size);

    LLNode* node = stack_pop(functionCalls);
    Stack* s = (Stack*)node->data;
    if(hasPrinted) {
        fprintf(out, ",\n");
    } else {
        fprintf(out, "{\"name\": \"%s\", \"functions\": [", getenv("_"));
    }

    fprintf(out,
            "{ \"function\": {"
            "\"addr\": \"%p\", "
            "\"return\": ",
            s->bottom->data);
    output_arg(retVal, 0);
    fprintf(out, "\"args\": [");

    free_function(node);
    fprintf(out, "] } }");
    if(isPointerLike(typeId)) {
        free(retVal->value.pval);
    }
    free(retVal);

    hasPrinted = 1;
}
