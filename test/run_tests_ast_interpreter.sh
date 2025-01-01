# Compile
cd ../
make compile
cd test

if [ ! -d "../output" ]; then
    mkdir ../output
fi

if [ ! -d "../output/astInterpreter" ]; then
    mkdir ../output/astInterpreter
fi

# Clean output folder
rm ../output/astInterpreter/*.out > /dev/null 2>&1
rm ../output/astInterpreter/*.ast > /dev/null 2>&1

# Run output
function test {
    ../bin/parser -i ./$1.feeny -oast ../output/astInterpreter/$1.ast
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