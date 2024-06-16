#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "buffer.h"
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "object.h"

Buffer buffer;
VM vm;

static void runtimeError(const char *format, ...);

static Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}

static Value strNative(int argCount, Value *args) {
    Value value = args[0];
    switch (value.type) {
        case VAL_BOOL:
            return OBJ_VAL(AS_BOOL(value) ? makeString("true", 4, false) : makeString("false", 5, false));
        case VAL_NIL:
            return OBJ_VAL(makeString("nil", 3, false));
        case VAL_NUMBER: {
            char buff[32];
            int written = snprintf(buff, 32, "%g", AS_NUMBER(value));
            return OBJ_VAL(makeString(buff, written > 31 ? 31 : written, false));
        }
        case VAL_OBJ:
            switch (OBJ_TYPE(value)) {
                case OBJ_STRING: {
                    return value;
                }
                case OBJ_FUNCTION: {
                    char buff[64];
                    ObjFunction *function = AS_FUNCTION(value);
                    int written = snprintf(buff, 64, "<fn %.*s>", function->name->length, AS_CSTRING(function->name));
                    return OBJ_VAL(makeString(buff, written > 63 ? 63 : written, false));
                }
                case OBJ_NATIVE:
                    return OBJ_VAL(makeString("<native fn>", 11, false));
            }
        default:
            runtimeError("Unsupported type.");
            return UNDEFINED_VAL;
    }
}

static Value sqrtNative(int argCount, Value *args) {
    if (!IS_NUMBER(args[0])) {
        runtimeError("Argument should be a number.");
        return UNDEFINED_VAL;
    }
    return NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
}

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

static void defineNative(const char *name, NativeFn function, int arity) {
    if (buffer.globalVars.count + 1 > 0) {
        fprintf(stderr, "Too many native functions.\n");
        exit(127);
    }

    push(OBJ_VAL(makeString(name, (int) strlen(name), false)));
    push(OBJ_VAL(newNative(function, arity)));

    writeValueArray(&buffer.globalVars, vm.stack[0]);
    writeValueArray(&buffer.globalVars, vm.stack[1]);
    tableSet(&buffer.globalVarIdentifiers, vm.stack[0], NUMBER_VAL(buffer.globalVars.count - 1));

    pop(2);
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings, VAL_OBJ);
    initBuffer(&buffer);

    defineNative("clock", clockNative, 0);
    defineNative("str", strNative, 1);
    defineNative("sqrt", sqrtNative, 1);
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
            case OBJ_NATIVE: {
                ObjNative *native = AS_NATIVE(callee);
                if (native->arity != argCount) {
                    runtimeError("Expected %d arguments but got %d.",
                                 native->arity, argCount);
                    return false;
                }
                Value result = native->function(argCount, vm.stackTop - argCount);
                if (IS_UNDEFINED(result)) {
                    return false;
                }
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
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
    register uint8_t *ip = frame->ip;

#define READ_BYTE() (*(ip++))
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
    do {                         \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            frame->ip = ip;      \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                        \
        double b = AS_NUMBER(pop(1));                     \
        double a = AS_NUMBER(pop(1));                     \
        push(valueType(a op b)); \
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
        disassembleInstruction(&frame->function->chunk, (int) (ip - frame->function->chunk.code));
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
                    frame->ip = ip;
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
                    frame->ip = ip;
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
                    frame->ip = ip;
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
                    frame->ip = ip;
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
                ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                ip += isFalsey(peek(0)) * offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                frame->ip = ip;
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
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
                ip = frame->ip;
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