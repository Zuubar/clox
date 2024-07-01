#include "common.h"
#include "unity.h"
#include "../src/compiler.h"

typedef void (*TestFn)();

void setUp() {
    initVM();
}

void tearDown() {
    freeVM();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testArithmeticExpressions);
    RUN_TEST(testBooleanExpressions);
    RUN_TEST(testGlobalVariables);
    RUN_TEST(testAssignment);
    RUN_TEST(testScope);
    RUN_TEST(testIfStatement);
    RUN_TEST(testWhileStatement);
    RUN_TEST(testForStatement);
    return UNITY_END();
}