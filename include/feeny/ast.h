#ifndef AST_H
#define AST_H

#include "feeny/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_ERROR 1
#define STATUS_OK 0

typedef enum {
    INT_EXP,
    NULL_EXP,
    PRINTF_EXP,
    ARRAY_EXP,
    OBJECT_EXP,
    SLOT_EXP,
    SET_SLOT_EXP,
    CALL_SLOT_EXP,
    CALL_EXP,
    SET_EXP,
    IF_EXP,
    WHILE_EXP,
    REF_EXP,
    VAR_STMT,
    FN_STMT,
    SEQ_STMT,
    EXP_STMT
} AstTag;

typedef struct {
    AstTag tag;
} Exp;

typedef struct {
    AstTag tag;
} SlotStmt;

typedef struct {
    AstTag tag;
} ScopeStmt;

typedef struct {
    AstTag tag;
    int value;
} IntExp;

typedef struct {
    AstTag tag;
    char *format;
    int nexps;
    Exp **exps;
} PrintfExp;

typedef struct {
    AstTag tag;
    Exp *length;
    Exp *init;
} ArrayExp;

typedef struct {
    AstTag tag;
    Exp *parent;
    int nslots;
    SlotStmt **slots;
} ObjectExp;

typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
} SlotExp;

typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
    Exp *value;
} SetSlotExp;

typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
    int nargs;
    Exp **args;
} CallSlotExp;

typedef struct {
    AstTag tag;
    char *name;
    int nargs;
    Exp **args;
} CallExp;

typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
} SetExp;

typedef struct {
    AstTag tag;
    Exp *pred;
    ScopeStmt *conseq;
    ScopeStmt *alt;
} IfExp;

typedef struct {
    AstTag tag;
    Exp *pred;
    ScopeStmt *body;
} WhileExp;

typedef struct {
    AstTag tag;
    char *name;
} RefExp;

// Slot Statements
typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
} SlotVar;

typedef struct {
    AstTag tag;
    char *name;
    int nargs;
    char **args;
    ScopeStmt *body;
} SlotMethod;

// Scope Statements
typedef struct {
    AstTag tag;
    char *name;
    Exp *exp;
} ScopeVar;

typedef struct {
    AstTag tag;
    char *name;
    int nargs;
    char **args;
    ScopeStmt *body;
} ScopeFn;

typedef struct {
    AstTag tag;
    ScopeStmt *a;
    ScopeStmt *b;
} ScopeSeq;

typedef struct {
    AstTag tag;
    Exp *exp;
} ScopeExp;

Exp *make_IntExp(int value);
Exp *make_NullExp(void);
Exp *make_PrintfExp(char *format, int nexps, Exp **exps);
Exp *make_ArrayExp(Exp *length, Exp *init);
Exp *make_ObjectExp(Exp *parent, int nslots, SlotStmt **slots);
Exp *make_SlotExp(char *name, Exp *exp);
Exp *make_SetSlotExp(char *name, Exp *exp, Exp *value);
Exp *make_CallSlotExp(char *name, Exp *exp, int nargs, Exp **args);
Exp *make_CallExp(char *name, int nargs, Exp **args);
Exp *make_SetExp(char *name, Exp *exp);
Exp *make_IfExp(Exp *pred, ScopeStmt *conseq, ScopeStmt *alt);
Exp *make_WhileExp(Exp *pred, ScopeStmt *body);
Exp *make_RefExp(char *name);

SlotStmt *make_SlotVar(char *name, Exp *exp);
SlotStmt *make_SlotMethod(char *name, int nargs, char **args, ScopeStmt *body);

ScopeStmt *make_ScopeVar(char *name, Exp *exp);
ScopeStmt *make_ScopeFn(char *name, int nargs, char **args, ScopeStmt *body);
ScopeStmt *make_ScopeSeq(ScopeStmt *a, ScopeStmt *b);
ScopeStmt *make_ScopeExp(Exp *exp);

void print_exp(Exp *e);
void print_slotstmt(SlotStmt *s);
void print_scopestmt(ScopeStmt *s);

Exp *read_exp();
SlotStmt *read_slot();
ScopeStmt *read_scopestmt();
ScopeStmt *read_ast(char *filename);

#endif
