#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

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

#endif //COMMON_H