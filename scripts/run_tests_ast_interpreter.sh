# Compile
gcc -std=c99 -O3 -I../include ../src/*.c -o ../bin/cfeeny -Wno-int-to-void-pointer-cast

# Clean output folder
rm ../output/astInterpreter/*.out
rm ../output/astInterpreter/*.ast

# Run output
function test {
    ../bin/parser -i ../examples/$1.feeny -oast ../output/astInterpreter/$1.ast
    ../bin/cfeeny -a ../output/astInterpreter/$1.ast > ../output/astInterpreter/$1.out
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
test hanoi
test stack
test morehanoi