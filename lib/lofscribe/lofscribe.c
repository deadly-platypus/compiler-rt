#include <stdio.h>

void lof_precall(char numargs) {
    printf("lof_precall called with numargs = %d\n", numargs);
}

void lof_record_arg(void* parameter, int isPointer) {
    printf("lof_record_arg called with parameter = %08x and is%s a pointer\n", parameter, isPointer ? "" : " NOT");
}

void lof_postcall(void* returnValue, int isPointer) {
    printf("lof_postcall called with returnValue = %08x and is%s a pointer\n", returnValue, isPointer ? "" : " NOT");
}
