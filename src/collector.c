#include "feeny/collector.h"

intptr_t heap_start = 0;
intptr_t heap_ptr = 0;
intptr_t to_space = 0;
intptr_t to_ptr = 0;
size_t total_bytes = 0;

#include <sys/resource.h>

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

void *halloc(int nbytes) {
    // align to 8 bytes
    nbytes = (nbytes + 7) & ~7;
    static int alloc_count = 0;
    total_bytes += nbytes;

    return malloc(nbytes);

    // if (heap_ptr + nbytes > heap_start + HEAP_SIZE) {
    //     if (!garbage_collector()) {
    //         fprintf(stderr, "Out of memory\n");
    //         exit(1);
    //     }
    //     if (heap_ptr + nbytes > heap_start + HEAP_SIZE) {
    //         fprintf(stderr, "Out of memory after GC\n");
    //         exit(1);
    //     }
    // }

    // void *result = heap_ptr;
    // heap_ptr += nbytes;
    // return result;
}