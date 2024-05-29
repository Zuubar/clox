#ifndef CLOX_VM_H
#define CLOX_VM_H

#define STACK_MIN 256
#define STACK_MAX 2048

#include "chunk.h"
#include "value.h"
#include "table.h"

typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value *stack;
    size_t stackCapacity;
    size_t stackNextTop;
    Table strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILER_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();

void freeVM();

InterpretResult interpret(const char* source);

void push(Value value);

Value pop(uint16_t count);

#endif //CLOX_VM_H
