#include <stdio.h>
#include "common.h"
#include "line.h"
#include "memory.h"

void initLineArray(LineArray* lineArray) {
    lineArray->capacity = 0;
    lineArray->count = 0;
    lineArray->array = NULL;
}

void writeLineArray(LineArray* lineArray, uint32_t line) {
    if (lineArray->capacity < lineArray->count + 1) {
        int oldCapacity = lineArray->capacity;
        lineArray->capacity = GROW_CAPACITY(oldCapacity);
        lineArray->array = GROW_ARRAY(Line, lineArray->array, oldCapacity, lineArray->capacity);
    }

    if (lineArray->count == 0 || lineArray->array[lineArray->count - 1].line != line) {
        lineArray->array[lineArray->count] = (Line){line, 1};
        lineArray->count++;
        return;
    }

    lineArray->array[lineArray->count - 1].count++;
}

uint32_t getLine(LineArray* lineArray, uint32_t offset) {
    size_t len = 0, at = 0;

    for (int i = 0; i < lineArray->count; i++) {
        len += lineArray->array[i].count;
    }

    uint32_t* lines = malloc(sizeof(uint32_t) * len);

    for (int i = 0; i < lineArray->count; i++) {
        for (int j = 0; j < lineArray->array[i].count; j++) {
            lines[at] = lineArray->array[i].line;
            at++;
        }
    }

    uint32_t result = lines[offset];
    free(lines);

    return result;
}

void freeLineArray(LineArray* lineArray) {
    FREE_ARRAY(Line, lineArray->array, lineArray->capacity);
    initLineArray(lineArray);
}