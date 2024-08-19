#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,

    VAL_UNDEFINED
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

#define IS_BOOL(value)         ((value).type == VAL_BOOL)
#define IS_NIL(value)          ((value).type == VAL_NIL)
#define IS_NUMBER(value)       ((value).type == VAL_NUMBER)
#define IS_OBJ(value)          ((value).type == VAL_OBJ)
#define IS_UNDEFINED(value)    ((value).type == VAL_UNDEFINED)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)    ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL            ((Value){VAL_NIL, {.number = 0}})
#define UNDEFINED_VAL      ((Value){VAL_UNDEFINED, {.number = 0}})
#define NUMBER_VAL(value)  ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)    ((Value){VAL_OBJ, {.obj = (Obj*)(object)}})

#define VALUE_HASH(value, type) \
    uint32_t valueHash = 0;     \
    do {                        \
        switch(type) {          \
            case VAL_NUMBER: {  \
                double number = AS_NUMBER(value); \
                valueHash = hashString((char *)&number, sizeof(number)); \
                break;          \
            }                   \
            case VAL_BOOL: {    \
                valueHash = AS_BOOL(value) ? 1 : 0;                      \
                break;          \
            }                   \
            case VAL_OBJ: {     \
                switch (OBJ_TYPE(value)) {        \
                    case OBJ_STRING:              \
                        valueHash = AS_STRING(value)->hash;              \
                        break;  \
                }               \
                break;          \
            }                   \
            default:            \
                printf("Unsupported table key type.");                   \
                exit(1);        \
        }                       \
    } while(false)              \


typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

bool valuesEqual(Value a, Value b);

void initValueArray(ValueArray *array);

void writeValueArray(ValueArray *array, Value value);

void freeValueArray(ValueArray *array);

void printValue(Value value);

#endif //CLOX_VALUE_H
