#include <stddef.h>

typedef enum data_type { 
    ptr,        /* generic pointer type */
    str_type,   /* char * */
    i_type,     /* integer */
    l_type,     /* long */
    c_type,     /* char */
    f_type,     /* float */
    d_type      /* double */
} DataType;
typedef union Data {
        void* pval;
        char* strval;
        int ival;
        long lval;
        char cval;
        float fval;
        double dval;
} Data;

/* Called to allocate a new function entry onto the stack */
void lof_precall(int numargs);

/* Records the input data to a function */
void lof_record_arg(
        Data parameter,     /* The value of an argument passed to a function */
        int isPointer,      /* True if parameter is a pointer, false otherwise */
        DataType dataType   /* The underlying datatype */
        );

/* Records the return value, and reports program state changes */
void lof_postcall(
        Data returnValue,   /* The value of an argument passed to a function */
        int isPointer,      /* True if parameter is a pointer, false otherwise */
        DataType dataType   /* The underlying datatype */
        );

/* We are going to keep track of function calls using a stack implemented
 * as a linked list. Upon exit, the args are popped off and printed out */

typedef struct func_arg_t {
    Data value;
    DataType data_type;
    int isPointer;
} FuncArg;

typedef struct ll_node {
    void* data;
    struct ll_node *next, *prev;
} LLNode;

/* LIFO stack struct */
typedef struct stack {
    LLNode *bottom, /* prev == data == NULL */
           *top;    /* The most recently added LLNode */
    size_t size;    /* Number of nodes in this list not including bottom */
} Stack;

Stack* functionCalls;

/* Initializes stack */
Stack* create_stack(void);

/* Pushes a new node to the top of the stack */
void stack_push(Stack* stack, LLNode* value);

/* Pops off value from the top of the stack, or returns null if 
 * the stack contains only the bottom member */
LLNode* stack_pop(Stack* stack);

/* Initializes LLNode */
LLNode* create_node(void* data);

/* Frees the node, but not the underlying data */
void free_node(LLNode* node);
