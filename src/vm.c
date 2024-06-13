#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "buffer.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "object.h"

Buffer buffer;
VM vm;

static void resetStack() {
    vm.stackTop = &vm.stack[0];
    vm.frameCount = 0;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        uint32_t line = getLine(&function->chunk.lines, instruction);
        fprintf(stderr, "[line %u] in ", line);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%.*s()\n", function->name->length, AS_CSTRING(function->name));
        }
    }
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings, VAL_OBJ);
    initBuffer(&buffer);
}

void freeVM() {
    freeTable(&vm.strings);
    freeBuffer(&buffer);
    freeObjects();
}

void push(Value value) {
    if (vm.stackTop == &vm.stack[STACK_MAX - 1]) {
        fprintf(stderr, "Stack overflow.");
        exit(127);
    }
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop(uint16_t count) {
    vm.stackTop -= count;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return *(vm.stackTop - 1 - distance);
}

static bool call(ObjFunction *function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = (vm.stackTop - argCount - 1);
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            default:
                break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *objB = (ObjString *) AS_OBJ(pop(1));
    ObjString *objA = (ObjString *) AS_OBJ(pop(1));

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
    CallFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() ({ \
    uint8_t byte1 = READ_BYTE(); \
    uint8_t byte2 = READ_BYTE(); \
    (byte1 | (byte2 << 8)); \
})
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() ({ \
    uint8_t byte1 = READ_BYTE();\
    uint8_t byte2 = READ_BYTE();\
    uint8_t byte3 = READ_BYTE();\
    (frame->function->chunk.constants.values[(byte1 | (byte2 << 8) | (byte3 << 16))]); \
})
#define BINARY_OP(valueType, op) \
    do {                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop(1));                      \
        double a = AS_NUMBER(pop(1));                      \
        push(valueType(a op b));                          \
    } while(false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("\t\t");
        for (Value *slot = vm.stack; slot != vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->function->chunk, (int) (frame->ip - frame->function->chunk.code));
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
            case OP_DUPLICATE:
                push(peek(0));
                break;
            case OP_POP:
                pop(1);
                break;
            case OP_POPN:
                pop(READ_SHORT());
                break;
            case OP_GET_GLOBAL: {
                uint16_t variableIndex = READ_SHORT();
                Value *globals = buffer.globalVars.values;
                if (globals[variableIndex].type == VAL_UNDEFINED) {
                    ObjString *varName = AS_STRING(globals[variableIndex - 1]);
                    runtimeError("Undefined variable '%.*s'.", varName->length, AS_CSTRING(varName));
                    return INTERPRET_RUNTIME_ERROR;
                }

                Value value = globals[variableIndex];
                push(value);
                break;
            }
            case OP_SET_GLOBAL: {
                uint16_t variableIndex = READ_SHORT();
                Value *globals = buffer.globalVars.values;

                if (globals[variableIndex].type == VAL_UNDEFINED) {
                    ObjString *varName = AS_STRING(globals[variableIndex - 1]);
                    runtimeError("Undefined variable '%.*s'.", varName->length, AS_CSTRING(varName));
                    return INTERPRET_RUNTIME_ERROR;
                }

                globals[variableIndex] = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                uint16_t variableIndex = READ_SHORT();
                buffer.globalVars.values[variableIndex] = peek(0);
                pop(1);
                break;
            }
            case OP_GET_LOCAL: {
                uint16_t slot = READ_SHORT();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint16_t slot = READ_SHORT();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_EQUAL: {
                Value b = pop(1);
                Value a = pop(1);
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
                    double b = AS_NUMBER(pop(1));
                    double a = AS_NUMBER(pop(1));
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
                push(BOOL_VAL(isFalsey(pop(1))));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop(1))));
                break;
            case OP_PRINT:
                printValue(pop(1));
                printf("\n");
                break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                frame->ip += isFalsey(peek(0)) * offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_RETURN: {
                Value result = pop(1);
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop(1); // pop script function at the bottom of the stack
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    call(function, 0);

    return run();
}