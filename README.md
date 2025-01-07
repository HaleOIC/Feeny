# Feeny Language

Feeny is a dynamic programming language that runs on a self-constructed virtual machine. This implementation includes a bytecode compiler and interpreter, demonstrating core concepts of language implementation.

## Features

- Dynamic typing system with primitive types and objects
- Object-oriented programming support with inheritance
- Garbage collection
- NaN boxing optimization for efficient value representation
- Python-like indentation-based syntax

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/HaleOIC/FeenyLanguage.git
cd feeny

# Build the interpreter
make compile
```

After running the above commands, an executable file is located in `./bin` directory.

### Running Your First Feeny Program

1. Create a file `hello.feeny`:

   ```feeny
   printf("Hello, World!\n")
   ```

2. Run it, there are two versions of interpretation, one is ast-level interpretation with flag `-a`, the other is bytecode-level interpretation with flag `-f`:

   ```bash
   ./bin/cfeeny -a hello.feeny # ast interpreter
   ./bin/cfeeny -f hello.feeny # bytecode compiler on vm
   ```

### Basic Syntax

1. Variables and Functions

   ```feeny
    var x = 42          ;Global variable
    defn add(a, b):     ; Function definition
        a + b
   ```

2. Objects and Methods

   ```feeny
    object:
        var x = 10
        var y = 20
        method sum():
            this.x + this.y
   ```

3. Control Flow

   ```feeny
    ; If statement
    if x < 10:
        printf("Less than 10\n")
    else:
        printf("Greater or equal to 10\n")

    ; While loop
    while i < 10:
        printf("i = ~\n", i)
        i = i + 1
   ```

More syntax can be found in `./docs`

## Example Code

```feeny
defn three():
    3

defn hello(i):
    printf("Hello ~ ", i)

defn world(j, k):
    printf("World ~ ~\n", j, k)

defn main():
    var i = three()
    hello(i)
    i = 10
    world(4, i)

main()
```

### Test

```bash
cd test
./run_test_bytecode_compiler.sh
./run_test_ast_interpreter.sh
```
