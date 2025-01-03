#include "feeny/collector.h"
#include <sys/mman.h>
#include <sys/resource.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

// #define MEMORY_DEBUG 1

// heap_size = 1MB
size_t heap_size = 1024 * 1024;

// Core variables for garbage collector
intptr_t heap_start = 0;
intptr_t heap_ptr = 0;
intptr_t to_space = 0;
intptr_t to_ptr = 0;
size_t total_bytes = 0;

// Helper function: check if an address is forward pointer
int is_forward(intptr_t address) {
    return ((RTObj *)address)->type == BROKEN_HEART;
}

// Helper function: set forward address for an object
void set_forward_address(intptr_t obj, intptr_t newAddress) {
    ((RTObj *)obj)->type = BROKEN_HEART;
    *((intptr_t *)obj + 1) = (intptr_t)newAddress;
}

// Helper function: get forward address of an object
intptr_t get_forward_address(intptr_t obj) {
    if (!is_forward(obj)) {
        fprintf(stderr, "Error: Attempting to get forward address of non-forwarded object\n");
        exit(1);
    }
    return *((intptr_t *)obj + 1);
}

// Check if pointer is within heap
static int is_heap_ptr(intptr_t ptr) {
    return ptr >= heap_start && ptr < heap_start + heap_size;
}

TClass *find_class_by_type(ObjType type) {
    for (int i = 0; i < vector_size(machine->classes); i++) {
        TClass *template = (TClass *)vector_get(machine->classes, i);
        if (template->type == type) {
            return template;
        }
    }
    return NULL;
}

// Helper function: get the size of an object
static size_t get_object_size(intptr_t obj) {
    switch (((RTObj *)obj)->type) {
    case INT_TYPE:
        return 0;
    case NULL_TYPE:
        return 0;
    case ARRAY_TYPE:
        return sizeof(RArray) + ((RArray *)obj)->length * sizeof(intptr_t);
    case BROKEN_HEART:
        fprintf(stderr, "Error: Broken heart object can not be calculated\n");
        exit(1);
    default: {
        // General case, lookup method's class template and calculate size
        TClass *template = find_class_by_type(((RTObj *)obj)->type);
        if (!template) {
            fprintf(stderr, "Found type: %ld\n", ((RTObj *)obj)->type);
            fprintf(stderr, "Error: Class template not found\n");
            exit(1);
        }
        return sizeof(RClass) + vector_size(template->varNames) * sizeof(intptr_t);
    }
    }
}

// Copy object to to-space
static intptr_t copy_object(intptr_t obj) {
    if (!obj || !is_heap_ptr(obj))
        return obj;

    if (IS_INT(obj) || IS_NULL(obj)) {
        return obj;
    }

    obj = UNTAG_PTR(obj);

    // Check if already forwarded
    if (is_forward(obj)) {
        return get_forward_address(obj);
    }

    // Copy object to to-space
    size_t size = get_object_size(obj);
    intptr_t new_location = to_ptr;

    memcpy((void *)to_ptr, (void *)obj, size);
    to_ptr += size;

    // Set forwarding address
    set_forward_address(obj, new_location);

    return TAG_PTR(new_location);
}

// Scan an object's pointers
static void scan_object(intptr_t obj) {
    if (!obj)
        return;

    RTObj *robj = (RTObj *)obj;
    switch (robj->type) {
    case INT_TYPE:
    case NULL_TYPE:
        return; // No pointers

    case ARRAY_TYPE: {
        RArray *arr = (RArray *)obj;
        for (size_t i = 0; i < arr->length; i++) {
            arr->slots[i] = copy_object(arr->slots[i]);
        }
        break;
    }

    default: {
        // Must be a class instance
        RClass *cls = (RClass *)obj;
        // Update parent pointer
        cls->parent = copy_object(cls->parent);

        // Find class template
        TClass *template = find_class_by_type(cls->type);

        if (!template) {
            print_detailed_memory();
            fprintf(stderr, "Found type: %ld\n", cls->type);
            fprintf(stderr, "Error: Class template not found\n");
            exit(1);
        }

        // Update all var slots
        for (int i = 0; i < vector_size(template->varNames); i++) {
            cls->var_slots[i] = copy_object(cls->var_slots[i]);
        }
    }
    }
}

// Scan root set (globals, frames, operand stack)
static void scan_root_set() {
    // Scan globals
    TClass *globalTemplate = find_class_by_type(GLOBAL_TYPE);
    if (machine->global) {
        for (int i = 0; i < vector_size(globalTemplate->varNames); i++) {
            machine->global->var_slots[i] = copy_object(machine->global->var_slots[i]);
        }
        machine->global = (RClass *)UNTAG_PTR((intptr_t)copy_object(TAG_PTR((intptr_t)machine->global)));
    }

    // Scan frames
    Frame *frame = machine->cur;
    while (frame) {
        // Scan local variables
        for (int i = 0; i < frame->method->nargs + frame->method->nlocals; i++) {
            frame->locals[i] = copy_object(frame->locals[i]);
        }
        frame = frame->parent;
    }

    // Scan operand stack
    for (int i = 0; i < vector_size(machine->stack); i++) {
        intptr_t obj = (intptr_t)vector_get(machine->stack, i);
        vector_set(machine->stack, i, (void *)copy_object(obj));
    }
}

void init_heap() {
    // Allocate 1GB heap space for from-space and to-space
    heap_start = (intptr_t)mmap(NULL, heap_size,
                                PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap_start == -1) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    }
    heap_ptr = heap_start;

    to_space = (intptr_t)mmap(NULL, heap_size,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (to_space == -1) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    }
    to_ptr = to_space;
    total_bytes = 0;
}
int expand_heap() {
#ifdef MEMORY_DEBUG
    printf("\n=== Expanding Heap ===\n");
    printf("Current heap_size: %zu, new_size: %zu\n", heap_size, heap_size << 1);
    printf("Before expansion:\n");
    printf("  heap_start: %p\n", (void *)heap_start);
    printf("  heap_ptr: %p (used: %zu bytes)\n", (void *)heap_ptr, heap_ptr - heap_start);
    printf("  to_space: %p\n", (void *)to_space);
#endif
    size_t new_size = heap_size << 1;

    intptr_t new_heap = (intptr_t)mmap(NULL, new_size,
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_heap == -1) {
        printf("Failed to allocate new heap space!\n");
        return 0;
    }

    intptr_t old_to_space = to_space;
    to_space = new_heap;
#ifdef MEMORY_DEBUG
    printf("Allocated new heap space at: %p\n", (void *)new_heap);
    printf("Set new to_space: %p (old was: %p)\n", (void *)to_space, (void *)old_to_space);

    printf("\nStarting garbage collection...\n");
#endif
    garbage_collector();

#ifdef MEMORY_DEBUG
    printf("Garbage collection completed\n");
    printf("After GC:\n");
    printf("  heap_start: %p\n", (void *)heap_start);
    printf("  heap_ptr: %p (used: %zu bytes)\n", (void *)heap_ptr, heap_ptr - heap_start);
    printf("  to_space: %p\n", (void *)to_space);

    printf("\nReleasing old spaces...\n");
    printf("  Releasing to_space: %p (size: %zu)\n", (void *)to_space, heap_size);
    printf("  Releasing old_to_space: %p (size: %zu)\n", (void *)old_to_space, heap_size);
#endif
    munmap((void *)to_space, heap_size);
    munmap((void *)old_to_space, heap_size);

    intptr_t new_to_space = (intptr_t)mmap(NULL, new_size,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_to_space == -1) {
        printf("Failed to allocate new to_space!\n");
        munmap((void *)new_heap, new_size);
        return 0;
    }

    to_space = new_to_space;
    heap_size = new_size;

#ifdef MEMORY_DEBUG
    printf("Allocated new to_space at: %p\n", (void *)new_to_space);
    printf("\nFinal heap state:\n");
    printf("  heap_start: %p\n", (void *)heap_start);
    printf("  heap_ptr: %p (used: %zu bytes)\n", (void *)heap_ptr, heap_ptr - heap_start);
    printf("  to_space: %p\n", (void *)to_space);
    printf("Heap expanded from %zu to %zu bytes\n", heap_size / 2, heap_size);
    printf("=== Heap Expansion Complete ===\n\n");
#endif
    return 1;
}

int garbage_collector() {
    to_ptr = to_space;

    memset((void *)to_space, 0, heap_ptr - heap_start);

    scan_root_set();

    intptr_t scan = to_space;
    while (scan < to_ptr) {
        scan_object(scan);
        scan += get_object_size(scan);
    }

    intptr_t temp = heap_start;
    heap_start = to_space;
    to_space = temp;
    heap_ptr = to_ptr;
    return 1;
}
void *halloc(int nbytes) {
    // Align to 8 bytes
    nbytes = (nbytes + 7) & ~7;

    // Calculate current heap usage percentage
    double usage_percentage = ((double)(heap_ptr - heap_start) / heap_size) * 100;

    // If we don't have enough space for this allocation or usage is over 85%, try GC first
    if (heap_ptr + nbytes > heap_start + heap_size || usage_percentage > 90.0) {
        // printf("Before GC: usage is %.2f%%\n", usage_percentage);

        // Try garbage collection
        garbage_collector();

        // Recalculate usage after GC
        usage_percentage = ((double)(heap_ptr - heap_start) / heap_size) * 100;
        // printf("After GC: usage is %.2f%%\n", usage_percentage);

        // If after GC still over 70% or not enough space, expand heap
        if (usage_percentage > 70.0 || heap_ptr + nbytes > heap_start + heap_size) {
            // printf("Usage still high (%.2f%%) after GC, expanding heap\n", usage_percentage);
            if (!expand_heap()) {
                print_heap_objects();
                fprintf(stderr, "Fatal: Memory exhausted. Cannot expand heap further.\n");
                fprintf(stderr, "Current heap size: %zu bytes\n", heap_size);
                fprintf(stderr, "Requested allocation: %d bytes\n", nbytes);
                fprintf(stderr, "Available space: %ld bytes\n",
                        (heap_start + heap_size) - heap_ptr);
                exit(1);
            }
        }
    }

    total_bytes += nbytes;
    void *result = (void *)heap_ptr;
    heap_ptr += nbytes;
    return result;
}

void print_detailed_memory() {
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);

    FILE *status = fopen("/proc/self/status", "r");
    char line[256];

    printf("=== Memory Usage Details ===\n");
    printf("Maximum resident set size: %ld KB\n", r_usage.ru_maxrss);

    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0 ||
            strncmp(line, "VmStk:", 6) == 0 ||
            strncmp(line, "VmPeak:", 7) == 0) {
            printf("%s", line);
        }
    }
    fclose(status);

    printf("Total bytes allocated: %zu\n", total_bytes);
    printf("========================\n");
}

void print_heap_objects() {
    printf("\n=== Heap Objects (from-space) ===\n");
    printf("heap_start: %p\n", (void *)heap_start);
    printf("heap_ptr: %p\n", (void *)heap_ptr);

    intptr_t current = heap_start;
    int obj_count = 0;
    printf("\nTotal objects: %d\n", obj_count);
    printf("Total heap usage: %ld bytes\n", heap_ptr - heap_start);
    printf("=== End of Heap Objects ===\n\n");

    intptr_t total_stack_size = 0;
    intptr_t total_null = 0;
    for (int i = 0; i < vector_size(machine->stack); i++) {
        intptr_t obj = (intptr_t)vector_get(machine->stack, i);
        if (IS_NULL(obj)) {
            total_null++;
            continue;
        }
        if (IS_INT(obj)) {
            continue;
        }
        total_stack_size += get_object_size(obj);
    }
    printf("Total stack usage: %ld bytes\n", total_stack_size);
    printf("Total null objects: %ld\n", total_null);
}
