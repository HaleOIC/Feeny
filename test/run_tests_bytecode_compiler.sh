# Compile
cd ../
make compile
cd test

if [ ! -d "../output" ]; then
    mkdir ../output
    echo "Created directory: ../output"
fi

if [ ! -d "../output/bytecode_compiler" ]; then
    mkdir -p ../output/bytecode_compiler
    echo "Created directory: ../output/bytecode_compiler"
fi

# Clean output folder - only if directory exists and contains files
if [ -d "../output/bytecode_compiler" ]; then
    rm -f ../output/bytecode_compiler/*.out 2>/dev/null
fi
# Run output
function test {
    echo "Running bytecode compiler on $1.feeny"
    ../bin/cfeeny -f ./$1.feeny > ../output/bytecode_compiler/$1.out
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
test sudoku3
test hanoi
test morehanoi
test stack
