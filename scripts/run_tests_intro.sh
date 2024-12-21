# Clean output folder
rm ../output/intro/*.out

# Run output
function test {
   ../bin/feeny -e ../examples/$1.feeny > ../output/intro/$1.out
}
test hanoi
test stack
test morehanoi

