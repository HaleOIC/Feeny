#include "feeny/ast.h"
#include "feeny/bytecode.h"
#include "feeny/compiler.h"
#include "feeny/interpreter.h"
#include "feeny/utils.h"
#include "feeny/vm.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage(const char *program_name) {
    printf("Usage: %s [options] <filename>\n", program_name);
    printf("Options:\n");
    printf("  -a, --ast             Run AST interpreter (default)\n");
    printf("  -b, --bytecode        Run bytecode interpreter\n");
    printf("  -f, --fullcompile     Run bytecode compiler and interpreter\n");
    printf("  -h, --help            Show this help message\n");
    exit(1);
}

typedef enum {
    MODE_AST,
    MODE_BYTECODE,
    MODE_FULL
} RunMode;

int main(int argc, char **argv) {
    RunMode mode = MODE_AST;
    int verbose = 0;

    static struct option long_options[] = {
        {"ast", no_argument, 0, 'a'},
        {"bytecode", no_argument, 0, 'b'},
        {"full", no_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int option;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "abvhf",
                                 long_options, &option_index)) != -1) {
        switch (option) {
        case 'a':
            mode = MODE_AST;
            break;
        case 'b':
            mode = MODE_BYTECODE;
            break;
        case 'f':
            mode = MODE_FULL;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            break;
        case '?':
            // getopt_long already printed an error message
            print_usage(argv[0]);
            break;
        default:
            abort();
        }
    }

    // Check if a filename was provided
    if (optind >= argc) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
    }

    char *filename = argv[optind];
    if (verbose) {
        printf("Mode: %s\n", mode == MODE_AST ? "AST" : "Bytecode");
        printf("Input file: %s\n", filename);
    }

    switch (mode) {
    case MODE_AST: {
        if (verbose)
            printf("Reading AST from file...\n");
        ScopeStmt *stmt = read_ast(filename);
        if (stmt == NULL) {
            fprintf(stderr, "Error: Failed to read AST from file: %s\n", filename);
            return 1;
        }
        if (verbose)
            printf("Interpreting AST...\n");
        interpret(stmt);
        break;
    }

    case MODE_BYTECODE: {
        if (verbose)
            printf("Loading bytecode from file...\n");
        Program *p = load_bytecode(filename);
        if (p == NULL) {
            fprintf(stderr, "Error: Failed to load bytecode from file: %s\n", filename);
            return 1;
        }

        if (verbose)
            printf("Initializing VM...\n");

        // Allocate new machine and link the program to it
        Machine *m = (Machine *)malloc(sizeof(Machine));
        initvm(p, m);

        if (verbose)
            printf("Running VM...\n");

        // Actually run the vm to get output
        runvm(m);

        free(m);
        break;
    }
    case MODE_FULL: {
        if (verbose) {
            printf("Loading ast from file...\n");
        }
        ScopeStmt *stmt = read_ast(filename);
        if (stmt == NULL) {
            fprintf(stderr, "Error: Failed to read AST from file: %s\n", filename);
            return 1;
        }
        Program *program = compile(stmt);
        if (verbose)
            printf("Initializing VM...\n");

        // Allocate new machine and link the program to it
        Machine *machine = (Machine *)malloc(sizeof(Machine));
        initvm(program, machine);

        if (verbose)
            printf("Running VM...\n");

        // Actually run the vm to get output
        runvm(machine);

        free(machine);
        break;
    }
    }

    return 0;
}
