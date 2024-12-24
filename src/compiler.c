#include "feeny/compiler.h"

Program *compile(ScopeStmt *stmt) {
    printf("Compiling Program:\n");
    print_scopestmt(stmt);
    printf("\n");
    exit(-1);
}
