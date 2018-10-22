#include <stddef.h>

/* This is copied from include/llvm/IR/Type.h. If this changes, then
 * those changes will have to be reflected here. It is just easier
 * to copy this enum than to try to figure out how to get the the
 * C++ file to link properly in a C program */
typedef enum TypeID {
    // PrimitiveTypes - make sure LastPrimitiveTyID stays up to date.
    VoidTyID = 0,    ///<  0: type with no size
    HalfTyID,        ///<  1: 16-bit floating point type
    FloatTyID,       ///<  2: 32-bit floating point type
    DoubleTyID,      ///<  3: 64-bit floating point type
    X86_FP80TyID,    ///<  4: 80-bit floating point type (X87)
    FP128TyID,       ///<  5: 128-bit floating point type (112-bit mantissa)
    PPC_FP128TyID,   ///<  6: 128-bit floating point type (two 64-bits, PowerPC)
    LabelTyID,       ///<  7: Labels
    MetadataTyID,    ///<  8: Metadata
    X86_MMXTyID,     ///<  9: MMX vectors (64 bits, X86 specific)
    TokenTyID,       ///< 10: Tokens

    // Derived types... see DerivedTypes.h file.
    // Make sure FirstDerivedTyID stays up to date!
    IntegerTyID,     ///< 11: Arbitrary bit width integers
    FunctionTyID,    ///< 12: Functions
    StructTyID,      ///< 13: Structures
    ArrayTyID,       ///< 14: Arrays
    PointerTyID,     ///< 15: Pointers
    VectorTyID,      ///< 16: SIMD 'packed' format, or other vector type
} TypeID;

typedef struct ptrval {
    void* loc;
    char* value;
} PtrVal;

typedef union Data {
        void* pval;
        int ival;
        long lval;
        char cval;
        float fval;
        double dval;
} Data;

/* Called to allocate a new function entry onto the stack */
void lof_precall(char* funcname);

/* Records the input data to a function */
void lof_record_arg(
        TypeID typeId,      /* The type of the argument */
        size_t size,        /* Allocated size for pointers, or number of bits for primitives */
        Data parameter      /* The value of an argument passed to a function */
        );

/* A separate call to handle floating types correctly */
void lof_double_record_arg(TypeID typeId,
        size_t size,
        double parameter
        );

/* Records the return value, and reports program state changes */
void lof_postcall(
        TypeID typeId,       /* The type of the return value */
        size_t size,
        Data returnValue    /* The value of an argument returned from a function */
        );

void lof_double_postcall(TypeID typeId,
        size_t size,
        double returnValue
);

/* We are going to keep track of function calls using a stack implemented
 * as a linked list. Upon exit, the args are popped off and printed out */

typedef struct func_arg_t {
    Data value;
    TypeID typeId;
    size_t size;
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
