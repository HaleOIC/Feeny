#include "feeny/ast.h"

//============================================================
//================= CONSTRUCTORS =============================
//============================================================

Exp *make_IntExp(int value) {
    IntExp *e = malloc(sizeof(IntExp));
    e->tag = INT_EXP;
    e->value = value;
    return (Exp *)e;
}

Exp *make_NullExp() {
    Exp *e = malloc(sizeof(Exp));
    e->tag = NULL_EXP;
    return e;
}

Exp *make_PrintfExp(char *format, int nexps, Exp **exps) {
    PrintfExp *e = malloc(sizeof(PrintfExp));
    e->tag = PRINTF_EXP;
    e->format = format;
    e->nexps = nexps;
    e->exps = exps;
    return (Exp *)e;
}

Exp *make_ArrayExp(Exp *length, Exp *init) {
    ArrayExp *e = malloc(sizeof(ArrayExp));
    e->tag = ARRAY_EXP;
    e->length = length;
    e->init = init;
    return (Exp *)e;
}

Exp *make_ObjectExp(Exp *parent, int nslots, SlotStmt **slots) {
    ObjectExp *e = malloc(sizeof(ObjectExp));
    e->tag = OBJECT_EXP;
    e->parent = parent;
    e->nslots = nslots;
    e->slots = slots;
    return (Exp *)e;
}

Exp *make_SlotExp(char *name, Exp *exp) {
    SlotExp *e = malloc(sizeof(SlotExp));
    e->tag = SLOT_EXP;
    e->name = name;
    e->exp = exp;
    return (Exp *)e;
}

Exp *make_SetSlotExp(char *name, Exp *exp, Exp *value) {
    SetSlotExp *e = malloc(sizeof(SetSlotExp));
    e->tag = SET_SLOT_EXP;
    e->name = name;
    e->exp = exp;
    e->value = value;
    return (Exp *)e;
}

Exp *make_CallSlotExp(char *name, Exp *exp, int nargs, Exp **args) {
    CallSlotExp *e = malloc(sizeof(CallSlotExp));
    e->tag = CALL_SLOT_EXP;
    e->name = name;
    e->exp = exp;
    e->nargs = nargs;
    e->args = args;
    return (Exp *)e;
}

Exp *make_CallExp(char *name, int nargs, Exp **args) {
    CallExp *e = malloc(sizeof(CallExp));
    e->tag = CALL_EXP;
    e->name = name;
    e->nargs = nargs;
    e->args = args;
    return (Exp *)e;
}

Exp *make_SetExp(char *name, Exp *exp) {
    SetExp *e = malloc(sizeof(SetExp));
    e->tag = SET_EXP;
    e->name = name;
    e->exp = exp;
    return (Exp *)e;
}

Exp *make_IfExp(Exp *pred, ScopeStmt *conseq, ScopeStmt *alt) {
    IfExp *e = malloc(sizeof(IfExp));
    e->tag = IF_EXP;
    e->pred = pred;
    e->conseq = conseq;
    e->alt = alt;
    return (Exp *)e;
}

Exp *make_WhileExp(Exp *pred, ScopeStmt *body) {
    WhileExp *e = malloc(sizeof(WhileExp));
    e->tag = WHILE_EXP;
    e->pred = pred;
    e->body = body;
    return (Exp *)e;
}

Exp *make_RefExp(char *name) {
    RefExp *e = malloc(sizeof(RefExp));
    e->tag = REF_EXP;
    e->name = name;
    return (Exp *)e;
}

SlotStmt *make_SlotVar(char *name, Exp *exp) {
    SlotVar *s = malloc(sizeof(SlotVar));
    s->tag = VAR_STMT;
    s->name = name;
    s->exp = exp;
    return (SlotStmt *)s;
}

SlotStmt *make_SlotMethod(char *name, int nargs, char **args, ScopeStmt *body) {
    SlotMethod *s = malloc(sizeof(SlotMethod));
    s->tag = FN_STMT;
    s->name = name;
    s->nargs = nargs;
    s->args = args;
    s->body = body;
    return (SlotStmt *)s;
}

ScopeStmt *make_ScopeVar(char *name, Exp *exp) {
    ScopeVar *s = malloc(sizeof(ScopeVar));
    s->tag = VAR_STMT;
    s->name = name;
    s->exp = exp;
    return (ScopeStmt *)s;
}

ScopeStmt *make_ScopeFn(char *name, int nargs, char **args, ScopeStmt *body) {
    ScopeFn *s = malloc(sizeof(ScopeFn));
    s->tag = FN_STMT;
    s->name = name;
    s->nargs = nargs;
    s->args = args;
    s->body = body;
    return (ScopeStmt *)s;
}

ScopeStmt *make_ScopeSeq(ScopeStmt *a, ScopeStmt *b) {
    ScopeSeq *s = malloc(sizeof(ScopeSeq));
    s->tag = SEQ_STMT;
    s->a = a;
    s->b = b;
    return (ScopeStmt *)s;
}

ScopeStmt *make_ScopeExp(Exp *exp) {
    ScopeExp *s = malloc(sizeof(ScopeExp));
    s->tag = EXP_STMT;
    s->exp = exp;
    return (ScopeStmt *)s;
}

//============================================================
//=================== PRINTING ===============================
//============================================================

void print_exp(Exp *e) {
    switch (e->tag) {
    case INT_EXP: {
        IntExp *e2 = (IntExp *)e;
        printf("%d", e2->value);
        break;
    }
    case NULL_EXP: {
        printf("null");
        break;
    }
    case PRINTF_EXP: {
        PrintfExp *e2 = (PrintfExp *)e;
        printf("printf(");
        print_string(e2->format);
        for (int i = 0; i < e2->nexps; i++) {
            printf(", ");
            print_exp(e2->exps[i]);
        }
        printf(")");
        break;
    }
    case ARRAY_EXP: {
        ArrayExp *e2 = (ArrayExp *)e;
        printf("array(");
        print_exp(e2->length);
        printf(", ");
        print_exp(e2->init);
        printf(")");
        break;
    }
    case OBJECT_EXP: {
        ObjectExp *e2 = (ObjectExp *)e;
        printf("object : (");
        for (int i = 0; i < e2->nslots; i++) {
            if (i > 0)
                printf(" ");
            print_slotstmt(e2->slots[i]);
        }
        printf(")");
        break;
    }
    case SLOT_EXP: {
        SlotExp *e2 = (SlotExp *)e;
        print_exp(e2->exp);
        printf(".%s", e2->name);
        break;
    }
    case SET_SLOT_EXP: {
        SetSlotExp *e2 = (SetSlotExp *)e;
        print_exp(e2->exp);
        printf(".%s = ", e2->name);
        print_exp(e2->value);
        break;
    }
    case CALL_SLOT_EXP: {
        CallSlotExp *e2 = (CallSlotExp *)e;
        print_exp(e2->exp);
        printf(".%s(", e2->name);
        for (int i = 0; i < e2->nargs; i++) {
            if (i > 0)
                printf(", ");
            print_exp(e2->args[i]);
        }
        printf(")");
        break;
    }
    case CALL_EXP: {
        CallExp *e2 = (CallExp *)e;
        printf("%s(", e2->name);
        for (int i = 0; i < e2->nargs; i++) {
            if (i > 0)
                printf(", ");
            print_exp(e2->args[i]);
        }
        printf(")");
        break;
    }
    case SET_EXP: {
        SetExp *e2 = (SetExp *)e;
        printf("%s = ", e2->name);
        print_exp(e2->exp);
        break;
    }
    case IF_EXP: {
        IfExp *e2 = (IfExp *)e;
        printf("if ");
        print_exp(e2->pred);
        printf(" : (");
        print_scopestmt(e2->conseq);
        printf(") else : (");
        print_scopestmt(e2->alt);
        printf(")");
        break;
    }
    case WHILE_EXP: {
        WhileExp *e2 = (WhileExp *)e;
        printf("while ");
        print_exp(e2->pred);
        printf(" : (");
        print_scopestmt(e2->body);
        printf(")");
        break;
    }
    case REF_EXP: {
        RefExp *e2 = (RefExp *)e;
        printf("%s", e2->name);
        break;
    }
    default:
        printf("Unrecognized Expression with tag %d\n", e->tag);
        exit(-1);
    }
}

void print_slotstmt(SlotStmt *s) {
    switch (s->tag) {
    case VAR_STMT: {
        SlotVar *s2 = (SlotVar *)s;
        printf("var %s = ", s2->name);
        print_exp(s2->exp);
        break;
    }
    case FN_STMT: {
        SlotMethod *s2 = (SlotMethod *)s;
        printf("method %s (", s2->name);
        for (int i = 0; i < s2->nargs; i++) {
            if (i > 0)
                printf(", ");
            printf("%s", s2->args[i]);
        }
        printf(") : (");
        print_scopestmt(s2->body);
        printf(")");
        break;
    }
    default:
        printf("Unrecognized slot statement with tag %d\n", s->tag);
        exit(-1);
    }
}

void print_scopestmt(ScopeStmt *s) {
    switch (s->tag) {
    case VAR_STMT: {
        ScopeVar *s2 = (ScopeVar *)s;
        printf("var %s = ", s2->name);
        print_exp(s2->exp);
        break;
    }
    case FN_STMT: {
        ScopeFn *s2 = (ScopeFn *)s;
        printf("defn %s (", s2->name);
        for (int i = 0; i < s2->nargs; i++) {
            if (i > 0)
                printf(", ");
            printf("%s", s2->args[i]);
        }
        printf(") : (");
        print_scopestmt(s2->body);
        printf(")");
        break;
    }
    case SEQ_STMT: {
        ScopeSeq *s2 = (ScopeSeq *)s;
        print_scopestmt(s2->a);
        printf(" ");
        print_scopestmt(s2->b);
        break;
    }
    case EXP_STMT: {
        ScopeExp *s2 = (ScopeExp *)s;
        print_exp(s2->exp);
        break;
    }
    default:
        printf("Unrecognized scope statement with tag %d\n", s->tag);
        exit(-1);
    }
}

//============================================================
//=================== LOADING ================================
//============================================================

static FILE *inputfile;

static char read_byte() {
    int i = fgetc(inputfile);
    if (i < 0) {
        printf("Unexpected end of file.\n");
        exit(-1);
    }
    return (char)i;
}
static int read_int() {
    unsigned char b1 = read_byte();
    unsigned char b2 = read_byte();
    unsigned char b3 = read_byte();
    unsigned char b4 = read_byte();
    return (int)b1 + ((int)b2 << 8) + ((int)b3 << 16) + ((int)b4 << 24);
}
static char *read_string() {
    int len = read_int();
    char *str = malloc(len + 1);
    for (int i = 0; i < len; i++)
        str[i] = read_byte();
    str[len] = 0;
    return str;
}

// Lists
static char **read_strings(int n) {
    char **strs = malloc(sizeof(char *) * n);
    for (int i = 0; i < n; i++)
        strs[i] = read_string();
    return strs;
}
static Exp **read_exps(int n) {
    Exp **exps = malloc(sizeof(Exp *) * n);
    for (int i = 0; i < n; i++)
        exps[i] = read_exp();
    return exps;
}
static SlotStmt **read_slots(int n) {
    SlotStmt **slots = malloc(sizeof(SlotStmt *) * n);
    for (int i = 0; i < n; i++)
        slots[i] = read_slot();
    return slots;
}

Exp *read_exp() {
    AstTag tag = read_int();
    switch (tag) {
    case INT_EXP: {
        int value = read_int();
        return make_IntExp(value);
    }
    case NULL_EXP: {
        return make_NullExp();
    }
    case PRINTF_EXP: {
        char *format = read_string();
        int nexps = read_int();
        Exp **exps = read_exps(nexps);
        return make_PrintfExp(format, nexps, exps);
    }
    case ARRAY_EXP: {
        Exp *length = read_exp();
        Exp *init = read_exp();
        return make_ArrayExp(length, init);
    }
    case OBJECT_EXP: {
        Exp *parent = read_exp();
        int nslots = read_int();
        SlotStmt **slots = read_slots(nslots);
        return make_ObjectExp(parent, nslots, slots);
    }
    case SLOT_EXP: {
        char *name = read_string();
        Exp *exp = read_exp();
        return make_SlotExp(name, exp);
    }
    case SET_SLOT_EXP: {
        char *name = read_string();
        Exp *exp = read_exp();
        Exp *value = read_exp();
        return make_SetSlotExp(name, exp, value);
    }
    case CALL_SLOT_EXP: {
        char *name = read_string();
        Exp *exp = read_exp();
        int nargs = read_int();
        Exp **args = read_exps(nargs);
        return make_CallSlotExp(name, exp, nargs, args);
    }
    case CALL_EXP: {
        char *name = read_string();
        int nargs = read_int();
        Exp **args = read_exps(nargs);
        return make_CallExp(name, nargs, args);
    }
    case SET_EXP: {
        char *name = read_string();
        Exp *exp = read_exp();
        return make_SetExp(name, exp);
    }
    case IF_EXP: {
        Exp *pred = read_exp();
        ScopeStmt *conseq = read_scopestmt();
        ScopeStmt *alt = read_scopestmt();
        return make_IfExp(pred, conseq, alt);
    }
    case WHILE_EXP: {
        Exp *pred = read_exp();
        ScopeStmt *body = read_scopestmt();
        return make_WhileExp(pred, body);
    }
    case REF_EXP: {
        char *name = read_string();
        return make_RefExp(name);
    }
    default: {
        printf("Expression with unrecognized tag: %d\n", tag);
        exit(-1);
    }
    }
}

SlotStmt *read_slot() {
    AstTag tag = read_int();
    switch (tag) {
    case VAR_STMT: {
        char *name = read_string();
        Exp *exp = read_exp();
        return make_SlotVar(name, exp);
    }
    case FN_STMT: {
        char *name = read_string();
        int nargs = read_int();
        char **args = read_strings(nargs);
        ScopeStmt *body = read_scopestmt();
        return make_SlotMethod(name, nargs, args, body);
    }
    default: {
        printf("Unrecognized slot with tag: %d\n", tag);
        exit(-1);
    }
    }
}

ScopeStmt *read_scopestmt() {
    AstTag tag = read_int();
    switch (tag) {
    case VAR_STMT: {
        char *name = read_string();
        Exp *exp = read_exp();
        return make_ScopeVar(name, exp);
    }
    case FN_STMT: {
        char *name = read_string();
        int nargs = read_int();
        char **args = read_strings(nargs);
        ScopeStmt *body = read_scopestmt();
        return make_ScopeFn(name, nargs, args, body);
    }
    case SEQ_STMT: {
        ScopeStmt *a = read_scopestmt();
        ScopeStmt *b = read_scopestmt();
        return make_ScopeSeq(a, b);
    }
    case EXP_STMT: {
        Exp *e = read_exp();
        return make_ScopeExp(e);
    }
    default: {
        printf("Scope statement with unrecognized tag: %d\n", tag);
        exit(-1);
    }
    }
    return 0;
}

ScopeStmt *read_ast(char *filename) {
    inputfile = fopen(filename, "r");
    if (!inputfile) {
        printf("Could not open file %s\n", filename);
        exit(-1);
    }
    ScopeStmt *s = read_scopestmt();
    fclose(inputfile);
    return s;
}
