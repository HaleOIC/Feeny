# Compile
gcc -O3 -I../include ../src/*.c -o ../bin/cfeeny -Wno-int-to-void-pointer-cast


if [ ! -d "../output/bytecode_compiler" ]; then
    mkdir -p ../output/bytecode_compiler
    echo "Created directory: ../output/bytecode_compiler"
fi

# Clean output folder - only if directory exists and contains files
if [ -d "../output/bytecode_compiler" ]; then
    rm -f ../output/bytecode_compiler/*.out 2>/dev/null
    rm -f ../output/bytecode_compiler/*.bc 2>/dev/null
fi
# Run output
function test {
    ../bin/parser -i ../examples/$1.feeny -oast ../output/bytecode_compiler/$1.ast
    ../bin/cfeeny -f -v ../output/bytecode_compiler/$1.ast > ../output/bytecode_compiler/$1.out
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
test morehanoi
test stack
