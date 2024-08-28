#include "common.h"
#include "unity.h"

void testArrays() {
    const char *program1 =
            "var empty = [];"
            "print empty;"
            "var primes = [2, 3, 5, 7, 11];"
            "print primes[2];"
            "print primes;"
            "var byte = [0, 0, 0, 0, 0, 0, 0, 0];"
            "byte[7] = 1;"
            "byte[4] = 1;"
            "byte[0] = 1;"
            "var result = 0;"
            "for (var i = 0; i < 8; i = i + 1) {"
            "   if (byte[(7 - i)] == 0) {"
            "       continue;"
            "   }"
            "   var power = 1;"
            "   for (var j = 0; j < i; j = j + 1) {"
            "       power = power * 2;"
            "   }"
            "   result = result + power;"
            "}"
            "print \"Binary\" + str(byte) + \" == \" + str(result);";

    const char *program2 =
            "var matrix = [];"
            "for (var i = 1; i < 5; i = i + 1) {"
            "   var sub = [];"
            "   for (var j = 2 * i; j < 32 * i; j = j * 2) {"
            "       append(sub, j);"
            "   }"
            "   append(matrix, sub);"
            "}"
            "print matrix;"
            "print str(matrix[0][0]) + \", \" + str(matrix[1][1]) + \", \" + str(matrix[2][2]) + \", \" + str(matrix[3][3]);";
    const char *cases[][2] = {
            {program1, "[]\n5\n[2, 3, 5, 7, 11]\nBinary[1, 0, 0, 0, 1, 0, 0, 1] == 137\n"},
            {program2, "[[2, 4, 8, 16], [4, 8, 16, 32], [6, 12, 24, 48], [8, 16, 32, 64]]\n2, 8, 24, 64\n"},
    };
    TEST_PROGRAMS(cases);
}

void setUp() {

}

void tearDown() {

}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testArrays);
    return UNITY_END();
}