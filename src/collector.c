#include "feeny/collector.h"
#include <sys/resource.h>

// Core variables for garbage collector
intptr_t heap_start = 0;
intptr_t heap_ptr = 0;
intptr_t to_space = 0;
intptr_t to_ptr = 0;
size_t total_bytes = 0;

// Helper function: check if an address is forward pointer
static int is_forward(intptr_t address) {
    return ((RTObj *)address)->type == BROKEN_HEART;
}

// Helper function: set forward address for an object
static void set_forward_address(intptr_t obj, intptr_t newAddress) {
    ((RTObj *)obj)->type = BROKEN_HEART;
    *((intptr_t *)obj + 1) = (intptr_t)newAddress;
}

// Helper function: get forward address of an object
static intptr_t get_forward_address(intptr_t obj) {
    if (!is_forward(obj)) {
        fprintf(stderr, "Error: Attempting to get forward address of non-forwarded object\n");
        exit(1);
    }
    return *((intptr_t *)obj + 1);
}

// Check if pointer is within heap
static int is_heap_ptr(intptr_t ptr) {
    return ptr >= heap_start && ptr < heap_start + HEAP_SIZE;
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
        return sizeof(RInt);
    case NULL_TYPE:
        return sizeof(RNull);
    case ARRAY_TYPE:
        return sizeof(RArray) + ((RArray *)obj)->length * sizeof(intptr_t);
    case BROKEN_HEART:
        fprintf(stderr, "Error: Broken heart object can not be calculated\n");
        exit(1);
    default:
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

void print_heap_objects() {
    printf("\n=== Heap Objects (from-space) ===\n");
    printf("heap_start: %p\n", (void *)heap_start);
    printf("heap_ptr: %p\n", (void *)heap_ptr);

    intptr_t current = heap_start;
    int obj_count = 0;

    while (current < heap_ptr) {
        RTObj *obj = (RTObj *)current;
        size_t size = get_object_size(current);

        printf("\nObject #%d at %p (size: %zu bytes):\n", ++obj_count, (void *)current, size);
        printf("  Type: ");

        switch (obj->type) {
        case INT_TYPE:
            printf("INT_TYPE (%ld)\n", ((RInt *)obj)->value);
            break;

        case NULL_TYPE:
            printf("NULL_TYPE\n");
            break;

        case ARRAY_TYPE: {
            RArray *arr = (RArray *)obj;
            printf("ARRAY_TYPE (length: %zu)\n", arr->length);
            printf("  Elements:\n");
            for (size_t i = 0; i < arr->length; i++) {
                printf("    [%zu]: %p\n", i, (void *)arr->slots[i]);
            }
            break;
        }

        case BROKEN_HEART:
            printf("BROKEN_HEART (forward to: %p)\n",
                   (void *)get_forward_address(current));
            break;

        default: {
            TClass *template = find_class_by_type(obj->type);
            if (template) {
                printf("CLASS_TYPE (%ld)\n", obj->type);
                RClass *cls = (RClass *)obj;
                printf("  Parent: %p\n", (void *)cls->parent);
                printf("  Variables:\n");
                for (int i = 0; i < vector_size(template->varNames); i++) {
                    char *varName = (char *)vector_get(template->varNames, i);
                    printf("    %s: %p\n", varName, (void *)cls->var_slots[i]);
                }
            } else {
                printf("UNKNOWN_TYPE (%ld)\n", obj->type);
            }
            break;
        }
        }

        current += size;
    }

    printf("\nTotal objects: %d\n", obj_count);
    printf("Total heap usage: %ld bytes\n", heap_ptr - heap_start);
    printf("=== End of Heap Objects ===\n\n");
}

// Copy object to to-space
static intptr_t copy_object(intptr_t obj) {
    if (!obj || !is_heap_ptr(obj))
        return obj;

    // Check if already forwarded
    if (is_forward(obj)) {
        return get_forward_address(obj);
    }

    // Copy object to to-space
    size_t size = get_object_size(obj);
    intptr_t new_location = to_ptr;

    memcpy((void *)to_ptr, (void *)obj, size);
    to_ptr += size;
    // printf("Copied object from %p to %p using %ld bytes\n", (void *)obj, (void *)new_location, size);

    // Set forwarding address
    set_forward_address(obj, new_location);

    return new_location;
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
            // printf("Scanning global var slot %p\n", (void *)machine->global->var_slots[i]);
            machine->global->var_slots[i] = copy_object(machine->global->var_slots[i]);
        }
        machine->global = (RClass *)copy_object((intptr_t)machine->global);
    }

    // Scan frames
    Frame *frame = machine->cur;
    while (frame) {
        // Scan local variables
        for (int i = 0; i < frame->method->nargs + frame->method->nlocals; i++) {
            // printf("Scanning frame local %p\n", (void *)frame->locals[i]);
            frame->locals[i] = copy_object(frame->locals[i]);
        }
        frame = frame->parent;
    }

    // Scan operand stack
    for (int i = 0; i < vector_size(machine->stack); i++) {
        intptr_t obj = (intptr_t)vector_get(machine->stack, i);
        // printf("Scanning stack object %p\n", (void *)obj);
        vector_set(machine->stack, i, (void *)copy_object(obj));
    }
}

void init_heap() {
    // Allocate 1GB heap space for from-space and to-space
    heap_start = (intptr_t)mmap(NULL, HEAP_SIZE,
                                PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap_start == -1) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    }
    heap_ptr = heap_start;

    to_space = (intptr_t)mmap(NULL, HEAP_SIZE,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (to_space == -1) {
        fprintf(stderr, "Error: mmap failed\n");
        exit(1);
    }
    to_ptr = to_space;
    total_bytes = 0;
}

int garbage_collector() {
    // Reset to-space pointer
    to_ptr = to_space;

    // Copy root set
    scan_root_set();

    // Scan all copied objects
    intptr_t scan = to_space;
    while (scan < to_ptr) {
        scan_object(scan);
        scan += get_object_size(scan);
    }

    // Swap spaces
    intptr_t temp = heap_start;
    heap_start = to_space;
    to_space = temp;
    heap_ptr = to_ptr;

    return 1; // Collection successful
}

// static size_t alloc_count = 0;

// void *halloc(int nbytes) {
//     alloc_count++;

//     // Align to 8 bytes
//     int original_size = nbytes;
//     nbytes = (nbytes + 7) & ~7;

//     printf("=== Allocation #%zu ===\n", alloc_count);
//     printf("Requested size: %d bytes\n", original_size);
//     printf("Aligned size: %d bytes\n", nbytes);
//     printf("Before GC - heap_ptr: %p\n", (void *)heap_ptr);

//     garbage_collector();

//     total_bytes += nbytes;
//     void *result = (void *)heap_ptr;
//     heap_ptr += nbytes;

//     printf("Allocated at: %p\n", result);
//     printf("New heap_ptr: %p\n", (void *)heap_ptr);
//     printf("Remaining heap space: %ld bytes\n", (heap_start + HEAP_SIZE) - heap_ptr);
//     printf("==================\n\n");

//     return result;
// }
void *halloc(int nbytes) {
    // Align to 8 bytes
    nbytes = (nbytes + 7) & ~7;

    // Check if enough space in from-space
    garbage_collector();
    // if (heap_ptr + nbytes > heap_start + HEAP_SIZE) {
    //     // If not enough space, run garbage collector
    //     if (!garbage_collector()) {
    //         fprintf(stderr, "Error: Out of memory\n");
    //         exit(1);
    //     }
    //     if (heap_ptr + nbytes > heap_start + HEAP_SIZE) {
    //         fprintf(stderr, "Error: Out of memory after GC\n");
    //         exit(1);
    //     }
    // }

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