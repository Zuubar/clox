#ifndef CLOX_LINE_H
#define CLOX_LINE_H

#include "common.h"

typedef struct {
    uint32_t line;
    int count;
} Line;

typedef struct {
    int capacity;
    int count;
    Line* array;
} LineArray;

void initLineArray(LineArray* lineArray);
void writeLineArray(LineArray* lineArray, uint32_t line);
uint32_t getLine(LineArray* lineArray, uint32_t offset);
void freeLineArray(LineArray* lineArray);

#endif //CLOX_LINE_H
