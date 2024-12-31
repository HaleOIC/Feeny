#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "types.h"
#include "utils.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#define HEAP_SIZE (1024 * 1024) // 1MB heap size

extern intptr_t heap_start;
extern intptr_t heap_ptr;
extern intptr_t to_space;
extern intptr_t to_ptr;
extern size_t total_bytes;

void init_heap();
void *halloc(int);
int garbage_collector();
void print_detailed_memory();

// Forwarding pointers related operations
static int is_forward(intptr_t);
static void set_forward_address(intptr_t, intptr_t);
static intptr_t get_forward_address(intptr_t);

#endif // COLLECTOR_H