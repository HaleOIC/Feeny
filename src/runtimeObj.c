#include "feeny/runtimeObj.h"

RInt *newIntObj(int value) {
    RInt *rv = (RInt *)malloc(sizeof(RInt));
    rv->type = INT_TYPE;
    rv->value = value;
    return rv;
}

RNull *newNullObj() {
    RNull *rv = (RNull *)malloc(sizeof(RNull));
    rv->type = NULL_TYPE;
    rv->space = 0;
    return rv;
}

RArray *newArrayObj(int length, RTObj *initValue) {
    RArray *rv = (RArray *)malloc(sizeof(RArray) + length * sizeof(intptr_t));
    rv->type = ARRAY_TYPE;
    rv->length = length;
    for (int i = 0; i < length; i++) {
        rv->slots[i] = (intptr_t)initValue;
    }
    return rv;
}

RClass *newClassObj(ObjType type, int slotNum) {
    RClass *rv = (RClass *)malloc(sizeof(RClass) + slotNum * sizeof(intptr_t));
    rv->type = type;
    rv->parent = (intptr_t)NULL;
    for (int i = 0; i < slotNum; i++) {
        rv->var_slots[i] = 0;
    }
    return rv;
}

TClass *newTemplateClass(ObjType type, int index) {
    TClass *rv = (TClass *)malloc(sizeof(TClass));
    rv->type = type;
    rv->poolIndex = index;
    rv->varNames = make_vector();
    rv->funcNameToPoolIndex = newMap();
    return rv;
}