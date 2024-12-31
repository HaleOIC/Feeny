// types.h
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define GLOBAL_TYPE 0
#define NULL_TYPE 1
#define INT_TYPE 2
#define ARRAY_TYPE 3
#define OBJECT_TYPE 4
#define BROKEN_HEART -1
typedef intptr_t ObjType;

// Forward declarations
typedef struct RTObj RTObj;
typedef struct RInt RInt;
typedef struct RNull RNull;
typedef struct RArray RArray;
typedef struct RClass RClass;
typedef struct TClass TClass;

#endif
