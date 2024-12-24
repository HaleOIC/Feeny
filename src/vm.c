#include "feeny/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// #define DEBUG 1

static MethodValue *lookup_method(EnvObj *obj, char *method_name) {
    if (!obj) {
        return NULL;
    }

    MethodValue *rv = (MethodValue *)get_entry(obj, method_name);
    if (rv != NULL) {
        if (rv->tag != METHOD_VAL) {
            fprintf(stderr, "Error: Given %s is not a method!\n", method_name);
            exit(1);
        }
        return rv;
    }
    if (obj->parent && obj->parent->tag == ENV_OBJ) {
        return lookup_method((EnvObj *)obj->parent, method_name);
    }
    return NULL;
}

static void make_frame(Machine *machine, MethodValue *method) {
    // Allocate memory for new frame
    Frame *frame = (Frame *)malloc(sizeof(Frame));
    if (!frame) {
        fprintf(stderr, "Memory allocation failed for new frame\n");
        exit(1);
    }

    // Initialize frame components
    frame->labels = newMap();           // Create new label map for this frame
    frame->locals = make_vector();      // Create vector for local variables
    frame->codes = method->code;        // Point to method's code
    frame->prev = machine->state->cur;  // Link to previous frame
    frame->ra = machine->state->ip + 1; // Save current ip as return address

    // Pre-allocate space for arguments and local variables
    vector_set_length(frame->locals, method->nargs + method->nlocals, NULL);

    // Save current machine state
    int oldIp = machine->state->ip;

    // Temporarily set machine state to scan labels
    machine->state->cur = frame;
    machine->state->ip = 0;

    // Scan through method's code to process all labels
    for (int i = 0; i < vector_size(method->code); i++) {
        machine->state->ip = i;
        ByteIns *instr = vector_get(method->code, i);
        if (instr->tag == LABEL_OP) {
            LabelIns *labelIns = (LabelIns *)instr;
            // Get the label string value from program's constant pool
            Value *labelStr = vector_get(machine->program->values, labelIns->name);

            // Verify the value is a string
            if (labelStr->tag != STRING_VAL) {
                fprintf(stderr, "Label instruction's name must be string\n");
                exit(1);
            }

            // Extract the actual label string and add to frame's label map
            char *label = ((StringValue *)labelStr)->value;
            addNewTuple(frame->labels, label, (void *)(intptr_t)i);
        }
    }

    // Reset ip to start of new frame's code
    machine->state->ip = 0;
}

static State *initState(Program *program) {
    State *s = (State *)malloc(sizeof(State));
    if (!s) {
        fprintf(stderr, "Memory allocation failed for state\n");
        exit(1);
    }
    s->cur = NULL;
    s->stack = make_vector();
    s->ip = 0;

    // Attach global constants to virtual machine
    s->global_variables = newMap();
    for (int i = 0; i < vector_size(program->slots); i++) {
        Value *global = vector_get(program->values, (intptr_t)vector_get(program->slots, i));
        switch (global->tag) {
        case SLOT_VAL:
            SlotValue *slot = (SlotValue *)global;

            Value *slotName = vector_get(program->values, slot->name);
            if (slotName->tag != STRING_VAL) {
                fprintf(stderr, "Global Slot value's name must be string!\n");
                exit(1);
            }
            addNewTuple(s->global_variables, ((StringValue *)slotName)->value, NULL);
            break;

        case METHOD_VAL:
            MethodValue *method = (MethodValue *)global;
            Value *methodName = vector_get(program->values, method->name);
            if (methodName->tag != STRING_VAL) {
                fprintf(stderr, "Global Method value's name must be string!\n");
                exit(1);
            }
            addNewTuple(s->global_variables, ((StringValue *)methodName)->value, method);
            break;
        default:
            fprintf(stderr, "Unknown global values!\n");
            break;
        }
    }
    return s;
}

void initvm(Program *program, Machine *machine) {
    machine->program = program;
    machine->state = initState(program);
    MethodValue *method = vector_get(program->values, program->entry);
    make_frame(machine, method);
    machine->state->cur->ra = -1;
    machine->state->cur->prev = NULL;
}

static void handle_lit_instr(Machine *machine, LitIns *ins) {
    Value *value = vector_get(machine->program->values, ins->idx);
    switch (value->tag) {
    case INT_VAL:
        IntValue *int_value = (IntValue *)value;
        vector_add(machine->state->stack, make_int_obj(int_value->value));
        break;
    case NULL_VAL:
        vector_add(machine->state->stack, make_null_obj());
        break;
    default:
        fprintf(stderr, "Only int or null object can be used in lit\n");
        exit(1);
    }
}

// Handle print instruction
static void handle_print_instr(Machine *machine, PrintfIns *ins) {
    Value *format = vector_get(machine->program->values, ins->format);
    if (format->tag != STRING_VAL) {
        fprintf(stderr, "Print format must be string\n");
        exit(1);
    }

    // Get arguments from stack
    Obj **args = malloc(sizeof(Value *) * ins->arity);
    for (int i = ins->arity - 1; i >= 0; i--) {
        args[i] = vector_pop(machine->state->stack);
        if (args[i]->tag != INT_OBJ) {
            printf("Error: printf only accepts integers\n");
            exit(1);
        }
    }
    char *str_format = ((StringValue *)format)->value;
    int idx = 0;
    while (*str_format != '\0') {
        if (*str_format == '~') {
            Obj *value = args[idx++];
            printf("%d", ((IntObj *)value)->value);
        } else {
            putchar(*str_format);
        }
        str_format++;
    }

    // Cleanup and push null as return value
    free(args);
    vector_add(machine->state->stack, make_null_obj());
}

static void handle_array_instr(Machine *machine) {
    Obj *init_val = vector_pop(machine->state->stack);
    Obj *length_val = vector_pop(machine->state->stack);

    if (length_val->tag != INT_OBJ) {
        fprintf(stderr, "Array length must be integer\n");
        exit(1);
    }

    ArrayObj *array = make_array_obj((IntObj *)length_val, init_val);
    vector_add(machine->state->stack, (Obj *)array);
}

static void handle_object_instr(Machine *machine, ObjectIns *ins) {
    ClassValue *class = vector_get(machine->program->values, ins->class);
    if (class->tag != CLASS_VAL) {
        fprintf(stderr, "Object instruction requires class value\n");
        exit(1);
    }

    // Distinguish n variables slots, m method slots
    int nslots = 0;
    int mslots = 0;
    Vector *names = make_vector();
    Vector *values = make_vector();
    Vector *ids = make_vector();
    for (int i = 0; i < vector_size(class->slots); i++) {
        Value *v = (Value *)vector_get(machine->program->values, (intptr_t)vector_get(class->slots, i));
        switch (v->tag) {
        case METHOD_VAL:
            mslots++;
            MethodValue *method = (MethodValue *)v;
            char *methodName = ((StringValue *)vector_get(machine->program->values, method->name))->value;
            vector_add(names, methodName);
            vector_add(values, method);
            break;
        case SLOT_VAL:
            nslots++;
            SlotValue *slot = (SlotValue *)v;
            char *varName = ((StringValue *)vector_get(machine->program->values, slot->name))->value;
            vector_add(names, varName);
            vector_add(values, NULL);
            vector_add(ids, (void *)(intptr_t)i);
            break;
        default:
            fprintf(stderr, "Error: Object instruction's slots should be Slot or method!\n");
            exit(1);
            break;
        }
    }

    // Pop initial values
    for (int i = nslots - 1; i >= 0; i--) {
        Obj *obj = vector_pop(machine->state->stack);
        vector_set(values, (intptr_t)vector_get(ids, i), obj);
    }

    Obj *parent = vector_pop(machine->state->stack);
    EnvObj *obj = make_env_obj(parent);

    free(obj->names);
    free(obj->entries);
    free(ids);
    obj->names = names;
    obj->entries = values;

    vector_add(machine->state->stack, (Obj *)obj);
}

static void handle_slot_instr(Machine *machine, SlotIns *ins) {
    Obj *receiver = vector_pop(machine->state->stack);
    if (receiver->tag != ENV_OBJ) {
        fprintf(stderr, "Get slot requires object\n");
        exit(1);
    }
    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;

    Obj *obj = (Obj *)get_entry((EnvObj *)receiver, slotName);
    if (!obj) {
        fprintf(stderr, "Invalid slot access\n");
        exit(1);
    }

    vector_add(machine->state->stack, obj);
}

static void handle_set_slot_instr(Machine *machine, SetSlotIns *ins) {
    Obj *value = vector_pop(machine->state->stack);
    Obj *receiver = vector_pop(machine->state->stack);

    if (receiver->tag != ENV_OBJ) {
        fprintf(stderr, "Set slot requires object\n");
        exit(1);
    }
    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;

    add_entry((EnvObj *)receiver, slotName, (Entry *)value);

    vector_add(machine->state->stack, value);
}

static void handle_call_slot_instr(Machine *machine, CallSlotIns *ins) {
    Vector *args = make_vector();
    for (int i = 0; i < ins->arity - 1; i++) {
        vector_add(args, vector_pop(machine->state->stack));
    }

    Obj *receiver = vector_pop(machine->state->stack);
    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;

    if (receiver->tag == INT_OBJ) {
        // Handle integer operations
        if (ins->arity != 2) {
            fprintf(stderr, "Error: Int Object slot invoke must take another argument\n");
            exit(1);
        }

        Obj *other = vector_get(args, 0);
        if (other->tag != INT_OBJ) {
            fprintf(stderr, "Error: Type check error on Int Object invoke\n");
            exit(1);
        }

        IntObj *operand = (IntObj *)other;
        Obj *result = NULL;

        // Handle different integer operations
        if (strcmp(slotName, "add") == 0) {
            result = (Obj *)add((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "sub") == 0) {
            result = (Obj *)sub((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "mul") == 0) {
            result = (Obj *)mul((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "div") == 0) {
            result = (Obj *)divi((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "mod") == 0) {
            result = (Obj *)mod((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "lt") == 0) {
            result = lt((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "gt") == 0) {
            result = gt((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "le") == 0) {
            result = le((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "ge") == 0) {
            result = ge((IntObj *)receiver, operand);
        } else if (strcmp(slotName, "eq") == 0) {
            result = eq((IntObj *)receiver, operand);
        } else {
            fprintf(stderr, "Error: Unsupported Int Object operation: %s\n", slotName);
            exit(1);
        }

        vector_add(machine->state->stack, (Obj *)result);
        machine->state->ip++;
    } else if (receiver->tag == ARRAY_OBJ) {
        ArrayObj *arr = (ArrayObj *)receiver;

        if (strcmp(slotName, "set") == 0) {
            if (ins->arity != 3) {
                fprintf(stderr, "Error: Array Object set operation must take two arguments\n");
                exit(1);
            }

            Obj *index = vector_get(args, 1);
            Obj *value = vector_get(args, 0);

            if (index->tag != INT_OBJ) {
                fprintf(stderr, "Error: Array index must be integer\n");
                exit(1);
            }

            Obj *result = (Obj *)array_set(arr, (IntObj *)index, value);
            vector_add(machine->state->stack, result);

        } else if (strcmp(slotName, "get") == 0) {
            if (ins->arity != 2) {
                fprintf(stderr, "Error: Array Object get operation must take one argument\n");
                exit(1);
            }

            Obj *index = vector_get(args, 0);
            if (index->tag != INT_OBJ) {
                fprintf(stderr, "Error: Array index must be integer\n");
                exit(1);
            }

            Obj *result = array_get(arr, (IntObj *)index);
            vector_add(machine->state->stack, result);

        } else if (strcmp(slotName, "length") == 0) {
            if (ins->arity != 1) {
                fprintf(stderr, "Error: Array Object length operation takes no arguments\n");
                exit(1);
            }

            Obj *result = (Obj *)array_length(arr);
            vector_add(machine->state->stack, result);

        } else {
            fprintf(stderr, "Error: Unsupported Array Object operation: %s\n", slotName);
            exit(1);
        }
        machine->state->ip++;
    } else if (receiver->tag == ENV_OBJ) {
        MethodValue *method = (MethodValue *)lookup_method((EnvObj *)receiver, slotName);
        if (!method || method->tag != METHOD_VAL) {
            fprintf(stderr, "Undefined function: %s\n", slotName);
            exit(1);
        }

        if (ins->arity != method->nargs) {
            fprintf(stderr, "Wrong number of arguments for function %s\n", slotName);
            exit(1);
        }

        // Create new frame with receiver as parent
        make_frame(machine, method);

        vector_set(machine->state->cur->locals, 0, receiver);
        for (int i = 0; i < ins->arity - 1; i++) {
            vector_set(machine->state->cur->locals, ins->arity - i - 1, vector_get(args, i));
        }
    } else {
        fprintf(stderr, "Error: Cannot invoke any operation on Null Object\n");
        exit(1);
    }
    free(args);
}

static void handle_call_instr(Machine *machine, CallIns *ins) {
    StringValue *funcName = vector_get(machine->program->values, ins->name);
    if (funcName->tag != STRING_VAL) {
        fprintf(stderr, "Function name must be string\n");
        exit(1);
    }

    MethodValue *method = findByName(machine->state->global_variables, funcName->value);
    if (!method || method->tag != METHOD_VAL) {
        fprintf(stderr, "Undefined function: %s\n", funcName->value);
        exit(1);
    }

    if (ins->arity != method->nargs) {
        fprintf(stderr, "Wrong number of arguments for function %s\n", funcName->value);
        exit(1);
    }

    make_frame(machine, method);

    for (int i = 0; i < ins->arity; i++) {
        vector_set(machine->state->cur->locals, ins->arity - 1 - i, vector_pop(machine->state->stack));
    }
}
static void handle_get_local_instr(Machine *machine, GetLocalIns *ins) {
    Frame *cur = machine->state->cur;
    vector_add(machine->state->stack, (Obj *)vector_get(cur->locals, ins->idx));
}

static void handle_set_local_instr(Machine *machine, SetLocalIns *ins) {
    Frame *cur = machine->state->cur;
    Obj *top = vector_peek(machine->state->stack);
    vector_set(cur->locals, ins->idx, top);
}

static void handle_get_global_instr(Machine *machine, GetGlobalIns *ins) {
    StringValue *global = (StringValue *)vector_get(machine->program->values, ins->name);
    if (global->tag != STRING_VAL) {
        fprintf(stderr, "Global variable should be string value!\n");
        exit(1);
    }
    Obj *target = findByName(machine->state->global_variables, global->value);
    vector_add(machine->state->stack, target);
}

static void handle_set_global_instr(Machine *machine, SetGlobalIns *ins) {
    StringValue *global = (StringValue *)vector_get(machine->program->values, ins->name);
    if (global->tag != STRING_VAL) {
        fprintf(stderr, "Global variable should be string value!\n");
        exit(1);
    }
    Obj *top = vector_peek(machine->state->stack);
    addNewTuple(machine->state->global_variables, global->value, top);
}

static void handle_goto_instr(Machine *machine, GotoIns *ins) {

    StringValue *labelName = vector_get(machine->program->values, ins->name);
    if (labelName->tag != STRING_VAL) {
        fprintf(stderr, "Label name must be string\n");
        exit(1);
    }

    void *target = findByName(machine->state->cur->labels, labelName->value);
    if (!target) {
        fprintf(stderr, "Label not found: %s\n", labelName->value);
        exit(1);
    }

    machine->state->ip = (int)(intptr_t)target;
}

static void handle_branch_instr(Machine *machine, BranchIns *ins) {
    Obj *condition = vector_pop(machine->state->stack);

    if (condition->tag != NULL_OBJ) {
        StringValue *labelName = vector_get(machine->program->values, ins->name);
        if (labelName->tag != STRING_VAL) {
            fprintf(stderr, "Label name must be string\n");
            exit(1);
        }

        void *target = findByName(machine->state->cur->labels, labelName->value);
        if (!target) {
            fprintf(stderr, "Label not found: %s\n", labelName->value);
            exit(1);
        }

        machine->state->ip = (int)(intptr_t)target;
    } else {
        machine->state->ip++;
    }
}

static void handle_return_instr(Machine *machine) {
    Frame *cur = machine->state->cur;

    machine->state->cur = cur->prev;
    machine->state->ip = cur->ra;

    vector_free(cur->labels->names);
    vector_free(cur->labels->values);
    free(cur->labels);
    vector_free(cur->locals);
    free(cur);
}

static void handle_drop_instr(Machine *machine) {
    vector_pop(machine->state->stack);
}

void runvm(Machine *machine) {
    State *state = machine->state;
    Program *program = machine->program;

    while (1) {

#ifdef DEBUG
        printf("cur ip: %d\n", state->ip);
#endif

        // When encounter the end of current frame, break the loop
        if (state->ip == -1 && state->cur == NULL) {
            break;
        }
        if (state->ip >= vector_size(state->cur->codes)) {
            fprintf(stderr, "Error: execute bytecodes out of bound!\n");
            exit(1);
        }

        ByteIns *instr = (ByteIns *)vector_get(state->cur->codes, state->ip);

#ifdef DEBUG
        print_ins(instr);
        printf("\n");
#endif

        switch (instr->tag) {
        case LABEL_OP:
            break;
        case LIT_OP:
            handle_lit_instr(machine, (LitIns *)instr);
            break;
        case PRINTF_OP:
            handle_print_instr(machine, (PrintfIns *)instr);
            break;
        case ARRAY_OP:
            handle_array_instr(machine);
            break;
        case OBJECT_OP:
            handle_object_instr(machine, (ObjectIns *)instr);
            break;
        case SLOT_OP:
            handle_slot_instr(machine, (SlotIns *)instr);
            break;
        case SET_SLOT_OP:
            handle_set_slot_instr(machine, (SetSlotIns *)instr);
            break;
        case CALL_SLOT_OP:
            handle_call_slot_instr(machine, (CallSlotIns *)instr);
            continue;
            break;
        case CALL_OP:
            handle_call_instr(machine, (CallIns *)instr);
            continue;
            break;
        case GET_LOCAL_OP:
            handle_get_local_instr(machine, (GetLocalIns *)instr);
            break;
        case SET_LOCAL_OP:
            handle_set_local_instr(machine, (SetLocalIns *)instr);
            break;
        case GET_GLOBAL_OP:
            handle_get_global_instr(machine, (GetGlobalIns *)instr);
            break;
        case SET_GLOBAL_OP:
            handle_set_global_instr(machine, (SetGlobalIns *)instr);
            break;
        case GOTO_OP:
            handle_goto_instr(machine, (GotoIns *)instr);
            continue;
            break;
        case BRANCH_OP:
            handle_branch_instr(machine, (BranchIns *)instr);
            continue;
            break;
        case RETURN_OP:
            handle_return_instr(machine);
            continue;
            break;
        case DROP_OP:
            handle_drop_instr(machine);
            break;
        default:
            fprintf(stderr, "Unknown instruction: %d\n", instr->tag);
            exit(1);
        }
        state->ip++;
    }
}
