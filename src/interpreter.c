#include "feeny/interpreter.h"

int obj_type(Obj *o) {
    switch (o->tag) {
    case NULL_OBJ:
        return 1;
        break;
    case INT_OBJ:
        return 2;
        break;
    case ARRAY_OBJ:
        return 3;
        break;
    case ENV_OBJ:
        return 4;
        break;
    default:
        return -1;
        break;
    }
}

NullObj *make_null_obj() {
    NullObj *obj = (NullObj *)malloc(sizeof(NullObj));
    obj->tag = NULL_OBJ;
    return obj;
}

IntObj *make_int_obj(int value) {
    IntObj *obj = (IntObj *)malloc(sizeof(IntObj));
    obj->tag = INT_OBJ;
    obj->value = value;
    return obj;
}

IntObj *add(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    return make_int_obj(x->value + y->value);
}

IntObj *sub(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    return make_int_obj(x->value - y->value);
}

IntObj *mul(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    return make_int_obj(x->value * y->value);
}

IntObj *divi(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    return make_int_obj(x->value / y->value);
}

IntObj *mod(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    return make_int_obj(x->value % y->value);
}

Obj *lt(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    if (x->value < y->value) {
        return (Obj *)make_int_obj(0);
    } else {
        return (Obj *)make_null_obj();
    }
}

Obj *gt(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    if (x->value > y->value) {
        return (Obj *)make_int_obj(0);
    } else {
        return (Obj *)make_null_obj();
    }
}

Obj *le(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    if (x->value <= y->value) {
        return (Obj *)make_int_obj(0);
    } else {
        return (Obj *)make_null_obj();
    }
}

Obj *ge(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    if (x->value >= y->value) {
        return (Obj *)make_int_obj(0);
    } else {
        return (Obj *)make_null_obj();
    }
}

Obj *eq(IntObj *x, IntObj *y) {
    assert(x->tag == INT_OBJ);
    assert(y->tag == INT_OBJ);
    if (x->value == y->value) {
        return (Obj *)make_int_obj(0);
    } else {
        return (Obj *)make_null_obj();
    }
}

ArrayObj *make_array_obj(IntObj *length, Obj *init) {
    assert(length->tag == INT_OBJ);
    ArrayObj *obj = malloc(sizeof(ArrayObj));
    obj->tag = ARRAY_OBJ;
    obj->elements = make_vector();
    vector_set_length(obj->elements, length->value, init);
    return obj;
}

IntObj *array_length(ArrayObj *obj) {
    assert(obj->tag == ARRAY_OBJ);
    return make_int_obj(obj->elements->size);
}

NullObj *array_set(ArrayObj *obj, IntObj *index, Obj *value) {
    assert(obj->tag == ARRAY_OBJ);
    assert(index->tag == INT_OBJ);
    vector_set(obj->elements, index->value, value);
    return make_null_obj();
}

Obj *array_get(ArrayObj *array, IntObj *index) {
    assert(array->tag == ARRAY_OBJ);
    assert(index->tag == INT_OBJ);
    return (Obj *)vector_get(array->elements, index->value);
}

EnvObj *make_env_obj(Obj *parent) {
    EnvObj *obj = (EnvObj *)malloc(sizeof(EnvObj));
    obj->tag = ENV_OBJ;
    obj->parent = parent;
    obj->names = make_vector();
    obj->entries = make_vector();
    return obj;
}

void free_env_obj(EnvObj *cur) {
    int i = 0;
    for (int i = 0; i < vector_size(cur->entries); i++) {
        free(vector_get(cur->entries, i));
    }
    vector_free(cur->names);
    vector_free(cur->entries);
    free(cur);
}

void add_entry(EnvObj *obj, char *name, Entry *entry) {
    assert(obj->tag == ENV_OBJ);
    // maybe exist duplicated name
    int pos = -1;
    for (int i = 0; i < vector_size(obj->names); i++) {
        if (strcmp(name, (char *)vector_get(obj->names, i)) == 0) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        vector_add(obj->names, name);
        vector_add(obj->entries, entry);
    } else {
        vector_set(obj->entries, pos, entry);
    }
}

Entry *get_entry(EnvObj *obj, char *name) {
    assert(obj->tag == ENV_OBJ);

    int pos = -1;
    for (int i = 0; i < vector_size(obj->names); i++) {
        if (strcmp(name, (char *)vector_get(obj->names, i)) == 0) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return NULL;
    } else {
        return (Entry *)vector_get(obj->entries, pos);
    }
}

/**
 * Inner Interpretation for Fenny language
 */
static int check_print_exp_args_num(char *format, int n) {
    char *c = format;
    int v_index = 0;

    while (*c != '\0') {
        if (*c == '~') {
            v_index++;
        }
        c++;
    }

    if (v_index > n) {
        printf("Error: Not enough arguments for printf\n");
        exit(1);
    }
    if (v_index < n) {
        printf("Error: Too many arguments for printf\n");
        exit(1);
    }

    return STATUS_OK;
}

static void eval_slot(EnvObj *env, SlotStmt *stmt) {
    switch (stmt->tag) {

    case VAR_STMT:
        SlotVar *s = (SlotVar *)stmt;

        VarEntry *entry = (VarEntry *)malloc(sizeof(VarEntry));
        entry->tag = VAR_ENTRY;
        entry->value = eval_exp(env, s->exp);

        add_entry(env, s->name, (Entry *)entry);
        break;

    case FN_STMT:
        SlotMethod *fn = (SlotMethod *)stmt;

        // Create a new entry point
        CodeEntry *codeEntry = (CodeEntry *)malloc(sizeof(CodeEntry));
        codeEntry->tag = CODE_ENTRY;
        codeEntry->params = make_vector();
        for (int i = 0; i < fn->nargs; i++) {
            vector_add(codeEntry->params, fn->args[i]);
        }
        codeEntry->body = fn->body;

        // Add into the environment
        add_entry(env, fn->name, (Entry *)codeEntry);
        break;

    default:
        printf("Error: Unknown slot statement type\n");
        break;
    }
}

static Entry *lookup_entry(EnvObj *env, char *name) {
    // Search entry in current env
    Entry *entry = get_entry(env, name);
    if (entry) {
        return entry;
    }

    // If has parent environment, lookup on parent env
    if (env->parent && env->parent->tag == ENV_OBJ) {
        return lookup_entry((EnvObj *)env->parent, name);
    }

    // Can not find this entry in any envs, return NULL
    return NULL;
}

void print_obj(Obj *obj) {
    switch (obj->tag) {
    case NULL_OBJ:
        printf("Null Object\n");
        break;
    case INT_OBJ:
        printf("Int Object with value: %d\n", ((IntObj *)obj)->value);
        break;
    case ARRAY_OBJ:
        printf("Array Object\n");
        break;
    case ENV_OBJ:
        printf("Environment Object\n");
        break;
    default:
        break;
    }
}

Obj *eval_exp(EnvObj *env, Exp *exp) {
    // Encounter empty Exp, return Null Obj
    if (!exp) {
        return (Obj *)make_null_obj();
    }

    // Core Function
    switch (exp->tag) {
    case INT_EXP: {
        IntExp *e = (IntExp *)exp;
        return (Obj *)make_int_obj(e->value);
    }

    case NULL_EXP: {
        return (Obj *)make_null_obj();
    }

    case PRINTF_EXP: {
        PrintfExp *e = (PrintfExp *)exp;

        Obj **values = malloc(sizeof(Obj *) * e->nexps);
        for (int i = 0; i < e->nexps; i++) {
            values[i] = eval_exp(env, e->exps[i]);
            // Ensure each argument is IntObj
            if (values[i]->tag != INT_OBJ) {
                printf("Error: printf only accepts integers\n but found ");
                print_obj(values[i]);
                exit(1);
            }
        }

        // Replace all ~ with acutal value
        if (check_print_exp_args_num(e->format, e->nexps) == STATUS_OK) {
            char *format = e->format;
            int value_index = 0;
            while (*format != '\0') {
                if (*format == '~') {
                    Obj *value = values[value_index++];
                    printf("%d", ((IntObj *)value)->value);
                } else {
                    putchar(*format);
                }
                format++;
            }
        }

        return (Obj *)make_null_obj();
    }

    case ARRAY_EXP: {
        ArrayExp *e = (ArrayExp *)exp;

        Obj *len = eval_exp(env, e->length);
        if (len->tag != INT_OBJ) {
            printf("Error: Array length must be integer\n");
            exit(1);
        }
        Obj *init = eval_exp(env, e->init);

        return (Obj *)make_array_obj((IntObj *)len, init);
    }

    case OBJECT_EXP: {
        ObjectExp *e = (ObjectExp *)exp;

        Obj *parent = eval_exp(env, e->parent);

        EnvObj *obj = make_env_obj((Obj *)env);

        if (parent->tag == ENV_OBJ) {
            EnvObj *parent_env = (EnvObj *)parent;
            for (int i = 0; i < parent_env->names->size; i++) {
                char *name = (char *)vector_get(parent_env->names, i);
                Entry *entry = (Entry *)vector_get(parent_env->entries, i);
                add_entry(obj, name, entry);
            }
        }

        for (int i = 0; i < e->nslots; i++) {
            eval_slot(obj, e->slots[i]);
        }

        return (Obj *)obj;
    }

    case SLOT_EXP: {
        SlotExp *e = (SlotExp *)exp;

        // First evaluate the expression value
        Obj *obj = eval_exp(env, e->exp);
        if (obj->tag != ENV_OBJ) {
            printf("Error: Cannot access slot of non-object\n");
            exit(STATUS_ERROR);
        }

        // Look up the entry in the object's environment
        Entry *entry = lookup_entry((EnvObj *)obj, e->name);
        if (!entry) {
            printf("Error: Undefined slot '%s'\n", e->name);
            exit(STATUS_ERROR);
        }

        if (entry->tag == VAR_ENTRY) {
            VarEntry *var = (VarEntry *)entry;
            return var->value;
        }

        if (entry->tag == CODE_ENTRY) {
            return (Obj *)make_null_obj();
        }

        printf("Error: Invalid entry type\n");
        exit(STATUS_ERROR);
    }

    case SET_SLOT_EXP: {
        SetSlotExp *e = (SetSlotExp *)exp;

        // First evaluate the object expression
        Obj *obj = eval_exp(env, e->exp);
        if (obj->tag != ENV_OBJ) {
            printf("Error: Cannot set slot of non-object\n");
            exit(STATUS_ERROR);
        }

        // Evaluate the value to be set
        Obj *value = eval_exp(env, e->value);

        // Create or update the entry
        VarEntry *entry = (VarEntry *)malloc(sizeof(VarEntry));
        entry->tag = VAR_ENTRY;
        entry->value = value;

        // Add or update the entry in the object's environment
        add_entry((EnvObj *)obj, e->name, (Entry *)entry);

        return value;
    }

    case CALL_SLOT_EXP: {
        CallSlotExp *e = (CallSlotExp *)exp;
        // Fetch receiver Env Object
        Obj *receiver = eval_exp(env, e->exp);
        switch (receiver->tag) {
        case INT_OBJ:
            // Type check
            if (e->nargs != 1) {
                printf("Error: Int Object slot invoke must take another argument\n");
                exit(1);
            }
            Obj *other = eval_exp(env, e->args[0]);
            if (other->tag != INT_OBJ) {
                printf("Error: Type check error on Int Object invoke\n");
                exit(1);
            }
            IntObj *operand = (IntObj *)other;

            // According to name, select target operation
            if (strcmp(e->name, "add") == 0) {
                return (Obj *)add((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "sub") == 0) {
                return (Obj *)sub((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "mul") == 0) {
                return (Obj *)mul((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "div") == 0) {
                return (Obj *)divi((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "mod") == 0) {
                return (Obj *)mod((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "lt") == 0) {
                return (Obj *)lt((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "gt") == 0) {
                return (Obj *)gt((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "le") == 0) {
                return (Obj *)le((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "ge") == 0) {
                return (Obj *)ge((IntObj *)receiver, operand);
            } else if (strcmp(e->name, "eq") == 0) {
                return (Obj *)eq((IntObj *)receiver, operand);
            } else {
                printf("Error: Unsupported Int Object operation: %s\n", e->name);
                exit(1);
            }

        case ARRAY_OBJ:
            ArrayObj *arr = (ArrayObj *)receiver;
            if (strcmp(e->name, "set") == 0) {
                if (e->nargs != 2) {
                    printf("Error: Array Object set operation must take two arguments\n");
                    exit(1);
                }
                Obj *index = eval_exp(env, e->args[0]);
                Obj *value = eval_exp(env, e->args[1]);

                if (index->tag != INT_OBJ) {
                    printf("Error: Array index must be integer\n");
                    exit(1);
                }

                return (Obj *)array_set(arr, (IntObj *)index, value);

            } else if (strcmp(e->name, "get") == 0) {
                if (e->nargs != 1) {
                    printf("Error: Array Object get operation must take one argument\n");
                    exit(1);
                }
                Obj *index = eval_exp(env, e->args[0]);

                if (index->tag != INT_OBJ) {
                    printf("Error: Array index must be integer\n");
                    exit(1);
                }

                return array_get(arr, (IntObj *)index);

            } else if (strcmp(e->name, "length") == 0) {
                if (e->nargs != 0) {
                    printf("Error: Array Object length operation takes no arguments\n");
                    exit(1);
                }
                return (Obj *)array_length(arr);

            } else {
                printf("Error: Unsupported Array Object operation: %s\n", e->name);
                exit(1);
            }

        case ENV_OBJ:
            Entry *entry = lookup_entry((EnvObj *)receiver, e->name);
            if (!entry || entry->tag != CODE_ENTRY) {
                printf("Undefined function: %s\n", e->name);
                exit(1);
            }

            CodeEntry *code = (CodeEntry *)entry;
            if (e->nargs != vector_size(code->params)) {
                printf("Wrong number of arguments for function %s\n", e->name);
                exit(1);
            }

            // Create a new EnvObj, bind `this` into env
            EnvObj *func_env = make_env_obj(receiver);
            VarEntry *this_entry = (VarEntry *)malloc(sizeof(VarEntry));
            this_entry->tag = VAR_ENTRY;
            this_entry->value = receiver;
            add_entry(func_env, "this", (Entry *)this_entry);

            // Evalue each parameter and binding
            for (int i = 0; i < e->nargs; i++) {
                Obj *arg_value = eval_exp(env, e->args[i]);
                VarEntry *arg_entry = (VarEntry *)malloc(sizeof(VarEntry));
                arg_entry->tag = VAR_ENTRY;
                arg_entry->value = arg_value;
                add_entry(func_env, (char *)vector_get(code->params, i), (Entry *)arg_entry);
            }

            // execute the statement
            return exec_stmt(func_env, code->body);
        default:
            printf("Error: Cannot invoke any operation on Null Object\n");
            exit(1);
            break;
        }
    }

    case CALL_EXP: {
        CallExp *e = (CallExp *)exp;

        Entry *entry = lookup_entry(env, e->name);
        if (!entry || entry->tag != CODE_ENTRY) {
            printf("Undefined function: %s\n", e->name);
            exit(1);
        }

        CodeEntry *code = (CodeEntry *)entry;
        if (e->nargs != vector_size(code->params)) {
            printf("Wrong number of arguments for function %s\n", e->name);
            exit(1);
        }

        // Create a new EnvObj
        EnvObj *func_env = make_env_obj((Obj *)env);

        // Evalue each parameter and binding
        for (int i = 0; i < e->nargs; i++) {
            Obj *arg_value = eval_exp(env, e->args[i]);
            VarEntry *arg_entry = (VarEntry *)malloc(sizeof(VarEntry));
            arg_entry->tag = VAR_ENTRY;
            arg_entry->value = arg_value;
            add_entry(func_env, (char *)vector_get(code->params, i), (Entry *)arg_entry);
        }

        // execute the statement
        return exec_stmt(func_env, code->body);
    }

    case SET_EXP: {
        SetExp *e = (SetExp *)exp;
        Entry *entry = lookup_entry(env, e->name);
        if (!entry || entry->tag != VAR_ENTRY) {
            printf("Cannot assign to undefined variable or function: %s\n", e->name);
            exit(1);
        }

        Obj *value = eval_exp(env, e->exp);
        ((VarEntry *)entry)->value = value;
        return value;
    }

    case IF_EXP: {
        IfExp *e = (IfExp *)exp;
        Obj *pred = eval_exp(env, e->pred);

        EnvObj *branch_env = make_env_obj((Obj *)env);
        if (pred->tag == NULL_OBJ) {
            if (e->alt) {
                return exec_stmt(branch_env, e->alt);
            }
        } else {
            return exec_stmt(branch_env, e->conseq);
        }
        free_env_obj(branch_env);
        return (Obj *)make_null_obj();
    }

    case WHILE_EXP: {
        WhileExp *e = (WhileExp *)exp;
        EnvObj *loop_env = make_env_obj((Obj *)env);
        while (1) {
            Obj *pred = eval_exp(loop_env, e->pred);
            if (pred->tag == NULL_OBJ) {
                break;
            }
            exec_stmt(loop_env, e->body);
        }
        free_env_obj(loop_env);
        return (Obj *)make_null_obj();
    }

    case REF_EXP: {
        RefExp *e = (RefExp *)exp;
        Entry *entry = lookup_entry(env, e->name);
        if (!entry) {
            printf("Undefined variable: %s\n", e->name);
            exit(1);
        }
        if (entry->tag == VAR_ENTRY) {
            return ((VarEntry *)entry)->value;
        } else {
            printf("Cannot reference a function directly: %s\n", e->name);
            exit(1);
        }
    }

    default: {
        print_exp(exp);
        printf("Unknown expression type!\n");
        exit(1);
    }
    }
}

Obj *exec_stmt(EnvObj *env, ScopeStmt *stmt) {
    switch (stmt->tag) {
    case VAR_STMT: {
        ScopeVar *s = (ScopeVar *)stmt;

        // Evaluate initial expression result
        Obj *value = eval_exp(env, s->exp);

        // Create a new entry point
        VarEntry *entry = (VarEntry *)malloc(sizeof(VarEntry));
        entry->tag = VAR_ENTRY;
        entry->value = value;

        // Add into the environment
        add_entry(env, s->name, (Entry *)entry);

        return (Obj *)make_null_obj();
    }

    case SEQ_STMT: {
        ScopeSeq *s = (ScopeSeq *)stmt;

        // Execute two statement one by one
        exec_stmt(env, s->a);
        return exec_stmt(env, s->b);
    }

    case EXP_STMT: {
        ScopeExp *s = (ScopeExp *)stmt;

        // Evaluate the expression without return value
        return eval_exp(env, s->exp);
    }

    case FN_STMT: {
        ScopeFn *fn = (ScopeFn *)stmt;

        // Create a new entry point
        CodeEntry *entry = (CodeEntry *)malloc(sizeof(CodeEntry));
        entry->tag = CODE_ENTRY;
        entry->params = make_vector();
        for (int i = 0; i < fn->nargs; i++) {
            vector_add(entry->params, fn->args[i]);
        }
        entry->body = fn->body;

        // Add into the environment
        add_entry(env, fn->name, (Entry *)entry);

        return (Obj *)make_null_obj();
    }

    default: {
        printf("Unknown statement!!!");
    }
    }
}

void interpret(ScopeStmt *stmt) {
    // printf("Interpret program:\n");
    // print_scopestmt(stmt);
    // printf("\n");

    // Global environment object, its parent is Null Obj
    EnvObj *global_env = make_env_obj((Obj *)make_null_obj());
    exec_stmt(global_env, stmt);
}
