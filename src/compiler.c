#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "common.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "table.h"

#ifdef DEBUG_PRINT_CODE

#include "debug.h"

#endif

#define UINT8_COUNT (UINT8_MAX + 1)

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_CONDITIONAL,     // ?:
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool isConst;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} FunctionType;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;

    Upvalue upvalues[UINT8_COUNT];

    Local *locals;
    int localCount;
    int localCapacity;
    int scopeDepth;
    int loopStart;
    int switchCaseDepth;
    int loopScopeDepth;
    struct {
        int stack[UINT8_MAX];
        int top;
        int count;
    } LoopBreak;
    struct {
        int stack[UINT8_MAX];
        int top;
        int count;
    } SwitchBreak;
} Compiler;

Parser parser;
Compiler *current = NULL;

#define PATCH_BREAK(breakType) \
    do {                       \
        for (int i = 0; i < (breakType).count; i++) { \
            patchJump((breakType).stack[--(breakType).top]); \
        }                      \
    } while(false)             \


static void expression();

static void statement();

static void declaration();

static ParseRule *getRule(TokenType type);

static void parsePrecedence(Precedence precedence);

static Chunk *currentChunk() {
    return &current->function->chunk;
}

static void errorAt(Token *token, const char *message) {
    if (parser.panicMode) return;

    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:;
        }
        advance();
    }
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

inline static void emitShort(uint8_t instruction, uint32_t operand) {
    if (operand > UINT16_MAX) {
        error("Unsupported number of resources for this operation.");
        return;
    }
    emitByte(instruction);
    emitByte((uint8_t) operand & 0xff);
    emitByte((uint8_t) ((operand >> 8) & 0xff));
}

inline static void emitLong(uint8_t instruction, int operand) {
    if (operand > UINT24_MAX) {
        error("Unsupported number of resources for this operation.");
        return;
    }
    emitByte(instruction);
    emitByte((uint8_t) operand & 0xff);
    emitByte((uint8_t) ((operand >> 8) & 0xff));
    emitByte((uint8_t) ((operand >> 16) & 0xff));
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitLoop(int loopStart) {
    int offset = currentChunk()->count - loopStart + 3;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }
    emitShort(OP_LOOP, offset);
}

static void emitReturn() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static void emitConstant(Value value) {
    writeConstant(currentChunk(), value, parser.previous.line);
}

static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = jump & 0xff;
    currentChunk()->code[offset + 1] = (jump >> 8) & 0xff;
}

static int makeConstant(Value value) {
    ValueArray *globalValues = &buffer.globalVars;
    if (globalValues->count + 1 > UINT16_MAX) {
        error("Too many globalValues in one chunk.");
        return 0;
    }
    writeValueArray(globalValues, value);
    writeValueArray(globalValues, UNDEFINED_VAL);
    return globalValues->count - 1;
}

static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;

    compiler->localCapacity = LOCALS_MIN;
    compiler->locals = ALLOCATE(Local, LOCALS_MIN);
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();

    compiler->loopStart = -1;
    compiler->loopScopeDepth = -1;
    compiler->switchCaseDepth = 0;

    compiler->LoopBreak.top = 0;
    compiler->LoopBreak.count = 0;
    compiler->SwitchBreak.top = 0;
    compiler->SwitchBreak.count = 0;
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = makeString(parser.previous.start, parser.previous.length, false);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
    local->isConst = false;
    local->isCaptured = false;
}

static ObjFunction *endCompiler() {
    emitReturn();
    FREE_ARRAY(Local, current->locals, current->localCapacity);
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name);
    }
#endif
    current = current->enclosing;
    return function;
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    uint16_t popCount = 0;
    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        popCount++;
        current->localCount--;
    }
}

static int identifierConstant(Token *name) {
    Value variable = OBJ_VAL(makeString(name->start, name->length, true));
    Value identifier;

    if (tableGet(&buffer.globalVarIdentifiers, variable, &identifier)) {
        return AS_NUMBER(identifier);
    }

    int newIdentifier = makeConstant(variable);
    tableSet(&buffer.globalVarIdentifiers, variable, NUMBER_VAL(newIdentifier));
    return newIdentifier;
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}


static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(&local->name, name)) {
            if (local->depth == -1) {
                error("Can't read local variable in it's initializer");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler *compiler, int index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, upvalue, false);
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount + 1 >= LOCALS_MAX) {
        error("Too many local variables in the scope.");
        return;
    }

    if (current->localCount + 1 > current->localCapacity) {
        int oldCapacity = current->localCapacity;
        current->localCapacity = GROW_CAPACITY(current->localCapacity);
        current->locals = GROW_ARRAY(Local, current->locals, oldCapacity, current->localCapacity);
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isConst = false;
    local->isCaptured = false;
}

static void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint32_t parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(&parser.previous);
}

static void markInitialized(bool isConst) {
    if (current->scopeDepth == 0) {
        return;
    }
    Local *local = &current->locals[current->localCount - 1];
    local->depth = current->scopeDepth;
    local->isConst = isConst;
}

static void defineVariable(uint32_t global, Token name, bool isConst) {
    if (current->scopeDepth > 0) {
        markInitialized(isConst);
        return;
    }

    if (isConst) {
        tableSet(&buffer.constVarIdentifiers, OBJ_VAL(makeString(name.start, name.length, true)), BOOL_VAL(true));
    }

    emitShort(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
    return argCount;
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        Value val;
        if ((tableGet(&buffer.constVarIdentifiers, OBJ_VAL(makeString(name.start, name.length, true)), &val)) ||
            (arg != -1 && setOp != OP_SET_GLOBAL && current->locals[arg].isConst)) {
            error("Cannot assign to a constant variable.");
            return;
        }

        expression();
        emitShort(setOp, arg);
    } else {
        emitShort(getOp, arg);
    }
}

static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        case TOKEN_MODULO:
            emitByte(OP_MODULO);
            break;
        default:
            return; // Unreachable.
    }
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return; // Unreachable.
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after an expression");
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(makeString(parser.previous.start + 1, parser.previous.length - 2, true)));
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}

static void conditional(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_CONDITIONAL);

    int endJump = emitJump(OP_JUMP);
    consume(TOKEN_COLON, "Expected ':' after '?'.");
    patchJump(elseJump);

    emitByte(OP_POP);
    parsePrecedence(PREC_ASSIGNMENT);
    patchJump(endJump);
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after a block.");
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint32_t constant = parseVariable("Expected parameter name.");
            defineVariable(constant, parser.previous, false);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitLong(OP_CLOSURE, addConstant(currentChunk(), OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void funDeclaration() {
    Token name = parser.current;
    uint32_t global = parseVariable("Expected function name.");
    markInitialized(false);
    function(TYPE_FUNCTION);
    defineVariable(global, name, false);
}

static void varDeclaration(bool isConst) {
    uint32_t global = parseVariable("Expected variable name.");
    Token name = parser.previous;

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        if (isConst) {
            error("Missing value in the const declaration.");
            return;
        }
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

    defineVariable(global, name, isConst);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
    emitByte(OP_POP);
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement() {
    int previousLoopStart = current->loopStart;
    int previousLoopScopeDepth = current->loopScopeDepth;
    current->loopStart = currentChunk()->count;
    current->loopScopeDepth = current->scopeDepth;

    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    int previousCount = current->LoopBreak.count;
    current->LoopBreak.count = 0;
    statement();
    emitLoop(current->loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
    PATCH_BREAK(current->LoopBreak);
    current->loopStart = previousLoopStart;
    current->loopScopeDepth = previousLoopScopeDepth;
    current->LoopBreak.count = previousCount;
}

static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");
    int loopVarSlot = -1;
    Token loopVarName;

    // Initializer
    if (match(TOKEN_SEMICOLON)) {
        // Do nothing.
    } else if (match(TOKEN_VAR)) {
        loopVarName = parser.current;
        varDeclaration(false);
        loopVarSlot = current->localCount - 1;
    } else {
        expressionStatement();
    }

    // Condition clause
    int previousLoopStart = current->loopStart;
    int previousLoopScopeDepth = current->loopScopeDepth;
    current->loopStart = currentChunk()->count;
    current->loopScopeDepth = current->scopeDepth;

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // Increment expression
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

        emitLoop(current->loopStart);
        current->loopStart = incrementStart;
        patchJump(bodyJump);
    }

    int previousCount = current->LoopBreak.count;
    current->LoopBreak.count = 0;

    int loopShadowSlot = -1;
    if (loopVarSlot != -1) {
        beginScope();
        emitShort(OP_GET_LOCAL, loopVarSlot);
        addLocal(loopVarName);
        markInitialized(false);
        loopShadowSlot = current->localCount - 1;
    }

    statement();

    if (loopVarSlot != -1) {
        emitShort(OP_GET_LOCAL, loopShadowSlot);
        emitShort(OP_SET_LOCAL, loopVarSlot);
        emitByte(OP_POP);
        endScope();
    }

    emitLoop(current->loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }

    PATCH_BREAK(current->LoopBreak);
    if (current->LoopBreak.count > 0 && loopVarSlot != -1) {
        emitByte(OP_POP);
    }

    endScope();
    current->loopStart = previousLoopStart;
    current->loopScopeDepth = previousLoopScopeDepth;
    current->LoopBreak.count = previousCount;
}

static void breakStatement() {
#define EMIT_BREAK(breakType) \
    do {                      \
        if (breakType.top + 1 > UINT8_MAX) { \
            error("Too many 'break(s)' in a scope."); \
            return;           \
        }                     \
        breakType.stack[breakType.top++] = emitJump(OP_JUMP); \
        breakType.count++;    \
    } while(false)            \

    consume(TOKEN_SEMICOLON, "Expected ';' after 'break'.");
    if (current->loopStart == -1 && current->switchCaseDepth == 0) {
        error("Unexpected 'break' outside of switch|for|while statements.");
        return;
    }

    if (current->loopStart != -1) {
        EMIT_BREAK(current->LoopBreak);
    } else {
        EMIT_BREAK(current->SwitchBreak);
    }

#undef EMIT_BREAK
}

static void continueStatement() {
    consume(TOKEN_SEMICOLON, "Expected ';' after 'continue'.");
    if (current->loopStart == -1) {
        error("Unexpected 'continue' outside of loop.");
    }
    uint16_t popCount = 0;
    for (int i = current->localCount - 1; i >= 0 && current->locals[i].depth > current->loopScopeDepth; i--) {
        popCount++;
    }

    if (popCount > 0) {
        emitShort(OP_POPN, popCount);
    }
    emitLoop(current->loopStart);
}

static void switchStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'switch'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before 'switch' body.");

    int previousCount = current->SwitchBreak.count;
    current->SwitchBreak.count = 0;

    addLocal(parser.previous);
    markInitialized(false);

    bool defaultCaseCompiled = false;
    int fallthroughJump = -1;
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        if (!match(TOKEN_SWITCH_CASE) && !match(TOKEN_SWITCH_DEFAULT)) {
            advance();
            error("Unexpected keyword inside 'switch' statement.");
        }

        if (defaultCaseCompiled && parser.previous.type == TOKEN_SWITCH_DEFAULT) {
            error("switch statement can only have 1 default case.");
        }
        if (parser.previous.type == TOKEN_SWITCH_DEFAULT) {
            defaultCaseCompiled = true;
            emitByte(OP_TRUE);
        } else {
            emitByte(OP_DUPLICATE);
            expression();
            emitByte(OP_EQUAL);
        }

        consume(TOKEN_COLON, "Expected ':' after switch case.");

        int nextCaseJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        if (fallthroughJump != -1) {
            patchJump(fallthroughJump);
        }

        current->switchCaseDepth++;
        while (!check(TOKEN_SWITCH_CASE) && !check(TOKEN_SWITCH_DEFAULT) && !check(TOKEN_RIGHT_BRACE) &&
               !check(TOKEN_EOF)) {
            statement();
        }
        current->switchCaseDepth--;

        fallthroughJump = emitJump(OP_JUMP);
        patchJump(nextCaseJump);
        emitByte(OP_POP);
    }
    consume(TOKEN_RIGHT_BRACE, "Expected '}' after switch body.");

    if (fallthroughJump != -1) {
        patchJump(fallthroughJump);
    }

    PATCH_BREAK(current->SwitchBreak);
    current->SwitchBreak.count = previousCount;
    endScope();
}

static void declaration() {
    if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR) || match(TOKEN_CONST)) {
        varDeclaration(parser.previous.type == TOKEN_CONST);
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_BREAK)) {
        breakStatement();
    } else if (match(TOKEN_CONTINUE)) {
        continueStatement();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, call, PREC_CALL},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        [TOKEN_MODULO]        = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        [TOKEN_QUESTION]      = {NULL, conditional, PREC_CONDITIONAL},
        [TOKEN_COLON]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, and_, PREC_AND},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, or_, PREC_OR},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_CONST]         = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_BREAK]         = {NULL, NULL, PREC_NONE},
        [TOKEN_CONTINUE]      = {NULL, NULL, PREC_NONE},
        [TOKEN_SWITCH]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SWITCH_CASE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expected an expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}

ObjFunction *compile(const char *source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFunction *function = endCompiler();
    return parser.hadError ? NULL : function;
}
