#ifndef VM_H
#define VM_H
#include "bytecode.h"
#include "interpreter.h"
#include "utils.h"

typedef struct Frame Frame;
struct Frame {
    Map *labels;
    Vector *locals;
    Vector *codes;
    Frame *prev;
    int ra; // Return address
};
static Frame *newFrame(MethodValue *);

typedef struct {
    Map *global_variables;
    Frame *cur; // Current code frame
    Vector *stack;
    int ip; // Instruction pointer
} State;

static State *initState(Program *);

typedef struct {
    Program *program;
    State *state;
} Machine;

/* Link source program to vm */
void initvm(Program *, Machine *);
/* Run vm to execute source program */
void runvm(Machine *);
/* Handle all kinds of instructions */
static void handle_lit_instr(Machine *, LitIns *);
static void handle_label_instr(Machine *, LabelIns *);
static void handle_print_instr(Machine *, PrintfIns *);
static void handle_array_instr(Machine *);
static void handle_object_instr(Machine *, ObjectIns *);
static void handle_slot_instr(Machine *, SlotIns *);
static void handle_set_slot_instr(Machine *, SetSlotIns *);
static void handle_call_slot_instr(Machine *, CallSlotIns *);
static void handle_call_instr(Machine *, CallIns *);
static void handle_get_local_instr(Machine *, GetLocalIns *);
static void handle_set_local_instr(Machine *, SetLocalIns *);
static void handle_get_global_instr(Machine *, GetGlobalIns *);
static void handle_set_global_instr(Machine *, SetGlobalIns *);
static void handle_goto_instr(Machine *, GotoIns *);
static void handle_branch_instr(Machine *, BranchIns *);
static void handle_return_instr(Machine *);
static void handle_drop_instr(Machine *);

#endif
