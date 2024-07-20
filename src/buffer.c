#include "buffer.h"
#include "memory.h"

void initBuffer(Buffer *buff) {
    initTable(&buff->globalVarIdentifiers, VAL_OBJ);
    initValueArray(&buff->globalVars);
    initTable(&buff->constVarIdentifiers, VAL_OBJ);
}

void freeBuffer(Buffer *buff) {
    freeTable(&buff->globalVarIdentifiers);
    freeValueArray(&buff->globalVars);
    freeTable(&buff->constVarIdentifiers);
}

void markBufferRoots() {
    for (int i = 0; i < buffer.globalVars.count; i++) {
        markValue(buffer.globalVars.values[i]);
    }
}