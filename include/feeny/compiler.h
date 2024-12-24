#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "bytecode.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Program *compile(ScopeStmt *stmt);

#endif
