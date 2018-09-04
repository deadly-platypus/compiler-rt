#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lofscribe.h"

void stack_push(Stack* stack, LLNode* value) {
    stack->top->next = value;
    value->prev = stack->top;
    stack->top = value;
    stack->size++;
}

LLNode* stack_pop(Stack* stack) {
    if(stack->top == stack->bottom) {
        return NULL;
    }

    LLNode* result = stack->top;
    stack->top = result->prev;
    stack->top->next = NULL;
    result->prev = NULL;
    stack->size--;
    return result;
}

LLNode* create_node(void* data) {
    LLNode* result = (LLNode*)malloc(sizeof(LLNode));
    if(!result) {
        fprintf(stderr, "Could not allocate node! Exiting...");
        exit(1);
    }
    memset(result, 0, sizeof(LLNode));
    result->data = data;
    return result;
}

Stack* create_stack() {
    Stack* result = (Stack*)malloc(sizeof(Stack));
    if(!result) {
        fprintf(stderr, "Could not allocate stack! Exiting...");
        exit(1);
    }
    memset(result, 0, sizeof(Stack));
    result->bottom = create_node(NULL);
    result->top = result->bottom;
    return result;
}

void free_node(LLNode* node) {
    if(node) {
        free(node);
    }
}
