#ifndef RUNTIMEOBJ_H
#define RUNTIMEOBJ_H

#include "utils.h"
#include <stdint.h>

#define GLOBAL_TYPE 0
#define NULL_TYPE 1
#define INT_TYPE 2
#define ARRAY_TYPE 3
#define OBJECT_TYPE 4
#define BROKEN_HEART -1

typedef intptr_t ObjType;

// RTObj -> Runtime Object
typedef struct RTObj {
    ObjType type;
} RTObj;

// RInt -> Runtime Int Object
typedef struct RInt {
    ObjType type;
    intptr_t value;
} RInt;

// RNull -> Runtime Null Object
typedef struct RNull {
    ObjType type;
    intptr_t space;
} RNull;

// RArray -> Runtime Array Object
typedef struct RArray {
    ObjType type;
    size_t length;
    intptr_t slots[];
} RArray;

// RClass -> Runtime Class Instance Object
typedef struct RClass {
    ObjType type;
    intptr_t parent;
    intptr_t var_slots[];
} RClass;

// TClass -> Template Class
typedef struct TClass {
    ObjType type;
    int poolIndex;
    Vector *varNames;
    Map *funcNameToPoolIndex;
} TClass;

RInt *newIntObj(int);
RNull *newNullObj();
RArray *newArrayObj(int, RTObj *);
RClass *newClassObj(ObjType, int);
TClass *newTemplateClass(ObjType, int);
#endif // RUNTIMEOBJ_H