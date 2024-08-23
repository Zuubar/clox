#ifndef CLOX_OBJ_H
#define CLOX_OBJ_H

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)
#define IS_STRING(value)        (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value)      (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value)        (isObjType(value, OBJ_NATIVE))
#define IS_CLOSURE(value)       (isObjType(value, OBJ_CLOSURE))
#define IS_UPVALUE(value)       (isObjType(value, OBJ_UPVALUE))
#define IS_CLASS(value)         (isObjType(value, OBJ_CLASS))
#define IS_INSTANCE(value)      (isObjType(value, OBJ_INSTANCE))
#define IS_BOUND_METHOD(value)  (isObjType(value, OBJ_BOUND_METHOD))
#define IS_ARRAY(value)         (isObjType(value, OBJ_ARRAY))

#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(objString)   (objString->referenced != NULL ? objString->referenced : objString->chars)
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value)))
#define AS_CLOSURE(value)       (((ObjClosure*)AS_OBJ(value)))
#define AS_UPVALUE(value)       (((ObjUpvalue*)AS_OBJ(value)))
#define AS_CLASS(value)         (((ObjClass*)AS_OBJ(value)))
#define AS_INSTANCE(value)      (((ObjInstance*)AS_OBJ(value)))
#define AS_BOUND_METHOD(value)  (((ObjBoundMethod*)AS_OBJ(value)))
#define AS_ARRAY(value)         (((ObjArray*)AS_OBJ(value)))
#define FREE_STRING(objString)  (reallocate(objString, sizeof(ObjString) + ((objString)->referenced != NULL ? 0 : sizeof(char[(objString)->length + 1])), 0))


typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_NATIVE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD,
    OBJ_ARRAY,
} ObjType;

struct Obj {
    struct Obj *next;
    ObjType type;
    bool isMarked;
};

struct ObjString {
    Obj obj;
    int length;
    uint32_t hash;
    const char *referenced;
    char chars[];
};

typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction *function;
    int upvalueCount;
    ObjUpvalue **upvalues;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString *name;
    Value initializer;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *klass;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    int capacity;
    int count;
    Value *values;
} ObjArray;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
    int arity;
} ObjNative;

uint32_t hashString(const char *key, int length);

struct ObjString *allocateString(int length, bool referenced);

struct ObjString *makeString(const char *chars, int length, bool reference);

Obj *allocateObject(size_t size, ObjType type);

ObjFunction *newFunction();

ObjNative *newNative(NativeFn function, int arity);

ObjClosure *newClosure(ObjFunction *function);

ObjUpvalue *newUpvalue(Value *slot);

ObjClass *newClass(ObjString *name);

ObjInstance *newInstance(ObjClass *klass);

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);

ObjArray *newArray(Value *start, uint16_t length);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJ_H
