#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "utils.h"
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
void print_detailed_memory();

#endif // COLLECTOR_H