#include "feeny/compiler.h"
#include <string.h>

// #define DEBUG 1

static int label_counter = 0;

static char *genLabel() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "L%d", label_counter++);
    return strdup(buffer);
}

ScopeContext *newScopeContext(ScopeContext *prev) {
    ScopeContext *context = (ScopeContext *)malloc(sizeof(ScopeContext));

    if (prev != NULL) {
        context->instructions = prev->instructions;
        context->args = prev->args;
        context->nargs = prev->nargs;
        context->nlocals = prev->nlocals;
    } else {
        context->instructions = make_vector();
        context->args = make_vector();
        context->nargs = 0;
        context->nlocals = 0;
    }
    context->locals = make_vector();
    context->flag = LOCAL;
    context->prev = prev;
    return context;
}

ObjContext *newObjContext() {
    ObjContext *context = (ObjContext *)malloc(sizeof(ObjContext));
    context->names = make_vector();
    context->slots = make_vector();
    context->prev = NULL;
    return context;
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
    rv->tag = METHOD_VAL;
    rv->code = code;
    rv->name = name;
    rv->nargs = nargs;
    rv->nlocals = nlocals;
    rv->processed = 0;
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

void addNullInstr(CompileInfo *info) {
    int nullIndex = addConstantValue(info->pool, (Value *)newNullValue());
    LitIns *nullInstr = (LitIns *)malloc(sizeof(LitIns));
    nullInstr->tag = LIT_OP;
    nullInstr->idx = nullIndex;
    vector_add(info->scopeContext->instructions, nullInstr);
}

void addReturnInstr(CompileInfo *info) {
    ByteIns *return_ins = (ByteIns *)malloc(sizeof(ByteIns));
    return_ins->tag = RETURN_OP;
    vector_add(info->scopeContext->instructions, return_ins);
}

void addDropInstr(CompileInfo *info) {
    ByteIns *drop_ins = (ByteIns *)malloc(sizeof(ByteIns));
    drop_ins->tag = DROP_OP;
    vector_add(info->scopeContext->instructions, drop_ins);
}

static int compare_instructions(ByteIns *ins1, ByteIns *ins2) {
    if (ins1->tag != ins2->tag) {
        return ins1->tag - ins2->tag;
    }

    switch (ins1->tag) {
    case LABEL_OP: {
        LabelIns *l1 = (LabelIns *)ins1;
        LabelIns *l2 = (LabelIns *)ins2;
        return l1->name - l2->name;
    }

    case LIT_OP: {
        LitIns *l1 = (LitIns *)ins1;
        LitIns *l2 = (LitIns *)ins2;
        return l1->idx - l2->idx;
    }

    case PRINTF_OP: {
        PrintfIns *p1 = (PrintfIns *)ins1;
        PrintfIns *p2 = (PrintfIns *)ins2;
        if (p1->format != p2->format)
            return p1->format - p2->format;
        return p1->arity - p2->arity;
    }

    case OBJECT_OP: {
        ObjectIns *o1 = (ObjectIns *)ins1;
        ObjectIns *o2 = (ObjectIns *)ins2;
        return o1->class - o2->class;
    }

    case SLOT_OP: {
        SlotIns *s1 = (SlotIns *)ins1;
        SlotIns *s2 = (SlotIns *)ins2;
        return s1->name - s2->name;
    }

    case SET_SLOT_OP: {
        SetSlotIns *s1 = (SetSlotIns *)ins1;
        SetSlotIns *s2 = (SetSlotIns *)ins2;
        return s1->name - s2->name;
    }

    case CALL_SLOT_OP: {
        CallSlotIns *c1 = (CallSlotIns *)ins1;
        CallSlotIns *c2 = (CallSlotIns *)ins2;
        if (c1->name != c2->name)
            return c1->name - c2->name;
        return c1->arity - c2->arity;
    }

    case CALL_OP: {
        CallIns *c1 = (CallIns *)ins1;
        CallIns *c2 = (CallIns *)ins2;
        if (c1->name != c2->name)
            return c1->name - c2->name;
        return c1->arity - c2->arity;
    }

    case SET_LOCAL_OP: {
        SetLocalIns *s1 = (SetLocalIns *)ins1;
        SetLocalIns *s2 = (SetLocalIns *)ins2;
        return s1->idx - s2->idx;
    }

    case GET_LOCAL_OP: {
        GetLocalIns *g1 = (GetLocalIns *)ins1;
        GetLocalIns *g2 = (GetLocalIns *)ins2;
        return g1->idx - g2->idx;
    }

    case SET_GLOBAL_OP: {
        SetGlobalIns *s1 = (SetGlobalIns *)ins1;
        SetGlobalIns *s2 = (SetGlobalIns *)ins2;
        return s1->name - s2->name;
    }

    case GET_GLOBAL_OP: {
        GetGlobalIns *g1 = (GetGlobalIns *)ins1;
        GetGlobalIns *g2 = (GetGlobalIns *)ins2;
        return g1->name - g2->name;
    }

    case BRANCH_OP: {
        BranchIns *b1 = (BranchIns *)ins1;
        BranchIns *b2 = (BranchIns *)ins2;
        return b1->name - b2->name;
    }

    case GOTO_OP: {
        GotoIns *g1 = (GotoIns *)ins1;
        GotoIns *g2 = (GotoIns *)ins2;
        return g1->name - g2->name;
    }

    case RETURN_OP:
    case DROP_OP:
        return 0;

    default:
        fprintf(stderr, "Error: Unknown instruction type in comparison!\n");
        exit(1);
    }
}

/* Assumption: Vector only contains int */
static int compare_vectors(Vector *v1, Vector *v2, int isIntVector) {
    if (v1->size != v2->size) {
        return v1->size - v2->size;
    }

    for (int i = 0; i < vector_size(v1); i++) {
        void *elem1 = vector_get(v1, i);
        void *elem2 = vector_get(v2, i);

        if (isIntVector == 1) {
            intptr_t val1 = (intptr_t)elem1;
            intptr_t val2 = (intptr_t)elem2;
            if (val1 != val2)
                return val1 - val2;
        } else {
            ByteIns *ins1 = (ByteIns *)elem1;
            ByteIns *ins2 = (ByteIns *)elem2;
            int result = compare_instructions(ins1, ins2);
            if (result != 0)
                return result;
        }
    }
    return 0;
}

static int compare(Value *v1, Value *v2) {
    if (v1->tag != v2->tag) {
        return v1->tag - v2->tag;
    }

    switch (v1->tag) {
    case NULL_VAL: {
        return 0;
    }

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
        return compare_vectors(m1->code, m2->code, 0);
    }

    case SLOT_VAL: {
        SlotValue *s1 = (SlotValue *)v1;
        SlotValue *s2 = (SlotValue *)v2;
        return s1->name - s2->name;
    }

    case CLASS_VAL: {
        ClassValue *c1 = (ClassValue *)v1;
        ClassValue *c2 = (ClassValue *)v2;
        return compare_vectors(c1->slots, c2->slots, 1);
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

static int findLocalVar(ScopeContext *context, char *name) {
    // Only when in function's scope will have args
    // All scope context will share the same args
    for (int i = 0; i < context->nargs; i++) {
        if (strcmp((char *)vector_get(context->args, i), name) == 0) {
            return i;
        }
    }

    ScopeContext *current = context;
    while (current != NULL) {
        for (int i = 0; i < vector_size(current->locals); i++) {
            if (strcmp((char *)vector_get(current->locals, i), name) == 0) {
                if (current->prev != NULL) {
                    return i + current->nargs + current->prev->nlocals;
                } else {
                    return i + current->nargs;
                }
            }
        }
        current = current->prev;
    }

    return -1;
}

static VarLocation findObjVar(ObjContext *context, char *name) {
    VarLocation location;
    location.index = -1;

    for (int i = 0; i < vector_size(context->names); i++) {
        if (strcmp((char *)vector_get(context->names, i), name) == 0) {
            location.index = (intptr_t)vector_get(context->slots, i);
            location.type = (context->prev == NULL) ? GLOBAL_VAR : SLOT_VAR;
            return location;
        }
    }

    if (context->prev != NULL) {
        return findObjVar(context->prev, name);
    }

    return location;
}

static void compileLiteral(CompileInfo *info, Value *value) {
    int val_idx = addConstantValue(info->pool, value);
    LitIns *lit = (LitIns *)malloc(sizeof(LitIns));
    lit->tag = LIT_OP;
    lit->idx = val_idx;
    vector_add(info->scopeContext->instructions, lit);
}

static void compilePrintf(CompileInfo *info, PrintfExp *printf_exp) {
    for (int i = 0; i < printf_exp->nexps; i++) {
        compileExpr(info, printf_exp->exps[i]);
    }

    StringValue *format = newStringValue(strdup(printf_exp->format));
    int format_idx = addConstantValue(info->pool, (Value *)format);

    PrintfIns *printf_ins = (PrintfIns *)malloc(sizeof(PrintfIns));
    printf_ins->tag = PRINTF_OP;
    printf_ins->format = format_idx;
    printf_ins->arity = printf_exp->nexps;
    vector_add(info->scopeContext->instructions, printf_ins);
}

static void compileMethodCall(CompileInfo *info, CallSlotExp *call) {
    compileExpr(info, call->exp);

    for (int i = 0; i < call->nargs; i++) {
        compileExpr(info, call->args[i]);
    }
    StringValue *name = newStringValue(strdup(call->name));
    int name_idx = addConstantValue(info->pool, (Value *)name);

    CallSlotIns *call_ins = (CallSlotIns *)malloc(sizeof(CallSlotIns));
    call_ins->tag = CALL_SLOT_OP;
    call_ins->name = name_idx;
    call_ins->arity = call->nargs + 1;
    vector_add(info->scopeContext->instructions, call_ins);
}

static void compileFunctionCall(CompileInfo *info, CallExp *call) {
    for (int i = 0; i < call->nargs; i++) {
        compileExpr(info, call->args[i]);
    }

    StringValue *name = newStringValue(strdup(call->name));
    int name_idx = addConstantValue(info->pool, (Value *)name);

    CallIns *call_ins = (CallIns *)malloc(sizeof(CallIns));
    call_ins->tag = CALL_OP;
    call_ins->name = name_idx;
    call_ins->arity = call->nargs;
    vector_add(info->scopeContext->instructions, call_ins);
}

static void compileArray(CompileInfo *info, ArrayExp *array) {
    compileExpr(info, array->length);
    if (array->init != NULL) {
        compileExpr(info, array->init);
    }
    ByteIns *new_array = (ByteIns *)malloc(sizeof(ByteIns));
    new_array->tag = ARRAY_OP;
    vector_add(info->scopeContext->instructions, new_array);
}

static void compileSlotAccess(CompileInfo *info, SlotExp *slot) {
    compileExpr(info, slot->exp);

    StringValue *name = newStringValue(strdup(slot->name));
    int name_idx = addConstantValue(info->pool, (Value *)name);

    SlotIns *slot_ins = (SlotIns *)malloc(sizeof(SlotIns));
    slot_ins->tag = SLOT_OP;
    slot_ins->name = name_idx;
    vector_add(info->scopeContext->instructions, slot_ins);
}

static void compileSetSlot(CompileInfo *info, SetSlotExp *expr) {
    compileExpr(info, expr->exp);

    compileExpr(info, expr->value);
    StringValue *name = newStringValue(expr->name);
    int name_idx = addConstantValue(info->pool, (Value *)name);

    SetSlotIns *set_slot = (SetSlotIns *)malloc(sizeof(SetSlotIns));
    set_slot->tag = SET_SLOT_OP;
    set_slot->name = name_idx;
    vector_add(info->scopeContext->instructions, set_slot);
}

static void collectVarSlot(CompileInfo *info, SlotVar *var_slot) {
    VarLocation loc = findObjVar(info->objContext, var_slot->name);
    if (loc.index >= 0) {
        fprintf(stderr, "Error: Variable slot '%s' already defined\n", var_slot->name);
        exit(1);
    }

    char *name = strdup(var_slot->name);
    StringValue *slotName = newStringValue(name);
    int name_idx = addConstantValue(info->pool, (Value *)slotName);

    SlotValue *slot = newSlotValue(name_idx);
    int slot_idx = addConstantValue(info->pool, (Value *)slot);

    vector_add(info->objContext->names, name);
    vector_add(info->objContext->slots, (void *)(intptr_t)slot_idx);
}

static void compileMethodSlot(CompileInfo *info, SlotMethod *method_slot) {
    VarLocation loc = findObjVar(info->objContext, method_slot->name);
    if (loc.index >= 0) {
        fprintf(stderr, "Error: Method slot '%s' already defined\n", method_slot->name);
        exit(1);
    }

    char *name = strdup(method_slot->name);
    StringValue *methodName = newStringValue(name);
    int name_idx = addConstantValue(info->pool, (Value *)methodName);

    ScopeContext *prev_scope = info->scopeContext;

    info->scopeContext = newScopeContext(NULL);
    info->scopeContext->nargs = method_slot->nargs + 1;
    info->scopeContext->nlocals = 0;

    vector_add(info->scopeContext->args, (void *)strdup("this"));
    for (int i = 0; i < method_slot->nargs; i++) {
        vector_add(info->scopeContext->args, method_slot->args[i]);
    }

    compileScope(info, method_slot->body);
    addReturnInstr(info);

    MethodValue *method = newMethodValue(
        name_idx,
        info->scopeContext->nargs,
        info->scopeContext->nlocals,
        info->scopeContext->instructions);
    int method_idx = addConstantValue(info->pool, (Value *)method);
    vector_add(info->objContext->names, name);
    vector_add(info->objContext->slots, (void *)(intptr_t)method_idx);

    info->scopeContext = prev_scope;
}

static void compileObject(CompileInfo *info, ObjectExp *obj) {
    if (obj->parent != NULL) {
        compileExpr(info, obj->parent);
    } else {
        addNullInstr(info);
    }

    ObjContext *obj_ctx = newObjContext();
    obj_ctx->prev = info->objContext;
    info->objContext = obj_ctx;

    for (int i = 0; i < obj->nslots; i++) {
        SlotStmt *slot = obj->slots[i];
        switch (slot->tag) {
        case FN_STMT:
            compileMethodSlot(info, (SlotMethod *)slot);
            break;
        case VAR_STMT:
            collectVarSlot(info, (SlotVar *)slot);
            break;
        default:
            fprintf(stderr, "Error: Unknown slot statement type\n");
            exit(1);
        }
    }

    ClassValue *class_val = newClassValue(info->objContext->slots);
    int class_idx = addConstantValue(info->pool, (Value *)class_val);

    for (int i = 0; i < obj->nslots; i++) {
        SlotStmt *slot = obj->slots[i];
        if (slot->tag == VAR_STMT) {
            SlotVar *var_slot = (SlotVar *)slot;
            if (var_slot->exp != NULL) {
                compileExpr(info, var_slot->exp);
            }
        }
    }

    ObjectIns *new_obj = (ObjectIns *)malloc(sizeof(ObjectIns));
    new_obj->tag = OBJECT_OP;
    new_obj->class = class_idx;
    vector_add(info->scopeContext->instructions, new_obj);

    info->objContext = obj_ctx->prev;
    free(obj_ctx);
}

static void compileIfExpr(CompileInfo *info, IfExp *ifExp) {
    StringValue *conseq_label = newStringValue(genLabel());
    int conseq_idx = addConstantValue(info->pool, (Value *)conseq_label);
    StringValue *end_label = newStringValue(genLabel());
    int end_idx = addConstantValue(info->pool, (Value *)end_label);

    compileExpr(info, ifExp->pred);

    BranchIns *branch = (BranchIns *)malloc(sizeof(BranchIns));
    branch->tag = BRANCH_OP;
    branch->name = conseq_idx;
    vector_add(info->scopeContext->instructions, branch);

    // Else block
    if (ifExp->alt->tag == EXP_STMT && ((ScopeExp *)ifExp->alt)->exp->tag == NULL_EXP) {
    } else {
        ScopeContext *alt_scope = newScopeContext(info->scopeContext);
        info->scopeContext = alt_scope;

        compileScope(info, ifExp->alt);

        if (alt_scope->nlocals > alt_scope->prev->nlocals) {
            alt_scope->prev->nlocals = alt_scope->nlocals;
        }
        info->scopeContext = alt_scope->prev;
        free(alt_scope);
    }

    GotoIns *goto_end = (GotoIns *)malloc(sizeof(GotoIns));
    goto_end->tag = GOTO_OP;
    goto_end->name = end_idx;
    vector_add(info->scopeContext->instructions, goto_end);

    LabelIns *conseq_ins = (LabelIns *)malloc(sizeof(LabelIns));
    conseq_ins->tag = LABEL_OP;
    conseq_ins->name = conseq_idx;
    vector_add(info->scopeContext->instructions, conseq_ins);

    ScopeContext *conseq_scope = newScopeContext(info->scopeContext);
    info->scopeContext = conseq_scope;

    compileScope(info, ifExp->conseq);

    if (conseq_scope->nlocals > conseq_scope->prev->nlocals) {
        conseq_scope->prev->nlocals = conseq_scope->nlocals;
    }
    info->scopeContext = conseq_scope->prev;
    free(conseq_scope);

    LabelIns *end_ins = (LabelIns *)malloc(sizeof(LabelIns));
    end_ins->tag = LABEL_OP;
    end_ins->name = end_idx;
    vector_add(info->scopeContext->instructions, end_ins);
}

static void compileWhileExpr(CompileInfo *info, WhileExp *whileExp) {
    StringValue *loop_label = newStringValue(genLabel());
    int loop_idx = addConstantValue(info->pool, (Value *)loop_label);
    StringValue *body_label = newStringValue(genLabel());
    int body_idx = addConstantValue(info->pool, (Value *)body_label);

    GotoIns *goto_loop = (GotoIns *)malloc(sizeof(GotoIns));
    goto_loop->tag = GOTO_OP;
    goto_loop->name = loop_idx;
    vector_add(info->scopeContext->instructions, goto_loop);

    LabelIns *body_ins = (LabelIns *)malloc(sizeof(LabelIns));
    body_ins->tag = LABEL_OP;
    body_ins->name = body_idx;
    vector_add(info->scopeContext->instructions, body_ins);

    ScopeContext *body_scope = newScopeContext(info->scopeContext);
    info->scopeContext = body_scope;

    compileScope(info, whileExp->body);

    if (body_scope->nlocals > body_scope->prev->nlocals) {
        body_scope->prev->nlocals = body_scope->nlocals;
    }
    info->scopeContext = body_scope->prev;
    free(body_scope);

    LabelIns *loop_ins = (LabelIns *)malloc(sizeof(LabelIns));
    loop_ins->tag = LABEL_OP;
    loop_ins->name = loop_idx;
    vector_add(info->scopeContext->instructions, loop_ins);

    compileExpr(info, whileExp->pred);

    BranchIns *branch = (BranchIns *)malloc(sizeof(BranchIns));
    branch->tag = BRANCH_OP;
    branch->name = body_idx;
    vector_add(info->scopeContext->instructions, branch);
}

static void compileRefExpr(CompileInfo *info, RefExp *ref) {
    int local_idx = findLocalVar(info->scopeContext, ref->name);
    if (local_idx >= 0) {
        GetLocalIns *get = (GetLocalIns *)malloc(sizeof(GetLocalIns));
        get->tag = GET_LOCAL_OP;
        get->idx = local_idx;
        vector_add(info->scopeContext->instructions, get);
        return;
    }

    VarLocation loc = findObjVar(info->objContext, ref->name);
    if (loc.index >= 0) {
        if (loc.type == SLOT_VAR) {
            StringValue *varName = newStringValue(ref->name);
            int name_idx = addConstantValue(info->pool, (Value *)varName);
            SlotIns *get = (SlotIns *)malloc(sizeof(SlotIns));
            get->tag = SLOT_OP;
            get->name = name_idx;
            vector_add(info->scopeContext->instructions, get);
        } else {
            StringValue *varName = newStringValue(ref->name);
            int name_idx = addConstantValue(info->pool, (Value *)varName);
            GetGlobalIns *get = (GetGlobalIns *)malloc(sizeof(GetGlobalIns));
            get->tag = GET_GLOBAL_OP;
            get->name = name_idx;
            vector_add(info->scopeContext->instructions, get);
        }
        return;
    }

    fprintf(stderr, "Error: Undefined variable '%s'\n", ref->name);
    exit(1);
}

static void compileSetExpr(CompileInfo *info, SetExp *setExp) {
    compileExpr(info, setExp->exp);

    int local_idx = findLocalVar(info->scopeContext, setExp->name);

    if (local_idx >= 0) {
        SetLocalIns *set_local = (SetLocalIns *)malloc(sizeof(SetLocalIns));
        set_local->tag = SET_LOCAL_OP;
        set_local->idx = local_idx;
        vector_add(info->scopeContext->instructions, set_local);
    } else {
        VarLocation loc = findObjVar(info->objContext, setExp->name);

        if (loc.index >= 0) {
            StringValue *nameStr = newStringValue(strdup(setExp->name));
            int name_idx = addConstantValue(info->pool, (Value *)nameStr);
            if (loc.type == GLOBAL_VAR) {
                SetGlobalIns *set_global = (SetGlobalIns *)malloc(sizeof(SetGlobalIns));
                set_global->tag = SET_GLOBAL_OP;
                set_global->name = name_idx;
                vector_add(info->scopeContext->instructions, set_global);
            } else { // SLOT_VAR
                SetSlotIns *set_slot = (SetSlotIns *)malloc(sizeof(SetSlotIns));
                set_slot->tag = SET_SLOT_OP;
                set_slot->name = name_idx;
                vector_add(info->scopeContext->instructions, set_slot);
            }
        } else {
            fprintf(stderr, "Error: Undefined variable '%s' in assignment\n", setExp->name);
            exit(1);
        }
    }
}

static void compileVarStmt(CompileInfo *info, ScopeVar *varStmt) {
    if (info->scopeContext->flag == GLOBAL) {
        StringValue *name = newStringValue(strdup(varStmt->name));
        int name_idx = addConstantValue(info->pool, (Value *)name);

        SlotValue *slot = newSlotValue(name_idx);
        int slot_idx = addConstantValue(info->pool, (Value *)slot);

        vector_add(info->objContext->slots, (void *)(intptr_t)slot_idx);
        vector_add(info->objContext->names, varStmt->name);

        if (varStmt->exp != NULL) {
            compileExpr(info, varStmt->exp);

            SetGlobalIns *set_ins = (SetGlobalIns *)malloc(sizeof(SetGlobalIns));
            set_ins->tag = SET_GLOBAL_OP;
            set_ins->name = name_idx;
            vector_add(info->scopeContext->instructions, set_ins);
        }
    } else {
        StringValue *name = newStringValue(strdup(varStmt->name));
        int name_idx = addConstantValue(info->pool, (Value *)name);
        vector_add(info->scopeContext->locals, varStmt->name);
        info->scopeContext->nlocals++;
        int local_idx = findLocalVar(info->scopeContext, varStmt->name);

        if (varStmt->exp != NULL) {
            compileExpr(info, varStmt->exp);

            SetLocalIns *set_ins = (SetLocalIns *)malloc(sizeof(SetLocalIns));
            set_ins->tag = SET_LOCAL_OP;
            set_ins->idx = local_idx;
            vector_add(info->scopeContext->instructions, set_ins);
        }
    }
}

static void compileExpr(CompileInfo *info, Exp *expr) {
    switch (expr->tag) {
    case NULL_EXP: {
        Value *null_val = newNullValue();
        compileLiteral(info, null_val);
        break;
    }
    case INT_EXP: {
        IntExp *int_exp = (IntExp *)expr;
        IntValue *int_val = newIntValue(int_exp->value);
        compileLiteral(info, (Value *)int_val);
        break;
    }
    case ARRAY_EXP:
        compileArray(info, (ArrayExp *)expr);
        break;
    case PRINTF_EXP:
        compilePrintf(info, (PrintfExp *)expr);
        break;
    case OBJECT_EXP:
        compileObject(info, (ObjectExp *)expr);
        break;
    case SLOT_EXP:
        compileSlotAccess(info, (SlotExp *)expr);
        break;
    case SET_SLOT_EXP:
        compileSetSlot(info, (SetSlotExp *)expr);
        break;
    case CALL_SLOT_EXP:
        compileMethodCall(info, (CallSlotExp *)expr);
        break;
    case CALL_EXP:
        compileFunctionCall(info, (CallExp *)expr);
        break;
    case SET_EXP:
        compileSetExpr(info, (SetExp *)expr);
        break;
    case IF_EXP:
        compileIfExpr(info, (IfExp *)expr);
        break;
    case WHILE_EXP:
        compileWhileExpr(info, (WhileExp *)expr);
        break;
    case REF_EXP:
        compileRefExpr(info, (RefExp *)expr);
        break;
    }
}

static void compileFnStmt(CompileInfo *info, ScopeFn *fnStmt) {
    StringValue *name = newStringValue(strdup(fnStmt->name));
    int name_idx = addConstantValue(info->pool, (Value *)name);

    ScopeContext *old_ctx = info->scopeContext;
    ScopeContext *fn_ctx = newScopeContext(NULL);
    fn_ctx->nargs = fnStmt->nargs;
    for (int i = 0; i < fnStmt->nargs; i++) {
        vector_add(fn_ctx->args, fnStmt->args[i]);
    }
    info->scopeContext = fn_ctx;

    compileScope(info, fnStmt->body);
    addReturnInstr(info);

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

    info->scopeContext = old_ctx;
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
        break;

    default:
        fprintf(stderr, "Unknown statement type: %d\n", stmt->tag);
        break;
    }
}

void interpret_bc(Program *p) {
    printf("Interpreting Bytecode Program:\n");
    print_prog(p);
    printf("\n");
}

Program *compile(ScopeStmt *stmt) {
    Program *prog = (Program *)malloc(sizeof(Program));

    // Init all compile-related information
    CompileInfo *info = (CompileInfo *)malloc(sizeof(CompileInfo));
    info->pool = make_vector();
    info->scopeContext = newScopeContext(NULL);
    info->scopeContext->flag = GLOBAL;
    info->objContext = newObjContext();

    // Compile program from global level
    compileScope(info, stmt);

    // Add return to entry function
    addReturnInstr(info);

    // Add global context's binding function as new slot
    char *str = (char *)malloc(strlen("42entry24") + 1);
    strcpy(str, "42entry24");
    int nameIndex = addConstantValue(info->pool, (Value *)newStringValue(str));
    int entryIndex = addConstantValue(info->pool,
                                      (Value *)newMethodValue(nameIndex, 0, 0, info->scopeContext->instructions));

    prog->entry = entryIndex;
    prog->values = info->pool;
    prog->slots = info->objContext->slots;

#ifdef DEBUG
    interpret_bc(prog);
#endif
    return prog;
}
