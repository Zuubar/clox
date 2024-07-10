#ifndef CLOX_OBJ_H
#define CLOX_OBJ_H

#include "common.h"
#include "value.h"
#include "chunk.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value)  (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value)    (isObjType(value, OBJ_NATIVE))
#define IS_CLOSURE(value)   (isObjType(value, OBJ_CLOSURE))
#define IS_UPVALUE(value)   (isObjType(value, OBJ_UPVALUE))

#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(objString)   (objString->referenced != NULL ? objString->referenced : objString->chars)
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)        (((ObjNative*)AS_OBJ(value)))
#define AS_CLOSURE(value)       (((ObjClosure*)AS_OBJ(value)))
#define AS_UPVALUE(value)       (((ObjUpvalue*)AS_OBJ(value)))
#define FREE_STRING(objString)  (reallocate(objString, sizeof(ObjString) + ((objString)->referenced != NULL ? 0 : sizeof(char[(objString)->length + 1])), 0))


typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_NATIVE,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
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
    ObjUpvalue** upvalues;
} ObjClosure;

typedef Value (*NativeFn)(int argCount, Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
    int arity;
} ObjNative;

uint32_t hashString(const char *key, int length);

struct ObjString *allocateString(int length, bool referenced);

struct ObjString *makeString(const char *chars, int length, bool reference);

ObjFunction *newFunction();

ObjNative *newNative(NativeFn function, int arity);

ObjClosure *newClosure(ObjFunction *function);

ObjUpvalue *newUpvalue(Value *slot);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJ_H
