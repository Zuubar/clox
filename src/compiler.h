#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "object.h"
#include "scanner.h"
#include "vm.h"

#define LOCALS_MIN (UINT8_MAX + 1)
#define LOCALS_MAX (UINT16_MAX + 1)

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

ObjFunction* compile(const char *source);

#endif //CLOX_COMPILER_H
