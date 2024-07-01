#include <stdio.h>
#include "chunk.h"
#include "line.h"

void initChunk(Chunk *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    initLineArray(&chunk->lines);
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    freeLineArray(&chunk->lines);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, uint32_t line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
    writeLineArray(&chunk->lines, line);
}

int addConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int writeConstant(Chunk *chunk, Value value, int line) {
    if (chunk->constants.count + 1 == UINT24_MAX) {
        printf("Too many constant in one chunk.");
        exit(127);
    }

    int constant = addConstant(chunk, value);
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t) constant & 0xff, line);
    writeChunk(chunk, (uint8_t)((constant >> 8) & 0xff), line);
    writeChunk(chunk, (uint8_t)((constant >> 16) & 0xff), line);

    return constant;
}
