#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "bytecode.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    GLOBAL,
    LOCAL,
} Scope;

typedef struct {
    Vector *instructions;
    Vector *locals;
    Vector *args;
    int nlocals;
    int nargs;
    Scope flag;
} FuncContext;
FuncContext *newFuncContext();

typedef struct ObjContext ObjContext;
struct ObjContext {
    // Index of global slots in pool: int
    Vector *slots;
    // Name of global slots: char*
    Vector *names;
    ObjContext *prev;
};
ObjContext *newObjContext();

typedef struct {
    Vector *pool;
    FuncContext *funcContext;
    ObjContext *objContext;
} CompileInfo;

/* New six types of values */
Value *newNullValue();
IntValue *newIntValue(int);
StringValue *newStringValue(char *);
MethodValue *newMethodValue(int, int, int, Vector *);
SlotValue *newSlotValue(int);
ClassValue *newClassValue(Vector *);

/* Program related operation */
static Program *newProgram();
static void freeProgram(Program *);

/* Compile from Ast to Program */
static void compileScope(CompileInfo *, ScopeStmt *);

Program *compile(ScopeStmt *stmt);

#endif
