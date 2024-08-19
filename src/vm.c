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
                case OBJ_NATIVE:
                    return OBJ_VAL(makeString("<native fn>", 11, false));
                case OBJ_CLOSURE: {
                    char buff[64];
                    ObjFunction *function = AS_CLOSURE(value)->function;
                    int written = snprintf(buff, 64, "<fn %.*s>", function->name->length, AS_CSTRING(function->name));
                    return OBJ_VAL(makeString(buff, written > 63 ? 63 : written, false));
                }
                case OBJ_FUNCTION:
                    break;
                case OBJ_UPVALUE:
                    return OBJ_VAL(makeString("upvalue", 7, false));
                case OBJ_CLASS: {
                    char buff[64];
                    ObjClass *klass = AS_CLASS(value);
                    int written = snprintf(buff, 64, "<fn %.*s>", klass->name->length, AS_CSTRING(klass->name));
                    return OBJ_VAL(makeString(buff, written > 63 ? 63 : written, false));
                }
                case OBJ_INSTANCE: {
                    char buff[64];
                    ObjInstance *instance = AS_INSTANCE(value);
                    int written = snprintf(buff, 64, "<fn %.*s>", instance->klass->name->length,
                                           AS_CSTRING(instance->klass->name));
                    return OBJ_VAL(makeString(buff, written > 63 ? 63 : written, false));
                }
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

static Value getFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        runtimeError("First argument should be a class instance.");
        return UNDEFINED_VAL;
    }

    if (!IS_STRING(args[1])) {
        runtimeError("Second argument should be a string.");
        return UNDEFINED_VAL;
    }

    ObjInstance *instance = AS_INSTANCE(args[0]);
    ObjString *name = AS_STRING(args[1]);
    Value value;

    if (!tableGet(&instance->fields, args[1], &value)) {
        runtimeError("Undefined field '%.*s'.", name->length, AS_CSTRING(name));
        return UNDEFINED_VAL;
    }

    return value;
}

static Value setFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        runtimeError("First argument should be a class instance.");
        return UNDEFINED_VAL;
    }

    if (!IS_STRING(args[1])) {
        runtimeError("Second argument should be a string.");
        return UNDEFINED_VAL;
    }

    ObjInstance *instance = AS_INSTANCE(args[0]);
    tableSet(&instance->fields, args[1], args[2]);

    return NIL_VAL;
}

static Value deleteFieldNative(int argCount, Value *args) {
    if (!IS_INSTANCE(args[0])) {
        runtimeError("First argument should be a class instance.");
        return UNDEFINED_VAL;
    }

    if (!IS_STRING(args[1])) {
        runtimeError("Second argument should be a string.");
        return UNDEFINED_VAL;
    }

    ObjInstance *instance = AS_INSTANCE(args[0]);
    tableDelete(&instance->fields, &args[1]);

    return NIL_VAL;
}


static void resetStack() {
    vm.stackTop = &vm.stack[0];
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
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
    if (buffer.globalVars.count + 1 > UINT16_MAX) {
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

    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    initTable(&vm.strings, VAL_OBJ);

    vm.initString = NULL;
    vm.initString = makeString("init", 4, false);

    initBuffer(&buffer);

    defineNative("clock", clockNative, 0);
    defineNative("str", strNative, 1);
    defineNative("sqrt", sqrtNative, 1);
    defineNative("getField", getFieldNative, 2);
    defineNative("setField", setFieldNative, 3);
    defineNative("deleteField", deleteFieldNative, 2);
}

void freeVM() {
    freeTable(&vm.strings);
    freeBuffer(&buffer);
    vm.initString = NULL;
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

static bool call(ObjClosure *closure, int argCount) {
    ObjFunction *function = closure->function;
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
    frame->closure = closure;
    frame->ip = function->chunk.code;
    frame->slots = (vm.stackTop - argCount - 1);
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
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
            case OBJ_CLASS: {
                ObjClass *klass = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

                if (!IS_NIL(klass->initializer)) {
                    return call(AS_CLOSURE(klass->initializer), argCount);
                }

                if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d.", argCount);
                    return false;
                }
                return true;
            }
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return call(bound->method, argCount);
            }
            default:
                break;
        }
    }

    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjClass *klass, ObjString *name, uint8_t arcCount) {
    Value method;
    if (!tableGet(&klass->methods, OBJ_VAL(name), &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), arcCount);
}

static bool invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance *instance = AS_INSTANCE(receiver);

    Value value;
    if (tableGet(&instance->fields, OBJ_VAL(name), &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

static bool bindMethod(ObjClass *klass, ObjString *name) {
    Value method;
    if (!tableGet(&klass->methods, OBJ_VAL(name), &method)) {
        return false;
    }

    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));

    pop(1);
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = NULL;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == NULL) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value *last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static bool defineMethod(ObjString *name) {
    Value method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, OBJ_VAL(name), method);

    if (name == vm.initString) {
        if (!IS_NIL(klass->initializer)) {
            return false;
        }

        klass->initializer = method;
    }
    pop(1);
    return true;
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *objB = (ObjString *) AS_OBJ(peek(0));
    ObjString *objA = (ObjString *) AS_OBJ(peek(1));

    int length = objA->length + objB->length;
    ObjString *result = allocateString(length, false);
    memcpy(result->chars, AS_CSTRING(objA), objA->length);
    memcpy(result->chars + objA->length, AS_CSTRING(objB), objB->length);
    result->chars[length] = '\0';
    result->hash = hashString(result->chars, length);

    ObjString *interned = tableFindString(&vm.strings, result->chars, length, result->hash);
    if (interned != NULL) {
        result = interned;
    } else {
        push(OBJ_VAL(result));
        tableSet(&vm.strings, OBJ_VAL(result), NIL_VAL);
        pop(1);
    }

    pop(2);
    push(OBJ_VAL(result));
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
    register uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() ({ \
    uint8_t byte1 = READ_BYTE(); \
    uint8_t byte2 = READ_BYTE(); \
    (byte1 | (byte2 << 8)); \
})
#define READ_LONG() ({ \
    uint8_t byte1 = READ_BYTE(); \
    uint8_t byte2 = READ_BYTE(); \
    uint8_t byte3 = READ_BYTE(); \
    (byte1 | (byte2 << 8) | (byte3 << 16)); \
})
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_LONG()])
#define BINARY_OP(valueType, op, type) \
    do {                               \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            frame->ip = ip;            \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                              \
        type b = AS_NUMBER(pop(1));    \
        type a = AS_NUMBER(pop(1));    \
        push(valueType(a op b));       \
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
        disassembleInstruction(&frame->closure->function->chunk, (int) (ip - frame->closure->function->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                push(READ_CONSTANT());
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
            case OP_GET_UPVALUE: {
                uint16_t slot = READ_SHORT();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint16_t slot = READ_SHORT();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    frame->ip = ip;
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = AS_STRING(READ_CONSTANT());

                Value value;
                if (tableGet(&instance->fields, OBJ_VAL(name), &value)) {
                    pop(1);
                    push(value);
                    break;
                }

                if (!bindMethod(instance->klass, name)) {
                    frame->ip = ip;
                    runtimeError("Undefined property '%.*s'.", name->length, AS_CSTRING(name));
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    frame->ip = ip;
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_CONSTANT(), peek(0));
                Value value = pop(1);
                pop(1);
                push(value);
                break;
            }
            case OP_GET_SUPER: {
                ObjString *name = AS_STRING(READ_CONSTANT());
                ObjClass *superclass = AS_CLASS(pop(1));

                if (!bindMethod(superclass, name)) {
                    frame->ip = ip;
                    runtimeError("Undefined super property '%.*s'.", name->length, AS_CSTRING(name));
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value b = pop(1);
                Value a = pop(1);
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >, double);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <, double);
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
                BINARY_OP(NUMBER_VAL, -, double);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *, double);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /, double);
                break;
            case OP_MODULO:
                BINARY_OP(NUMBER_VAL, %, int);
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
            case OP_INVOKE: {
                ObjString *method = AS_STRING(READ_CONSTANT());
                int argCount = READ_BYTE();
                frame->ip = ip;
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_INVOKE_SUPER: {
                ObjString *method = AS_STRING(READ_CONSTANT());
                int argCount = READ_BYTE();
                ObjClass *superclass = AS_CLASS(pop(1));
                frame->ip = ip;
                if (!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    closure->upvalues[i] = isLocal ? captureUpvalue(frame->slots + index)
                                                   : frame->closure->upvalues[index];
                }
                break;
            }
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop(1);
                break;
            case OP_RETURN: {
                Value result = pop(1);
                closeUpvalues(frame->slots);
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
            case OP_INHERIT: {
                Value superclass = peek(1);

                if (!IS_CLASS(superclass)) {
                    frame->ip = ip;
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjClass *subclass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop(1);
                break;
            }
            case OP_CLASS: {
                push(OBJ_VAL(newClass(AS_STRING(READ_CONSTANT()))));
                break;
            }
            case OP_METHOD:
                if (!defineMethod(AS_STRING(READ_CONSTANT()))) {
                    frame->ip = ip;
                    runtimeError("class initializer redeclared");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_LONG
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char *source) {
    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop(1);
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}