/**
 * Tag premitive Optimization Assumption:
 * Machine->global only use original(untaged) address
 * otherwise, it will introduce redundant conversion into the whole implementation
 */
#include "feeny/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// #define EXEC_DEBUG 0
// #define MEMORY_DEBUG 0

#define MXARGS 32
Machine *machine = NULL;

static void print_classes(Vector *classes) {
    for (int i = 0; i < vector_size(classes); i++) {
        TClass *template = (TClass *)vector_get(classes, i);
        printf("Type: %ld, slotNames: ", template->type);
        for (int j = 0; j < vector_size(template->varNames); j++) {
            char *name = (char *)vector_get(template->varNames, j);
            printf("%s ", name);
        }
        printf(" slotFunction: ");
        for (int j = 0; j < vector_size(template->funcNameToPoolIndex->names); j++) {
            char *name = (char *)vector_get(template->funcNameToPoolIndex->names, j);
            printf("%s(%d) ", name, (int)(intptr_t)findByName(template->funcNameToPoolIndex, name));
        }
        printf("\n");
    }
}

static MethodValue *lookup_method(Machine *machine, ObjType type, char *method_name) {
    Vector *pool = machine->program->values;

    // Find the name(string) constant's index
    TClass *classTemplate = NULL;
    for (int i = 0; i < vector_size(machine->classes); i++) {
        TClass *curTemplate = (TClass *)vector_get(machine->classes, i);
        if (curTemplate->type == type) {
            classTemplate = curTemplate;
            break;
        }
    }
    if (!classTemplate) {
        fprintf(stderr, "Error: Unknown type in classes set!\n");
        exit(1);
    }
    void *result = findByName(classTemplate->funcNameToPoolIndex, method_name);
    if (!result) {
        return NULL;
    }

    // Fetch the methodValue in the constant pool
    Value *v = vector_get(pool, (int)(intptr_t)result);
    if (v->tag != METHOD_VAL) {
        fprintf(stderr, "Error: Given %s is not a method!\n", method_name);
        exit(1);
    }
    return (MethodValue *)v;
}

static TClass *findClassByPoolIndex(Machine *machine, int index) {
    for (int i = 0; i < vector_size(machine->classes); i++) {
        TClass *templateClass = (TClass *)vector_get(machine->classes, i);
        if (templateClass->poolIndex == index) {
            return templateClass;
        }
    }
    return NULL;
}

static int findSlotIndex(Machine *machine, ObjType type, char *name) {
    TClass *targetClass = NULL;
    if (type == GLOBAL_TYPE) {
        targetClass = (TClass *)vector_get(machine->classes, 0);
    } else {
        targetClass = (TClass *)vector_get(machine->classes, type - OBJECT_TYPE + 1);
    }
    for (int i = 0; i < vector_size(targetClass->varNames); i++) {
        if (strcmp(name, (char *)vector_get(targetClass->varNames, i)) == 0) {
            return i;
        }
    }
    return -1;
}

// Core function: init a local frame, change machine's context
static void make_frame(Machine *machine, MethodValue *method) {
    // Allocate memory for new frame
    int slots_num = method->nargs + method->nlocals;
    Frame *frame = (Frame *)malloc(sizeof(Frame) + sizeof(intptr_t) * slots_num);
    if (!frame) {
        fprintf(stderr, "Memory allocation failed for new frame\n");
        exit(1);
    }

    // Initialize frame components
    frame->parent = machine->cur; // Link to previous frame
    frame->method = method;
    frame->ra = machine->ip + 1; // Save current ip as return address
    for (int i = 0; i < slots_num; i++) {
        frame->locals[i] = 0;
    }

    // Temporarily set machine state to scan labels
    // Reset ip to start of new frame's code
    machine->cur = frame;
    machine->ip = 0;

    // Scan through method's code to process all labels
    if (method->processed == 1) {
        return;
    }
    Map *labels = newMap();
    for (int i = 0; i < vector_size(method->code); i++) {
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
            StringValue *labelName = (StringValue *)labelStr;

            // Extract the actual label string index and add to frame's label map
            // Tuple: string -> lineNumber
            addNewTuple(labels, labelName->value, (void *)(intptr_t)i);
        }
    }

    // Process all branch and goto byte instructions from name idx to true address
    for (int i = 0; i < vector_size(method->code); i++) {
        ByteIns *instr = vector_get(method->code, i);
        switch (instr->tag) {
        case BRANCH_OP: {
            BranchIns *branchInstr = (BranchIns *)instr;
            Value *branchLabelStr = vector_get(machine->program->values, branchInstr->name);
            if (branchLabelStr->tag != STRING_VAL) {
                fprintf(stderr, "Label instruction's name must be string\n");
                exit(1);
            }
            StringValue *branchLabelName = (StringValue *)branchLabelStr;
            branchInstr->name = (int)(intptr_t)findByName(labels, branchLabelName->value);
            break;
        }

        case GOTO_OP: {
            GotoIns *gotoInstr = (GotoIns *)instr;
            Value *gotoLabelStr = vector_get(machine->program->values, gotoInstr->name);
            if (gotoLabelStr->tag != STRING_VAL) {
                fprintf(stderr, "Label instruction's name must be string\n");
                exit(1);
            }
            StringValue *gotoLabelName = (StringValue *)gotoLabelStr;
            gotoInstr->name = (int)(intptr_t)findByName(labels, gotoLabelName->value);
            break;
        }

        default:
            break;
        }
    }
    freeMap(labels);
    method->processed = 1;
}

static void addSlotInfo(Vector *pool, TClass *template, Vector *slots) {
    for (int i = 0; i < vector_size(slots); i++) {
        Value *v = (Value *)(vector_get(pool, (int)(intptr_t)vector_get(slots, i)));
        switch (v->tag) {
        case SLOT_VAL: {
            SlotValue *slotValue = (SlotValue *)v;
            StringValue *slotName = (StringValue *)vector_get(pool, slotValue->name);
            if (slotName->tag != STRING_VAL) {
                fprintf(stderr, "Error: Class's slot name must be string!\n");
                exit(1);
            }
            vector_add(template->varNames, slotName->value);
            break;
        }

        case METHOD_VAL: {
            MethodValue *methodValue = (MethodValue *)v;
            StringValue *methodName = (StringValue *)vector_get(pool, methodValue->name);
            if (methodName->tag != STRING_VAL) {
                fprintf(stderr, "Error: Class's func name must be string!\n");
                exit(1);
            }
            addNewTuple(template->funcNameToPoolIndex, methodName->value, (void *)(intptr_t)vector_get(slots, i));
            break;
        }

        default:
            fprintf(stderr, "Error: Unknown class value's slot type!\n");
            exit(1);
            break;
        }
    }
}

void initvm(Program *program) {
    if (!machine) {
        machine = (Machine *)malloc(sizeof(Machine));
    }
    machine->program = program;
    machine->global = NULL;
    machine->stack = make_vector();
    machine->classes = make_vector();
    machine->cur = NULL;
    machine->ip = 0;

    // Attach all related classes info to virtual machine
    // Global can be seen as a special class and Object
    TClass *globalTemplate = newTemplateClass(GLOBAL_TYPE, -1);
    addSlotInfo(machine->program->values, globalTemplate, machine->program->slots);
    vector_add(machine->classes, globalTemplate);

    for (int i = 0; i < vector_size(program->values); i++) {
        Value *value = (Value *)vector_get(program->values, i);
        if (value->tag == CLASS_VAL) {
            ClassValue *classValue = (ClassValue *)value;
            TClass *newTemplate = newTemplateClass(OBJECT_TYPE + vector_size(machine->classes) - 1, i);
            addSlotInfo(machine->program->values, newTemplate, classValue->slots);
            vector_add(machine->classes, newTemplate);
        }
    }

    // Initialize garbage collector
    init_heap();
    machine->global = (RClass *)UNTAG_PTR((intptr_t)newClassObj(GLOBAL_TYPE, vector_size(globalTemplate->varNames)));

    // Init local frame
    MethodValue *method = vector_get(program->values, program->entry);
    make_frame(machine, method);
    machine->cur->ra = -1;
    machine->cur->parent = NULL;
}

static void handle_lit_instr(Machine *machine, LitIns *ins) {
    Value *value = vector_get(machine->program->values, ins->idx);
    switch (value->tag) {
    case INT_VAL: {
        IntValue *int_value = (IntValue *)value;
        vector_add(machine->stack, (void *)newIntObj(int_value->value));
        break;
    }

    case NULL_VAL:
        vector_add(machine->stack, (void *)newNullObj());
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
    intptr_t args[MXARGS];
    for (int i = ins->arity - 1; i >= 0; i--) {
        args[i] = (intptr_t)vector_pop(machine->stack);
        if (!IS_INT(args[i])) {
            printf("Error: printf only accepts integers\n");
            exit(1);
        }
    }
    char *str_format = ((StringValue *)format)->value;
    int idx = 0;
    while (*str_format != '\0') {
        if (*str_format == '~') {
            printf("%ld", UNTAG_INT(args[idx++]));
        } else {
            putchar(*str_format);
        }
        str_format++;
    }
}

static void handle_array_instr(Machine *machine) {
    intptr_t init_val = (intptr_t)vector_pop(machine->stack);
    intptr_t length_val = (intptr_t)vector_pop(machine->stack);

    if (!IS_INT(length_val)) {
        fprintf(stderr, "Array length must be integer\n");
        exit(1);
    }
    // !important: add inital_value to stack in case of GC can not see it
    vector_add(machine->stack, (void *)init_val);
    RArray *array = newArrayObj((int)UNTAG_INT(length_val), (void *)init_val);
    vector_pop(machine->stack);
    vector_add(machine->stack, array);
}

static void handle_object_instr(Machine *machine, ObjectIns *ins) {
    ClassValue *class = vector_get(machine->program->values, ins->class);
    if (class->tag != CLASS_VAL) {
        fprintf(stderr, "Object instruction requires class value\n");
        exit(1);
    }
    TClass *classTemplate = findClassByPoolIndex(machine, ins->class);
    if (!classTemplate) {
        fprintf(stderr, "Error: unknown object type\n");
        exit(1);
    }

    int slotNum = (int)(intptr_t)vector_size(classTemplate->varNames);
    RClass *instance = (RClass *)UNTAG_PTR((intptr_t)newClassObj(classTemplate->type, slotNum));

    // Pop initial values and parent
    for (int i = slotNum - 1; i >= 0; i--) {
        instance->var_slots[i] = (intptr_t)vector_pop(machine->stack);
    }
    instance->parent = (intptr_t)vector_pop(machine->stack);
    vector_add(machine->stack, (void *)TAG_PTR((intptr_t)instance));
}

static void handle_slot_instr(Machine *machine, SlotIns *ins) {
    intptr_t target_addr = (intptr_t)vector_pop(machine->stack);
    if (!IS_PTR(target_addr)) {
        fprintf(stderr, "Error: Slot requires object\n");
        exit(1);
    }
    RTObj *receiver = (RTObj *)UNTAG_PTR(target_addr);
    if (receiver->type < OBJECT_TYPE) {
        fprintf(stderr, "Error: Get slot requires object\n");
        exit(1);
    }

    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;
    RClass *instance = (RClass *)receiver;
    int slotIndex = findSlotIndex(machine, instance->type, slotName);
    if (slotIndex < 0) {
        fprintf(stderr, "Invalid slot access\n");
        exit(1);
    }
    vector_add(machine->stack, (void *)instance->var_slots[slotIndex]);
}

static void handle_set_slot_instr(Machine *machine, SetSlotIns *ins) {
    intptr_t value = (intptr_t)vector_pop(machine->stack);
    intptr_t target_addr = (intptr_t)vector_pop(machine->stack);
    if (!IS_PTR(target_addr)) {
        fprintf(stderr, "Error: Set slot requires object\n");
        exit(1);
    }
    RTObj *receiver = (RTObj *)UNTAG_PTR(target_addr);
    if (receiver->type < OBJECT_TYPE) {
        fprintf(stderr, "Error: Set slot requires object\n");
        exit(1);
    }

    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;
    RClass *instance = (RClass *)receiver;
    int slotIndex = findSlotIndex(machine, instance->type, slotName);
    if (slotIndex < 0) {
        fprintf(stderr, "Invalid slot access\n");
        exit(1);
    }
    instance->var_slots[slotIndex] = value;
}

static void handle_call_slot_instr(Machine *machine, CallSlotIns *ins) {
    intptr_t args[MXARGS];
    int arg_count = ins->arity - 1;
    for (int i = 0; i < arg_count; i++) {
        args[i] = (intptr_t)vector_pop(machine->stack);
    }

    intptr_t target = (intptr_t)vector_pop(machine->stack);

    char *slotName = ((StringValue *)vector_get(machine->program->values, ins->name))->value;

    // Handle integer operations
    if (IS_INT(target)) {
        // Handle integer operations
        intptr_t operand1 = target;
        if (ins->arity != 2) {
            fprintf(stderr, "Error: Int Object slot invoke must take another argument\n");
            exit(1);
        }

        if (!IS_INT(args[0])) {
            fprintf(stderr, "Error: Type check error on Int Object invoke\n");
            exit(1);
        }
        intptr_t operand2 = args[0];

        intptr_t result = 0;
        // Handle different integer operations
        if (strcmp(slotName, "add") == 0) {
            // f(x+y) = 8(x+y) = 8x + 8y = f(x) + f(y)
            result = operand1 + operand2;
        } else if (strcmp(slotName, "sub") == 0) {
            // f(x-y) = 8(x-y) = 8x - 8y = f(x) - f(y)
            result = operand1 - operand2;
        } else if (strcmp(slotName, "mul") == 0) {
            // f(x*y) = 8(x*y) = 8x * y = f(x) * y
            result = operand1 * UNTAG_INT(operand2);
        } else if (strcmp(slotName, "div") == 0) {
            // f(x/y) = 8(x/y) = 8x / y = f(x) / y
            result = TAG_INT(UNTAG_INT(operand1) / UNTAG_INT(operand2));
        } else if (strcmp(slotName, "mod") == 0) {
            // f(x%y) = f(x % y)
            result = TAG_INT(UNTAG_INT(operand1) % UNTAG_INT(operand2));
        } else if (strcmp(slotName, "lt") == 0) {
            // result = UNTAG_INT(operand1) < UNTAG_INT(operand2) ? newIntObj(0) : newNullObj();
            result = ((operand1 < operand2 ? 1 : 0) ^ 1) << 1;
        } else if (strcmp(slotName, "gt") == 0) {
            result = ((operand1 > operand2 ? 1 : 0) ^ 1) << 1;
        } else if (strcmp(slotName, "eq") == 0) {
            result = ((operand1 == operand2 ? 1 : 0) ^ 1) << 1;
        } else if (strcmp(slotName, "le") == 0) {
            result = ((operand1 <= operand2 ? 1 : 0) ^ 1) << 1;
        } else if (strcmp(slotName, "ge") == 0) {
            result = ((operand1 >= operand2 ? 1 : 0) ^ 1) << 1;
        } else {
            fprintf(stderr, "Error: Unsupported Int Object operation: %s\n", slotName);
            exit(1);
        }

        vector_add(machine->stack, (void *)result);
        machine->ip++;
        return;
    }

    // Handle array operations
    if (!IS_PTR(target)) {
        fprintf(stderr, "Error: Array Object slot invoke must take another argument\n");
        exit(1);
    }
    RTObj *receiver = (RTObj *)UNTAG_PTR(target);
    if (receiver->type == ARRAY_TYPE) {
        RArray *arr = (RArray *)receiver;

        if (strcmp(slotName, "set") == 0) {
            if (ins->arity != 3) {
                fprintf(stderr, "Error: Array Object set operation must take two arguments\n");
                exit(1);
            }

            intptr_t index = args[1];
            if (!IS_INT(index)) {
                fprintf(stderr, "Error: Array index must be integer\n");
                exit(1);
            }

            int arrayIndex = UNTAG_INT(index);
            arr->slots[arrayIndex] = args[0];

        } else if (strcmp(slotName, "get") == 0) {
            if (ins->arity != 2) {
                fprintf(stderr, "Error: Array Object get operation must take one argument\n");
                exit(1);
            }

            intptr_t index = args[0];
            if (!IS_INT(index)) {
                fprintf(stderr, "Error: Array index must be integer\n");
                exit(1);
            }

            int arrayIndex = UNTAG_INT(index);
            vector_add(machine->stack, (void *)arr->slots[arrayIndex]);
        } else if (strcmp(slotName, "length") == 0) {
            if (ins->arity != 1) {
                fprintf(stderr, "Error: Array Object length operation takes no arguments\n");
                exit(1);
            }

            vector_add(machine->stack, (void *)newIntObj(arr->length));
        } else {
            fprintf(stderr, "Error: Unsupported Array Object operation: %s\n", slotName);
            exit(1);
        }
        machine->ip++;
        return;
    }

    // Handle object operations
    if (receiver->type >= OBJECT_TYPE) {
        RTObj *current = receiver;
        MethodValue *method = NULL;

        while (current) {
            method = lookup_method(machine, current->type, slotName);
            if (method) {
                break;
            }
            intptr_t parent = ((RClass *)current)->parent;
            if (IS_NULL(parent)) {
                break;
            }
            if (!IS_PTR(parent)) {
                fprintf(stderr, "Error: Invalid parent pointer\n");
                exit(1);
            }
            current = (RTObj *)UNTAG_PTR(parent);
        }
        if (!method || method->tag != METHOD_VAL) {
            fprintf(stderr, "Error: Undefined function: %s\n", slotName);
            exit(1);
        }

        if (ins->arity != method->nargs) {
            fprintf(stderr, "Error: Wrong number of arguments for function %s\n", slotName);
            exit(1);
        }

        // Create new frame with receiver as parent
        make_frame(machine, method);
        machine->cur->locals[0] = target;

        for (int i = 0; i < ins->arity - 1; i++) {
            machine->cur->locals[ins->arity - i - 1] = args[i];
        }
    } else {
        fprintf(stderr, "Error: Cannot invoke any operation on Null Object\n");
        exit(1);
    }
}

static void handle_call_instr(Machine *machine, CallIns *ins) {
    StringValue *funcName = vector_get(machine->program->values, ins->name);
    if (funcName->tag != STRING_VAL) {
        fprintf(stderr, "Error: Function name must be string\n");
        exit(1);
    }

    MethodValue *method = lookup_method(machine, GLOBAL_TYPE, funcName->value);
    if (!method || method->tag != METHOD_VAL) {
        fprintf(stderr, "Error: Undefined function: %s\n", funcName->value);
        exit(1);
    }
    if (ins->arity != method->nargs) {
        fprintf(stderr, "Wrong number of arguments for function %s\n", funcName->value);
        exit(1);
    }

    make_frame(machine, method);
    for (int i = 0; i < ins->arity; i++) {
        machine->cur->locals[ins->arity - i - 1] = (intptr_t)vector_pop(machine->stack);
    }
}
static void handle_get_local_instr(Machine *machine, GetLocalIns *ins) {
    vector_add(machine->stack, (void *)machine->cur->locals[ins->idx]);
}

static void handle_set_local_instr(Machine *machine, SetLocalIns *ins) {
    machine->cur->locals[ins->idx] = (intptr_t)vector_pop(machine->stack);
}

static void handle_get_global_instr(Machine *machine, GetGlobalIns *ins) {
    StringValue *global = (StringValue *)vector_get(machine->program->values, ins->name);
    if (global->tag != STRING_VAL) {
        fprintf(stderr, "Error: Global variable should be string value!\n");
        exit(1);
    }

    int slotIndex = findSlotIndex(machine, GLOBAL_TYPE, global->value);
    vector_add(machine->stack, (void *)machine->global->var_slots[slotIndex]);
}

static void handle_set_global_instr(Machine *machine, SetGlobalIns *ins) {
    StringValue *global = (StringValue *)vector_get(machine->program->values, ins->name);
    if (global->tag != STRING_VAL) {
        fprintf(stderr, "Error: Global variable should be string value!\n");
        exit(1);
    }
    int slotIndex = findSlotIndex(machine, GLOBAL_TYPE, global->value);
    machine->global->var_slots[slotIndex] = (intptr_t)vector_pop(machine->stack);
}

static void handle_goto_instr(Machine *machine, GotoIns *ins) {
    machine->ip = (int)(intptr_t)ins->name;
}

static void handle_branch_instr(Machine *machine, BranchIns *ins) {
    intptr_t condition = (intptr_t)vector_pop(machine->stack);

    if (!IS_NULL(condition)) {
        machine->ip = (int)(intptr_t)ins->name;
    } else {
        machine->ip++;
    }
}

static void handle_return_instr(Machine *machine) {
    Frame *cur = machine->cur;
    machine->cur = cur->parent;
    machine->ip = cur->ra;
    free(cur);
}

static void handle_drop_instr(Machine *machine) {
    vector_pop(machine->stack);
}

void runvm() {
    Program *program = machine->program;

    while (1) {

#ifdef EXEC_DEBUG
        printf("cur ip: %ld\n", machine->ip);
#endif
        // When encounter the end of current frame, break the loop
        if (machine->ip == -1 && machine->cur == NULL) {
            break;
        }
        if (machine->ip >= vector_size(machine->cur->method->code)) {
            fprintf(stderr, "Error: execute bytecodes out of bound!\n");
            exit(1);
        }

        ByteIns *instr = (ByteIns *)vector_get(machine->cur->method->code, machine->ip);

#ifdef EXEC_DEBUG
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
        machine->ip++;
    }
#ifdef MEMORY_DEBUG
    print_detailed_memory();
    print_heap_objects();
#endif
}
