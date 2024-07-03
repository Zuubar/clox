#include "common.h"
#include "unity.h"
#include "../src/compiler.h"

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
    RUN_TEST(testSwitchStatement);
    RUN_TEST(testFunctions);
    RUN_TEST(testNativeStrFunction);
    return UNITY_END();
}