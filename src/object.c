#include <stdio.h>
#include <string.h>

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

static Obj *allocateObject(size_t size, ObjType type) {
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

ObjString *allocateString(int length, bool referenced) {
    size_t size = sizeof(ObjString);
    if (!referenced) {
        size = sizeof(ObjString) + sizeof(char[length + 1]);
    }
    ObjString *string = (ObjString *) allocateObject(size, OBJ_STRING);
    string->referenced = NULL;
    string->length = length;
    return string;
}

ObjString *makeString(const char *chars, int length, bool reference) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjString *string = allocateString(length, reference);
    if (reference) {
        string->referenced = chars;
    } else {
        memcpy(string->chars, chars, length);
        string->chars[length] = '\0';
    }
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);
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
    return klass;
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields, VAL_OBJ);
    return instance;
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
    printf("<fn %.*s>", function->name->length, AS_CSTRING(function->name));
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            ObjString *strObj = AS_STRING(value);
            const char *str = AS_CSTRING(strObj);
            for (int i = 0; i < strObj->length; i++) {
                printf("%c", str[i]);
            }
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
            printf("%.*s", className->length, AS_CSTRING(className));
            break;
        }
        case OBJ_INSTANCE: {
            ObjString *className = AS_INSTANCE(value)->klass->name;
            printf("%.*s instance", className->length, AS_CSTRING(className));
            break;
        }
        default:
            printf("Unknown object.");
            break;
    }
}