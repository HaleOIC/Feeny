#ifndef RUNTIMEOBJ_H
#define RUNTIMEOBJ_H

#include "collector.h"
#include "types.h"
#include "utils.h"
#include <stdint.h>

// RTObj -> Runtime Object
struct RTObj {
    ObjType type;
};

// RInt -> Runtime Int Object
struct RInt {
    ObjType type;
    intptr_t value;
};

// RNull -> Runtime Null Object
struct RNull {
    ObjType type;
    intptr_t space;
};

// RArray -> Runtime Array Object
struct RArray {
    ObjType type;
    size_t length;
    intptr_t slots[];
};

// RClass -> Runtime Class Instance Object
struct RClass {
    ObjType type;
    intptr_t parent;
    intptr_t var_slots[];
};

// TClass -> Template Class
struct TClass {
    ObjType type;
    int poolIndex;
    Vector *varNames;
    Map *funcNameToPoolIndex;
};

RInt *newIntObj(int);
RNull *newNullObj();
RArray *newArrayObj(int, RTObj *);
RClass *newClassObj(ObjType, int);
TClass *newTemplateClass(ObjType, int);
#endif // RUNTIMEOBJ_H