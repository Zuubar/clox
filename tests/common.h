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


void testExpressions(const char *cases[][2], int length);

void testPrograms(const char *cases[][2], int length);

#endif //COMMON_H