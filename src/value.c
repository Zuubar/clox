#include <stdio.h>
#include <memory.h>

#include "memory.h"
#include "value.h"
#include "object.h"

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->count = 0;
    array->capacity = 0;
}

void writeValueArray(ValueArray *array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_UNDEFINED(value)) {
        printf("undefined");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(value);
    }
#else
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
        case VAL_UNDEFINED:
            printf("undefined");
            break;
        default:
            printf("Unreachable.");
            return; // Unreachable
    }
#endif
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
//    Uncomment to fully implement IEEE 754 specs(e.g NAN != NAN)
//    if (IS_NUMBER(a) && IS_NUMBER(b)) {
//        return AS_NUMBER(a) == AS_NUMBER(b);
//    }

    return a == b;
#else
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_NIL:
            return true;
        case VAL_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
        default:
            return false; // Unreachable.
    }
#endif
}
