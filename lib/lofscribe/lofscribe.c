#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "lofscribe.h"
#include <sys/mman.h>
#include <errno.h>

#define MAX_ADDR ((void*)0x7FFFFFFFFFFFFFFF)

#define DBG_PRINT(fmt, ...) { if(in_debug) { printf(fmt, ##__VA_ARGS__); } }

/* Current function being recorded. curr->data is a Stack* */
static LLNode *curr;
static FILE *out;

static char in_debug;
static char output_json;

static unsigned int stack_depth;
static int hasPrinted = 0;

static void cleanup() {
    if (out) {
        fprintf(out, "]");
        fclose(out);
    }
}

static int make_readable_region(void* ptr, size_t size) {
    char *c = (char *) ((long)ptr & ~(getpagesize() - 1));
    errno = 0;
    if(mprotect(c, (char*)ptr - c + size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        DBG_PRINT("Could not set mprotect on %p (ptr = %p): %s\n", c, ptr, strerror(errno));
        return 0;
    }

    return 1;
}

static void output_hex(void *ptr, size_t size) {
    if (!ptr || ptr > MAX_ADDR) {
        return;
    }
    if(!make_readable_region(ptr, size)) {
        return;
    }

    for (u_int64_t i = 0; i < size; i++) {
        char curr = ((char*)ptr)[i];
/*        if(curr == '"') {
            fprintf(out, "\\\\%c", curr);
        } else if(curr >= '!' && curr <= '~') {
            fprintf(out, "%c", curr);
        } else {*/
        fprintf(out, "\\x%02x", curr);
//        }
    }
}

static void output_ptrval(FuncArg *arg) {
    PtrVal *ptrval = (PtrVal *) arg->value.pval;
    fprintf(out,
            "{ \"type\": %d, \"size\": %lu, \"precall\": \"",
            arg->typeId, arg->size);
    if (ptrval == NULL) {
        fprintf(out, "0");
    } else {
        output_hex(ptrval->value, arg->size);
        if (ptrval->loc) {
            fprintf(out,
                    "\", \"postcall\": \"");
            output_hex(ptrval->loc, arg->size);
        }
    }
    fprintf(out,
            "\" }");
}

static void output_arg(FuncArg *arg, int isLast) {
    if (!arg) {
        return;
    }

    switch (arg->typeId) {
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

    if (!isLast) {
        fprintf(out, ", ");
    }
}

static void free_function(LLNode *node) {
    if (!node) {
        return;
    }
    Stack *s = (Stack *) node->data;

    LLNode *curr = stack_pop(s);
    while (curr) {
        output_arg((FuncArg *) curr->data, s->size == 0);
        free(curr->data);
        free(curr);
        curr = stack_pop(s);
    }

    free(s->bottom->data);
    free(s->bottom);
    free(s);
    free(node);
}

static inline int isPointerLike(TypeID typeId) {
    return (typeId == ArrayTyID || typeId == PointerTyID);
}

static PtrVal *create_pointer_val(Data value, size_t size) {
    PtrVal *result = (PtrVal *) malloc(sizeof(PtrVal));
    if (!result) {
        fprintf(stderr, "Could not allocate PtrVal! Exiting...\n");
        exit(1);
    }

    result->value = (char *) malloc(size);
    if (!result->value) {
        fprintf(stderr, "Could not allocate PtrVal.value! Exiting\n");
        exit(1);
    }
    result->loc = value.pval;
    if (value.pval > NULL && value.pval < MAX_ADDR) {
        if(make_readable_region(value.pval, size)) {
            memcpy(result->value, value.pval, size);
        }
    }
    return result;
}

static FuncArg *create_arg(Data value, TypeID typeId, size_t size) {
    FuncArg *arg = (FuncArg *) malloc(sizeof(FuncArg));
    if (!arg) {
        fprintf(stderr, "Could not allocate FuncArg! Exiting...\n");
        exit(1);
    }

    arg->size = size;
    arg->typeId = typeId;

    /* Read pointer data */
    if (isPointerLike(typeId)) {
        if (size == sizeof(char)) {
            /* This is a char* -- +1 to size for the null terminator */
            if (value.pval == NULL) {
                arg->size = 0;
                arg->value.pval = NULL;
            } else {
                arg->size = strlen((char *) value.pval) + 1;
                arg->value.pval = create_pointer_val(value, arg->size);
            }
        } else {
            arg->value.pval = create_pointer_val(value, size);
        }
    } else {
        arg->value = value;
    }

    return arg;
}

void lof_precall(char *funcname) {
    if (functionCalls == NULL) {
        if(getenv("LOF_DEBUG") && strcmp("true", getenv("LOF_DEBUG")) == 0) {
            in_debug = 1;
        }

        functionCalls = create_stack();
        char name[128];
        int counter = 0;
        if(getenv("LOF_OUTPUT_JSON") && strcmp(getenv("LOF_OUTPUT_JSON"), "false") == 0) {
            strcpy(name, "/dev/null");
        } else {
            sprintf(name, "lof-output-%lu-%d.json", time(NULL), counter);
            output_json = 1;
        }

        if(output_json) {
            while (access(name, F_OK) != -1) {
                sprintf(name, "lof-output-%lu-%d.json", time(NULL), ++counter);
            }
        }


        out = fopen(name, "w+");
        if (!out) {
            fprintf(stderr, "Could not open lof-output.json! Exiting...\n");
            exit(1);
        }
        atexit(cleanup);
    }

    DBG_PRINT("Calling function %s (stack depth: %d)\n", funcname, stack_depth++);

    Stack *s = create_stack();
    s->bottom->data = strdup(funcname);
    curr = create_node(s);
    stack_push(functionCalls, curr);
}

void lof_double_record_arg(TypeID typeId, size_t size, double parameter) {
    Data d;
    d.dval = parameter;
    lof_record_arg(typeId, size, d);
}

void lof_record_arg(TypeID typeId, size_t size, Data parameter) {
    Stack *s = (Stack *) curr->data;

    FuncArg *arg = create_arg(parameter, typeId, size / 8);
    LLNode *node = create_node(arg);

    stack_push(s, node);
}

void lof_double_postcall(TypeID typeId, size_t size, double parameter) {
    Data d;
    d.dval = parameter;
    lof_postcall(typeId, size, d);
}

void lof_postcall(TypeID typeId, size_t size, Data returnValue) {
    LLNode *node = stack_pop(functionCalls);
    if (!node) {
        return;
    }

    FuncArg *retVal = create_arg(returnValue, typeId, size);

    Stack *s = (Stack *) node->data;
    if(in_debug) {
        DBG_PRINT("Exiting %s\n", s->bottom->data);
        stack_depth--;
    }

    if (hasPrinted) {
        fprintf(out, ",\n");
    } else {
        fprintf(out, "{\"name\": \"%s\", \"functions\": [", getenv("_"));
    }

    fprintf(out,
            "{ \"function\": {"
            "\"name\": \"%s\", "
            "\"return\": ",
            s->bottom->data);
    output_arg(retVal, 0);
    fprintf(out, "\"args\": [");

    free_function(node);
    fprintf(out, "] } }");
    if (isPointerLike(typeId)) {
        free(retVal->value.pval);
    }
    free(retVal);

    hasPrinted = 1;
}
