#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "types.h"
#include "utils.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

extern size_t heap_size;
extern intptr_t heap_start;
extern intptr_t heap_ptr;
extern intptr_t to_space;
extern intptr_t to_ptr;
extern size_t total_bytes;

void init_heap();
void *halloc(int);
int garbage_collector();
void print_detailed_memory();
void print_heap_objects();

// Forwarding pointers related operations
int is_forward(intptr_t);
void set_forward_address(intptr_t, intptr_t);
intptr_t get_forward_address(intptr_t);

#endif // COLLECTOR_H