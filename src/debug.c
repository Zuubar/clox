#include <stdio.h>

#include "debug.h"
#include "value.h"
#include "object.h"

void disassembleChunk(Chunk *chunk, ObjString *name) {
    if (name == NULL) {
        printf("== <script> ==\n");
    } else {
        printf("== %.*s ==\n", name->length, AS_CSTRING(name));
    }


    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

inline static int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

inline static int byteInstruction(const char *name, Chunk *chunk,
                                  int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

inline static int shortInstruction(const char *name, Chunk *chunk, int offset) {
    uint16_t operand = chunk->code[offset + 1] |
                       (chunk->code[offset + 2] << 8);
    printf("%-16s %4d\n", name, operand);
    return offset + 3;
}

inline static int longInstruction(const char *name, Chunk *chunk, int offset) {
    uint32_t operand = chunk->code[offset + 1] |
                       (chunk->code[offset + 2] << 8) |
                       (chunk->code[offset + 3] << 16);
    printf("%-16s %4d\n", name, operand);
    return offset + 4;
}

inline static int constantInstruction(Chunk *chunk, int offset) {
    uint32_t operand = chunk->code[offset + 1] |
                       (chunk->code[offset + 2] << 8) |
                       (chunk->code[offset + 3] << 16);
    printf("%-16s %4d '", "OP_CONSTANT", operand);
    printValue(chunk->constants.values[operand]);
    printf("'\n");
    return offset + 4;
}

inline static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    uint16_t jump = chunk->code[offset + 1] |
                    (chunk->code[offset + 2] << 8);
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}

inline static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    uint32_t constant = chunk->code[offset + 1] |
                        (chunk->code[offset + 2] << 8) |
                        (chunk->code[offset + 3] << 16);
    uint8_t argCount = chunk->code[offset + 4];

    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 5;
}

int disassembleInstruction(Chunk *chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 && getLine(&chunk->lines, offset) == getLine(&chunk->lines, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", getLine(&chunk->lines, offset));
    }
    printf("%4d ", getLine(&chunk->lines, offset));

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction(chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_DUPLICATE:
            return simpleInstruction("OP_DUPLICATE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_POPN:
            return shortInstruction("OP_POPN", chunk, offset);
        case OP_GET_GLOBAL:
            return shortInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return shortInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return shortInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_LOCAL:
            return shortInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return shortInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_UPVALUE:
            return shortInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return shortInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY:
            return longInstruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:
            return longInstruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_SUPER:
            return longInstruction("OP_GET_SUPER", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return invokeInstruction("OP_INVOKE", chunk, offset);
        case OP_INVOKE_SUPER:
            return invokeInstruction("OP_INVOKE_SUPER", chunk, offset);
        case OP_CLOSURE: {
            int constant = (chunk->code[offset + 1] | (chunk->code[offset + 2] << 8) | (chunk->code[offset + 3] << 16));
            offset += 4;
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(chunk->constants.values[constant]);
            printf("\n");

            ObjFunction *function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d    |                            %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }

            return offset;
        }
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CLASS:
            return longInstruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OP_METHOD:
            return longInstruction("OP_METHOD", chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            exit(1);
    }
}