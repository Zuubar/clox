#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "../src/compiler.h"
#include "unity.h"

static bool interpretTest(const char *source, char *buffer, int buffLen) {
    fflush(stdout);
    fflush(stderr);
    int outPipedes[2];
    int errPipedes[2];
    int outFd = dup(STDOUT_FILENO);
    int errFd = dup(STDERR_FILENO);

    if (pipe(outPipedes) == -1 || pipe(errPipedes) == -1) {
        printf("Failed to create pipe.");
        exit(127);
    }

    dup2(outPipedes[1], STDOUT_FILENO);
    dup2(errPipedes[1], STDERR_FILENO);
    close(outPipedes[1]);
    close(errPipedes[1]);

    initVM();
    InterpretResult result = interpret(source);
    freeVM();

    fflush(stdout);
    fflush(stderr);

    dup2(outFd, STDOUT_FILENO);
    dup2(errFd, STDERR_FILENO);

    if (result != INTERPRET_OK) {
        read(errPipedes[0], buffer, sizeof(char) * buffLen);
        return false;
    }

    read(outPipedes[0], buffer, sizeof(char) * buffLen);

    return true;
}

void testExpressions(const char *cases[][2], int length) {
    for (int i = 0; i < length; i++) {
        char buffer[256] = {0}, testCase[256] = {0}, testExpected[256] = {0}, testError[256] = {0};

        snprintf(testCase, sizeof(testCase) / sizeof(testCase[0]), "print (%s);", cases[i][0]);
        snprintf(testExpected, sizeof(testExpected) / sizeof(testExpected[0]), "%s\n", cases[i][1]);
        snprintf(testError, sizeof(testError) / sizeof(testError[0]), "Test #%d", i);

        interpretTest(testCase, buffer, (sizeof(buffer) / sizeof(buffer[0])) - 1);
        UNITY_TEST_ASSERT_EQUAL_STRING(testExpected, buffer, __LINE__, testError);
    }
}

void testPrograms(const char *cases[][2], int length) {
    for (int i = 0; i < length; i++) {
        char buffer[2048] = {0}, testError[256] = {0};

        snprintf(testError, (sizeof(testError) / sizeof(testError[0])) - 1, "Test #%d", i);

        interpretTest(cases[i][0], buffer, (sizeof(buffer) / sizeof(buffer[0])) - 1);
        UNITY_TEST_ASSERT_EQUAL_STRING(cases[i][1], buffer, __LINE__, testError);
    }
}
