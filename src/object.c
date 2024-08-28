#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, ObjType) \
    (type*)allocateObject(sizeof(type), ObjType)

uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;


#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void *) object, size, type);
#endif

    return object;
}

ObjString *allocateString(int length) {
    size_t size = sizeof(ObjString);
    ObjString *string = (ObjString *) allocateObject(size, OBJ_STRING);
    string->length = length;
    string->chars = NULL;
    string->reference = false;
    return string;
}

ObjString *makeString(const char *chars, int length, bool reference) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjString *string = allocateString(length);
    push(OBJ_VAL(string));
    string->reference = reference;

    if (reference) {
        string->chars = chars;
    } else {
        string->chars = ALLOCATE(char, length + 1);
        memcpy(string->chars, chars, length);
        string->chars[length] = '\0';
    }
    string->hash = hash;

    tableSet(&vm.strings, string, NIL_VAL);
    pop(1);
    return string;
}

ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjNative *newNative(NativeFn function, int arity) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity;
    return native;
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjClass *newClass(ObjString *name) {
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    klass->initializer = NIL_VAL;
    initTable(&klass->methods);
    return klass;
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjArray *newArray(Value *source, uint16_t length) {
    int capacity = length;
    if (length != 0) {
        capacity -= 1;
        for (int i = 1; i < 32; i *= 2) {
            capacity |= capacity >> i;
        }
        capacity += 1;
    }

    ObjArray *arrayObj = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    push(OBJ_VAL(arrayObj));
    arrayObj->count = 0;
    arrayObj->capacity = capacity;
    arrayObj->values = GROW_ARRAY(Value, NULL, 0, arrayObj->capacity);
    pop(1);

    for (Value *val = source; val < source + length; val++) {
        arrayObj->values[arrayObj->count++] = *val;
    }
    return arrayObj;
}

ObjUpvalue *newUpvalue(Value *value) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = value;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %.*s>", function->name->length, function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            ObjString *strObj = AS_STRING(value);
            printf("%.*s", strObj->length, strObj->chars);
            break;
        }
        case OBJ_FUNCTION: {
            printFunction(AS_FUNCTION(value));
            break;
        }
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        case OBJ_CLASS: {
            ObjString *className = AS_CLASS(value)->name;
            printf("%.*s", className->length, className->chars);
            break;
        }
        case OBJ_INSTANCE: {
            ObjString *className = AS_INSTANCE(value)->klass->name;
            printf("%.*s instance", className->length, className->chars);
            break;
        }
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_ARRAY: {
            ObjArray *array = AS_ARRAY(value);
            printf("[");
            for (int i = 0; i < array->count; i++) {
                printValue(array->values[i]);
                if (i + 1 != array->count) {
                    printf(", ");
                }
            }
            printf("]");
            break;
        }
        default:
            printf("Unknown object.");
            break;
    }
}
