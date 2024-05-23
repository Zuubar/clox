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

    object->next = vm.objects;
    vm.objects = object;

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
    tableSet(&vm.strings, OBJ_VAL(string), NIL_VAL);
    return string;
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
        default:
            printf("Unreachable.");
            break;
    }
}