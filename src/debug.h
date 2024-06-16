#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, ObjString *func);
int disassembleInstruction(Chunk* chunk, int offset);

#endif //CLOX_DEBUG_H
