#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "scanner.h"
#include "vm.h"


typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Table variableIdentifiers;
} Parser;

extern Parser parser;

bool compile(const char *source, Chunk *chunk);

#endif //CLOX_COMPILER_H
