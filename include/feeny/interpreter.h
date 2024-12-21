#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "feeny/ast.h"
#include "feeny/utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Using tag Union to implement inheritance
 * We have four different objects through the whole interpreter
 */
typedef enum {
    NULL_OBJ,
    INT_OBJ,
    ARRAY_OBJ,
    ENV_OBJ
} ObjTag;

typedef enum {
    VAR_ENTRY,
    CODE_ENTRY
} EntryTag;

/**
 * Base class Obj
 * According to Obj's tag to judge its type
 * -1 -> unknown
 * 1 -> NullObj
 * 2 -> IntObj
 * 3 -> ArrayObj
 * 4 -> EnvObj
 */
typedef struct Obj {
    ObjTag tag;
} Obj;
int obj_type(Obj *);

/**
 * Null Object
 * Do not support any operation except for a way to create them
 */
typedef struct NullObj {
    ObjTag tag;
} NullObj;
NullObj *make_null_obj();

/**
 * Int Object, should support basic arithmetic and
 * comparison operators
 */
typedef struct IntObj {
    ObjTag tag;
    int value;
} IntObj;
IntObj *make_int_obj(int);
IntObj *add(IntObj *, IntObj *);
IntObj *sub(IntObj *, IntObj *);
IntObj *mul(IntObj *, IntObj *);
IntObj *divi(IntObj *, IntObj *);
IntObj *mod(IntObj *, IntObj *);
Obj *lt(IntObj *, IntObj *);
Obj *gt(IntObj *, IntObj *);
Obj *le(IntObj *, IntObj *);
Obj *ge(IntObj *, IntObj *);
Obj *eq(IntObj *, IntObj *);

/**
 * Array Object
 * Array objects are created given a length and an initial value,
 * and needs to support retrieving its length,
 * storing a value at a speci c index, and retrieving
 * a value at a speci c index.
 */
typedef struct {
    ObjTag tag;
    Vector *elements;
} ArrayObj;
ArrayObj *make_array_obj(IntObj *, Obj *);
IntObj *array_length(ArrayObj *);
NullObj *array_set(ArrayObj *, IntObj *, Obj *);
Obj *array_get(ArrayObj *, IntObj *);

/**
 * There are two types of entry in the env
 * 1. Variable entries, which contains a single value,
 *      supports retrieving and changing this value.
 * 2. Code entries, which contains a list of arguments
 *      and a statement representing the code body,
 *      and supports operations for retrieving these.
quantities.
 */
typedef struct {
    EntryTag tag;
} Entry;
typedef struct {
    EntryTag tag;
    Obj *value;
} VarEntry;
typedef struct {
    EntryTag tag;
    Vector *params;
    ScopeStmt *body;
} CodeEntry;

/**
 * Environment Object
 * Environments must support two operations, adding a new binding from a
 * name to an environment entry, and retrieving the associated environment
 * entry for a given name.
 */
typedef struct {
    ObjTag tag;
    Obj *parent;
    Vector *names;
    Vector *entries;
} EnvObj;
EnvObj *make_env_obj(Obj *);
void add_entry(EnvObj *, char *, Entry *);
Entry *get_entry(EnvObj *, char *);

void print_obj(Obj*);
Obj *exec_stmt(EnvObj *, ScopeStmt *);
Obj *eval_exp(EnvObj *, Exp *);
void interpret(ScopeStmt *);

#endif
