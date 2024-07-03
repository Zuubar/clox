#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

#define TEST_EXPRESSIONS(cases) \
    do {                        \
        testExpressions(cases, sizeof(cases) / sizeof(cases[0])); \
    } while(false)              \

#define TEST_PROGRAMS(cases) \
    do {                     \
        testPrograms(cases, sizeof(cases) / sizeof(cases[0])); \
    } while(false)           \


bool interpretTest(const char *source, char *buffer, int buffLen);

void testExpressions(const char *cases[][2], int length);

void testPrograms(const char *cases[][2], int length);

void testBooleanExpressions();

void testArithmeticExpressions();

void testGlobalVariables();

void testAssignment();

void testScope();

void testIfStatement();

void testWhileStatement();

void testForStatement();

void testSwitchStatement();

void testFunctions();

void testNativeStrFunction();

#endif //COMMON_H