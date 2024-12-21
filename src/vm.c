#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include "feeny/utils.h"
#include "feeny/bytecode.h"
#include "feeny/vm.h"

void interpret_bc (Program* p) {
  printf("Interpreting Bytecode Program:\n");
  print_prog(p);
  printf("\n");
}

