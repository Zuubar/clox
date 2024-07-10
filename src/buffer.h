#ifndef CLOX_BUFFER_H
#define CLOX_BUFFER_H

#include "table.h"

typedef struct {
    Table globalVarIdentifiers;
    ValueArray globalVars;
    Table constVarIdentifiers;
} Buffer;

extern Buffer buffer;

void initBuffer(Buffer *buff);

void freeBuffer(Buffer *buff);

void markBufferRoots();

#endif //CLOX_BUFFER_H
