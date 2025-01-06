#include "feeny/ast.h"
#include "feeny/bytecode.h"
#include "feeny/compiler.h"
#include "feeny/interpreter.h"
#include "feeny/parser.h"
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
    printf("  -f, --fullBytecode    Run bytecode compiler and interpreter\n");
    printf("  -h, --help            Show this help message\n");
    exit(1);
}

typedef enum {
    MODE_AST,
    MODE_FULL
} RunMode;

int main(int argc, char **argv) {
    RunMode mode = MODE_AST;
    int verbose = 0;

    static struct option long_options[] = {
        {"ast", no_argument, 0, 'a'},
        {"full", no_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int option;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "avhf",
                                 long_options, &option_index)) != -1) {
        switch (option) {
        case 'a':
            mode = MODE_AST;
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

    Parser *parser = init_parser(filename);
    if (parser == NULL) {
        fprintf(stderr, "Error: Failed to initialize parser\n");
        return 1;
    }
    ScopeStmt *stmt = parse(parser);
    if (stmt == NULL) {
        fprintf(stderr, "Error: Failed to parse input file\n");
        return 1;
    }
    // printf("\n\n\n");

    // print_scopestmt(stmt);

    switch (mode) {
    case MODE_AST: {
        if (verbose)
            printf("Interpreting AST...\n");
        interpret(stmt);
        break;
    }

    case MODE_FULL: {
        Program *program = compile(stmt);
        if (verbose)
            printf("Initializing VM...\n");

        // print_prog(program);

        // Allocate new machine and link the program to it
        Machine *machine = (Machine *)malloc(sizeof(Machine));
        initvm(program);

        if (verbose)
            printf("Running VM...\n");

        // Actually run the vm to get output
        runvm();

        break;
    }
    }

    return 0;
}
