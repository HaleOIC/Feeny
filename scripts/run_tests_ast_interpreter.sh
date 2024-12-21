# Compile
gcc -std=c99 -O3 -I../include ../src/cfeeny.c ../src/utils.c ../src/ast.c ../src/interpreter.c -o ../bin/cfeeny -Wno-int-to-void-pointer-cast

# Clean output folder
rm ../output/*.out
rm ../output/*.ast

# Run output
function test {
   ../bin/parser -i ../examples/$1.feeny -oast ../output/$1.ast
   ../bin/cfeeny ../output/$1.ast > ../output/$1.out
}
test hello
test hello2
test hello3
test hello4
test hello5
test hello6
test hello7
test hello8
test hello9
test cplx
test bsearch
test fibonacci
test inheritance
test lists
test vector
test sudoku
test sudoku2
