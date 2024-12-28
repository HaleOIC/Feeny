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
typedef enum {
    SLOT_VAR,  // Object slot var
    GLOBAL_VAR // Global var
} VarType;

typedef struct {
    int index;    // Variable's index
    VarType type; // Variable's type
} VarLocation;

typedef struct ScopeContext ScopeContext;
struct ScopeContext {
    Vector *instructions;
    Vector *locals;
    Vector *args;
    int nlocals;
    int nargs;
    ScopeContext *prev;
    Scope flag;
};
ScopeContext *newScopeContext(ScopeContext *);
static int findLocalVar(ScopeContext *, char *);

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
    ScopeContext *scopeContext;
    ObjContext *objContext;
} CompileInfo;
static int addConstantValue(Vector *, Value *);

/* New six types of values */
Value *newNullValue();
IntValue *newIntValue(int);
StringValue *newStringValue(char *);
MethodValue *newMethodValue(int, int, int, Vector *);
SlotValue *newSlotValue(int);
ClassValue *newClassValue(Vector *);

/* Compile from Ast to Program */
static void compileScope(CompileInfo *, ScopeStmt *);
static void compileExpr(CompileInfo *, Exp *);

Program *compile(ScopeStmt *stmt);

#endif
