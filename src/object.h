#ifndef CLOX_OBJ_H
#define CLOX_OBJ_H

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))

#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(objString)   (objString->referenced != NULL ? objString->referenced : objString->chars)
#define FREE_STRING(objString)  (reallocate(objString, sizeof(ObjString) + ((objString)->referenced != NULL ? 0 : sizeof(char[(objString)->length + 1])), 0))


typedef enum {
    OBJ_STRING,
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

uint32_t hashString(const char *key, int length);

struct ObjString *allocateString(int length, bool referenced);

struct ObjString *makeString(const char *chars, int length, bool reference);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJ_H
