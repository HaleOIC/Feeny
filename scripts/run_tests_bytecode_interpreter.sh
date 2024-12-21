# Compile
gcc -O3 -I../include ../src/*.c -o ../bin/cfeeny -Wno-int-to-void-pointer-cast


# Clean output folder
rm ../output/bytecode_interpreter/*.out
rm ../output/bytecode_interpreter/*.bc

# Run output
function test {
    ../bin/compiler -i ../examples/$1.feeny -o ../output/bytecode_interpreter/$1.bc
    ../bin/cfeeny -b ../output/bytecode_interpreter/$1.bc > ../output/bytecode_interpreter/$1.out
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
