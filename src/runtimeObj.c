#include "feeny/runtimeObj.h"

#include <execinfo.h>
#include <stdio.h>

void print_caller() {
    void *callstack[2];
    int frames = backtrace(callstack, 2);
    char **strs = backtrace_symbols(callstack, frames);

    if (frames >= 2) {
        printf("Called by: %s\n", strs[1]);
    }
    free(strs);
}

// RInt *newIntObj(int value) {
//     RInt *rv = (RInt *)halloc(sizeof(RInt));
//     rv->type = INT_TYPE;
//     rv->value = value;
//     return rv;
// }
RInt newIntObj(int value) {
    return (intptr_t)TAG_INT(value);
}

// RNull *newNullObj() {
//     // print_caller();
//     RNull *rv = (RNull *)halloc(sizeof(RNull));
//     rv->type = NULL_TYPE;
//     rv->space = 0;
//     return rv;
// }
RNull newNullObj() {
    return (intptr_t)(NULL_TAG);
}

RArray *newArrayObj(int length, RTObj *initValue) {
    RArray *rv = (RArray *)halloc(sizeof(RArray) + length * sizeof(intptr_t));
    rv->type = ARRAY_TYPE;
    rv->length = length;
    for (int i = 0; i < length; i++) {
        if (IS_PTR((intptr_t)initValue)) {
            if (!is_forward((intptr_t)initValue)) {
                rv->slots[i] = (intptr_t)initValue;
            } else {
                rv->slots[i] = (intptr_t)get_forward_address(UNTAG_PTR((intptr_t)initValue));
            }
        } else {
            rv->slots[i] = (intptr_t)initValue;
        }
    }
    return (void *)TAG_PTR((intptr_t)rv);
    // return rv;
}

RClass *newClassObj(ObjType type, int slotNum) {
    RClass *rv = (RClass *)halloc(sizeof(RClass) + slotNum * sizeof(intptr_t));
    rv->type = type;
    rv->parent = NULL_TAG;
    for (int i = 0; i < slotNum; i++) {
        rv->var_slots[i] = 0;
    }
    return (void *)TAG_PTR((intptr_t)rv);
    // return rv;
}

TClass *newTemplateClass(ObjType type, int index) {
    TClass *rv = (TClass *)malloc(sizeof(TClass));
    rv->type = type;
    rv->poolIndex = index;
    rv->varNames = make_vector();
    rv->funcNameToPoolIndex = newMap();
    return rv;
}