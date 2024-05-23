#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "object.h"

VM vm;

static void resetStack() {
    vm.stack = malloc(sizeof(Value) * STACK_MIN);
    vm.stackCapacity = STACK_MIN;
    vm.stackNextTop = 0;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    uint32_t line = getLine(&vm.chunk->lines, instruction);
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings, VAL_OBJ);
    initValueArray(&vm.globals);
}

void freeVM() {
    freeTable(&parser.variableIdentifiers);
    FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
    freeTable(&vm.strings);
    freeValueArray(&vm.globals);
    freeObjects();
}

void push(Value value) {
    if (vm.stackNextTop >= STACK_MAX) {
        fprintf(stderr, "Stack overflow.");
        exit(127);
    }

    if (vm.stackNextTop >= vm.stackCapacity) {
        size_t oldCapacity = vm.stackCapacity;
        vm.stackCapacity = GROW_CAPACITY(vm.stackCapacity);
        vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
    }
    vm.stack[vm.stackNextTop] = value;
    vm.stackNextTop++;
}

Value pop() {
    vm.stackNextTop--;
    return vm.stack[vm.stackNextTop];
}

static Value peek(int distance) {
    return vm.stack[vm.stackNextTop - 1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *objB = (ObjString *) AS_OBJ(pop());
    ObjString *objA = (ObjString *) AS_OBJ(pop());

    int length = objA->length + objB->length;
    ObjString *result = allocateString(length, false);
    memcpy(result->chars, AS_CSTRING(objA), objA->length);
    memcpy(result->chars + objA->length, AS_CSTRING(objB), objB->length);
    result->chars[length] = '\0';
    result->hash = hashString(result->chars, length);

    ObjString *interned = tableFindString(&vm.strings, result->chars, length, result->hash);
    if (interned != NULL) {
        FREE_STRING(result);
        result = interned;
    } else {
        tableSet(&vm.strings, OBJ_VAL(result), NIL_VAL);
    }

    push(OBJ_VAL(result));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_LONG_BYTE() (READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_LONG_BYTE()])
#define BINARY_OP(valueType, op) \
    do {                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while(false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("\t\t");
        for (Value *slot = vm.stack; slot < (vm.stack + vm.stackNextTop); slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int) (vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT:
            case OP_CONSTANT_LONG: {
                Value constant = (instruction == OP_CONSTANT ? READ_CONSTANT() : READ_CONSTANT_LONG());
                push(constant);
                break;
            }
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_POP:
                pop();
                break;
            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                uint32_t variableIndex = instruction == OP_GET_GLOBAL ? READ_BYTE() : READ_LONG_BYTE();
                Value* globals = vm.globals.values;
                if (globals[variableIndex].type == VAL_UNDEFINED) {
                    ObjString* varName = AS_STRING(globals[variableIndex - 1]);
                    runtimeError("Undefined variable '%.*s'.", varName->length, AS_CSTRING(varName));
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = globals[variableIndex];
                push(value);
                break;
            }
            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                uint32_t variableIndex = instruction == OP_SET_GLOBAL ? READ_BYTE() : READ_LONG_BYTE();
                Value* globals = vm.globals.values;

                if (globals[variableIndex].type == VAL_UNDEFINED) {
                    ObjString* varName = AS_STRING(globals[variableIndex - 1]);
                    runtimeError("Undefined variable '%.*s'.", varName->length, AS_CSTRING(varName));
                    return INTERPRET_RUNTIME_ERROR;
                }

                globals[variableIndex] = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                uint32_t variableIndex = instruction == OP_DEFINE_GLOBAL ? READ_BYTE() : READ_LONG_BYTE();
                vm.globals.values[variableIndex] = peek(0);
                pop();
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_ADD:
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_RETURN: {
                // Exit interpreter.
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_LONG_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILER_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}