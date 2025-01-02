// types.h
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Tag primitives
#define TAG_MASK 7
#define TAG_BITS 3
#define INT_TAG 0
#define HEAP_TAG 1
#define NULL_TAG 2

// Tagging and untagging
#define TAG_INT(x) (((x) << TAG_BITS) | INT_TAG)
#define UNTAG_INT(x) ((x) >> TAG_BITS)

#define TAG_PTR(x) ((x) | HEAP_TAG)
#define UNTAG_PTR(x) ((x) & ~TAG_MASK)

// Check tags
#define CHECK_TAG(x) ((x) & TAG_MASK)
#define IS_INT(x) (CHECK_TAG(x) == INT_TAG)
#define IS_PTR(x) (CHECK_TAG(x) == HEAP_TAG)
#define IS_NULL(x) (CHECK_TAG(x) == NULL_TAG)

// Object types
#define GLOBAL_TYPE 0
#define NULL_TYPE 1 // Omitted
#define INT_TYPE 2  // Omitted
#define ARRAY_TYPE 3
#define OBJECT_TYPE 4
#define BROKEN_HEART -1
typedef intptr_t ObjType;

// Forward declarations
typedef struct RTObj RTObj;
typedef intptr_t RInt;
typedef intptr_t RNull;
typedef struct RArray RArray;
typedef struct RClass RClass;
typedef struct TClass TClass;

#endif
