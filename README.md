# Clox - yet another superset of Lox programming language

Clox is a superset of the Lox programming language like [Glox](https://github.com/Zuubar/glox), the main difference being, that Clox is a bytecode interpreter. As a result, it runs at the same speed as python and in certain scenarios, even surpasses python by 10-50%. 

## New Features

### 1. Multiline Comments
Multiline comments can be written between `/*` and `*/`, allowing for more flexible commentary within your code.

```lox
/*
This is a multiline comment.
You can add as many lines as you wish.
*/
var bits = 64;
```

### 2. Modulus Operator
The modulus operator % is introduced for performing arithmetic modulus operations.
```lox
var remainder = 10 % 3; // 1
```

### 3. Ternary Operators
Ternary operators offer a concise way to perform if-else operations in a single line.
```lox
var result = condition ? "True" : "False";
```

### 4. Built-in str() Function
The "str()" function converts non-string values into strings, useful for string concatenation.
```lox
var myNumber = 10;
print("My number is " + str(myNumber));
```

### 5. Break and Continue Statements
"break" and "continue" statements control the flow of loops more precisely.
```lox
for (var i = 0; i < 10; i = i + 1) {
    if (i == 5) continue; // Skip the rest of the loop when i is 5.
    if (i == 8) break; // Exit the loop when i is 8.
    print(i);
}
```

### 6. Switch
Clox has support for C-style switch statements.
```lox
var code = 2;
switch(a) {
    case 1:
        print 1;
    case 2:
        print 2; // Fallthroughs to next case if "break" is ommited.
    case 3:
        print 3;
        break;
    default:
        print "default";
}
/* 
Output:
2
3
*/ 
```

### 7. New variable on every For(...) loop iteration
Clox creates new version of main loop variable every new iteration, in order to preserve their values accurately in closures. 
```lox
var globalOne;
var globalTwo;

fun main() {
  for (var a = 1; a <= 2; a = a + 1) {
    fun closure() {
      print a; // Captures exact value of "a" at the time of declaration. 
    }
    if (globalOne == nil) {
      globalOne = closure;
    } else {
      globalTwo = closure;
    }
  }
}

main();
globalOne();
globalTwo();
/* 
Output:
1
2
*/ 
```

### 7. Arrays
Clox supports arrays to store a sequence of values.
```lox
var byte = [0, 0, 0, 0, 0, 0, 0, 0];
byte[0] = 1;
byte[4] = 1;
byte[7] = 1;
print(byte); //prints [1, 0, 0, 0, 1, 0, 0, 1]
```

Built-in "append" function modifies underlying array in place.
```lox
var primes = [];
append(primes, 2);
append(primes, 3);
append(primes, 5);
print(primes); //prints [2, 3, 5]
```

## Changes under the hood

Clox's primary focus is speed, therefore the majority of changes are concealed from the end-user. Main changes are:
1. **OP_CONST** instruction is 24 bits, which allow more than 256 constants in 1 script.
2. VM's **instruction pointer** is always stored in register during the program execution, thus increasing the speed of VM.
3. **OP_GET_GLOBAL** && **OP_SET_GLOBAL** instructions are much faster, thanks to the "buffer" between compiler and VM.
4. In order to reduce the overhead of poping several items from stack, **OP_POPN** instruction was introduced to pop the N number of items with 1 instruction.
5. Following instructions had their operand size increased to 16 bits:
   - **OP_GET_LOCAL / OP_SET_LOCAL**
   - **OP_GET_PROPERTY / OP_SET_PROPERTY**
   - **OP_GET_UPVALUE / OP_SET_UPVALUE**
   - **OP_GET_SUPER**

## Building
Clox only requires `C11`, `cmake` and `ninja` alongside only 1 third-party dependency which is bundled, so building it should be a breeze.
- Release version:
    ```bash
    cmake -B <build_directory_name> -DCMAKE_BUILD_TYPE=Release -G Ninja . && ninja <build_directory_name> 
    ```
- Debug version with sanitizers:
    ```bash
    cmake -B <build_directory_name> -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-fsanitize=address,undefined,leak -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined,leak -DDEBUG_TRACE_EXECUTION:BOOL=OFF -DDEBUG_PRINT_CODE:BOOL=OFF -DDEBUG_LOG_GC:BOOL=OFF -DDEBUG_STRESS_GC:BOOL=ON -DNAN_BOXING:BOOL=ON -G Ninja . && ninja <build_directory_name>
    ```
  additional variables can be found in [CMakeLists.txt](src/CMakeLists.txt) to enable all kinds of debugging options.

