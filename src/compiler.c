#include "feeny/compiler.h"

FuncContext *newFuncContext() {
    FuncContext *context = (FuncContext *)malloc(sizeof(FuncContext));
    context->instructions = make_vector();
    context->args = make_vector();
    context->locals = make_vector();
    context->nargs = 0;
    context->nlocals = 0;
    context->flag = LOCAL;
    return context;
}

ObjContext *newObjContext() {
    ObjContext *context = (ObjContext *)malloc(sizeof(ObjContext));
    context->names = make_vector();
    context->slots = make_vector();
    context->prev = NULL;
}

Value *newNullValue() {
    Value *rv = (Value *)malloc(sizeof(Value));
    rv->tag = NULL_VAL;
    return rv;
}

IntValue *newIntValue(int v) {
    IntValue *rv = (IntValue *)malloc(sizeof(IntValue));
    rv->tag = INT_VAL;
    rv->value = v;
    return rv;
}

StringValue *newStringValue(char *v) {
    StringValue *rv = (StringValue *)malloc(sizeof(StringValue));
    rv->tag = STRING_VAL;
    rv->value = v;
    return rv;
}

MethodValue *newMethodValue(int name, int nargs, int nlocals, Vector *code) {
    MethodValue *rv = (MethodValue *)malloc(sizeof(MethodValue));
    rv->code = code;
    rv->name = name;
    rv->nargs = nargs;
    rv->nlocals = nlocals;
    return rv;
}

SlotValue *newSlotValue(int name) {
    SlotValue *rv = (SlotValue *)malloc(sizeof(SlotValue));
    rv->tag = SLOT_VAL;
    rv->name = name;
    return rv;
}

ClassValue *newClassValue(Vector *slots) {
    ClassValue *rv = (ClassValue *)malloc(sizeof(ClassValue));
    rv->tag = CLASS_VAL;
    rv->slots = slots;
    return rv;
}

/* Assumption: Vector only contains int */
static int compare_vectors(Vector *v1, Vector *v2) {
    if (v1->size != v2->size) {
        return v1->size - v2->size;
    }

    for (int i = 0; i < vector_size(v1); i++) {
        int val1 = (intptr_t)vector_get(v1, i);
        int val2 = (intptr_t)vector_get(v1, i);
        if (val1 != val2) {
            return val1 - val2;
        }
    }
    return 0;
}

int compare(Value *v1, Value *v2) {
    if (v1->tag != v2->tag) {
        return v1->tag - v2->tag;
    }

    switch (v1->tag) {
    case INT_VAL: {
        IntValue *i1 = (IntValue *)v1;
        IntValue *i2 = (IntValue *)v2;
        return i1->value - i2->value;
    }

    case STRING_VAL: {
        StringValue *s1 = (StringValue *)v1;
        StringValue *s2 = (StringValue *)v2;
        return strcmp(s1->value, s2->value);
    }

    case METHOD_VAL: {
        MethodValue *m1 = (MethodValue *)v1;
        MethodValue *m2 = (MethodValue *)v2;

        // Compare function's signature
        if (m1->name != m2->name) {
            return m1->name - m2->name;
        }
        if (m1->nargs != m2->nargs) {
            return m1->nargs - m2->nargs;
        }
        if (m1->nlocals != m2->nlocals) {
            return m1->nlocals - m2->nlocals;
        }
        return 0;
    }

    case SLOT_VAL: {
        SlotValue *s1 = (SlotValue *)v1;
        SlotValue *s2 = (SlotValue *)v2;
        return s1->name - s2->name;
    }

    case CLASS_VAL: {
        ClassValue *c1 = (ClassValue *)v1;
        ClassValue *c2 = (ClassValue *)v2;
        return compare_vectors(c1->slots, c2->slots);
    }

    default:
        fprintf(stderr, "Error: Unknown value type during comparison!\n");
        exit(1);
        return 0;
    }
}

static int addConstantValue(Vector *pool, Value *value) {
    for (int i = 0; i < vector_size(pool); i++) {
        Value *existing = (Value *)vector_get(pool, i);
        if (compare(existing, value) == 0) {
            free(value);
            return i;
        }
    }

    vector_add(pool, value);
    return vector_size(pool) - 1;
}

static int findIndexByName(Vector *pool, Value *value) {
    for (int i = 0; i < vector_size(pool); i++) {
        Value *existing = (Value *)vector_get(pool, i);
        if (compare(existing, value) == 0) {
            return i;
        }
    }

    return -1;
}

static void compileExpr(CompileInfo *info, Exp *expr) {
}

static void compileVarStmt(CompileInfo *info, ScopeVar *varStmt) {
    if (info->funcContext->flag == GLOBAL) {
        StringValue *name = newStringValue(strdup(varStmt->name));
        int name_idx = addConstantValue(info->pool, (Value *)name);

        SlotValue *slot = newSlotValue(name_idx);

        vector_add(info->objContext->slots, slot);
        vector_add(info->objContext->names, varStmt->name);

        if (varStmt->exp != NULL) {
            compileExpr(info, varStmt->exp);

            SetGlobalIns *set_ins = (SetGlobalIns *)malloc(sizeof(SetGlobalIns));
            set_ins->tag = SET_GLOBAL_OP;
            set_ins->name = name_idx;
            vector_add(info->funcContext->instructions, set_ins);
        }
    } else {
        vector_add(info->funcContext->locals, varStmt->name);
        int local_idx = info->funcContext->nlocals++;

        if (varStmt->exp != NULL) {
            compileExpr(info, varStmt->exp);

            SetLocalIns *set_ins = (SetLocalIns *)malloc(sizeof(SetLocalIns));
            set_ins->tag = SET_LOCAL_OP;
            set_ins->idx = local_idx + info->funcContext->nargs;
            vector_add(info->funcContext->instructions, set_ins);
        }
    }
}

static void compileFnStmt(CompileInfo *info, ScopeFn *fnStmt) {
    StringValue *name = newStringValue(strdup(fnStmt->name));
    int name_idx = addConstantValue(info->pool, (Value *)name);

    FuncContext *fn_ctx = newFuncContext();
    fn_ctx->flag = LOCAL;

    fn_ctx->nargs = fnStmt->nargs;
    for (int i = 0; i < fnStmt->nargs; i++) {
        vector_add(fn_ctx->args, fnStmt->args[i]);
    }

    FuncContext *old_ctx = info->funcContext;
    info->funcContext = fn_ctx;

    compileScope(info, fnStmt->body);

    MethodValue *method = newMethodValue(
        name_idx,
        fn_ctx->nargs,
        fn_ctx->nlocals,
        fn_ctx->instructions);

    int method_idx = addConstantValue(info->pool, (Value *)method);

    if (old_ctx->flag == GLOBAL) {
        vector_add(info->objContext->slots, (void *)(intptr_t)method_idx);
        vector_add(info->objContext->names, fnStmt->name);
    }

    info->funcContext = old_ctx;
    free(fn_ctx);
}

static void compileScope(CompileInfo *info, ScopeStmt *stmt) {
    switch (stmt->tag) {
    case VAR_STMT:
        compileVarStmt(info, (ScopeVar *)stmt);
        break;

    case FN_STMT:
        compileFnStmt(info, (ScopeFn *)stmt);
        break;

    case SEQ_STMT: {
        ScopeSeq *seq = (ScopeSeq *)stmt;
        compileScope(info, seq->a);
        compileScope(info, seq->b);
        break;
    }

    case EXP_STMT:
        compileExpr(info, ((ScopeExp *)stmt)->exp);
        // Add DROP instruction since expression result is not used
        ByteIns *drop = (ByteIns *)malloc(sizeof(ByteIns));
        drop->tag = DROP_OP;
        vector_add(info->funcContext->instructions, drop);
        break;

    default:
        fprintf(stderr, "Unknown statement type: %d\n", stmt->tag);
        break;
    }
}

Program *compile(ScopeStmt *stmt) {
    Program *prog = (Program *)malloc(sizeof(Program));

    // Init all compile-related information
    CompileInfo *info = (CompileInfo *)malloc(sizeof(CompileInfo));
    info->pool = make_vector();
    info->funcContext = newFuncContext();
    info->funcContext->flag = GLOBAL;
    info->objContext = newObjContext();

    // Compile program from global level
    compileScope(info, stmt);

    // Add global context's binding function as new slot
    char *str = (char *)malloc(strlen("42entry24") + 1);
    strcpy(str, "42entry24");
    int nameIndex = addConstantValue(info->pool, (Value *)newStringValue(str));
    int entryIndex = addConstantValue(info->pool,
                                      (Value *)newMethodValue(nameIndex, 0, 0, info->funcContext->instructions));

    prog->entry = entryIndex;
    prog->values = info->pool;
    prog->slots = info->objContext->slots;
    return prog;
}
