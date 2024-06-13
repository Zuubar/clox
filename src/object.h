#ifndef CLOX_OBJ_H
#define CLOX_OBJ_H

#include "common.h"
#include "value.h"
#include "chunk.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value)    (isObjType(value, OBJ_FUNCTION))

#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(objString)   (objString->referenced != NULL ? objString->referenced : objString->chars)
#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define FREE_STRING(objString)  (reallocate(objString, sizeof(ObjString) + ((objString)->referenced != NULL ? 0 : sizeof(char[(objString)->length + 1])), 0))


typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;
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
    Chunk chunk;
    ObjString *name;
} ObjFunction;

uint32_t hashString(const char *key, int length);

struct ObjString *allocateString(int length, bool referenced);

struct ObjString *makeString(const char *chars, int length, bool reference);

ObjFunction *newFunction();

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJ_H
