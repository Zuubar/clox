#include "common.h"
#include "unity.h"

void testClassDeclaration() {
    const char *program1 = "{"
                           "    class Pair {}"
                           "    print Pair;"
                           "    print Pair();"
                           "}"
                           "class Pair {}"
                           "print Pair;"
                           "print Pair();";

    const char *cases[][2] = {
            {program1, "Pair\nPair instance\nPair\nPair instance\n"},
    };
    TEST_PROGRAMS(cases);
}

void testClassFields() {
    const char *program1 = "class Pair {}"
                           "var pair = Pair();"
                           "pair.first = 1;"
                           "pair.second = 2;"
                           "print pair.first + pair.second";

    const char *program2 = "class Pair {}"
                           "var pair = Pair();"
                           "var f = \"fir\" + \"st\";"
                           "var s = \"second\";"
                           "setField(pair, f, 1);"
                           "setField(pair, s, 2);"
                           "print pair.first + pair.second";


    const char *program3 = "{"
                           "    class Pair {}"
                           "    var pair = Pair();"
                           "    pair.first = 1;"
                           "    pair.second = 2;"
                           "    setField(pair, \"thi\", 3);"
                           "    print getField(pair, \"first\") + pair.second + pair.thi;"
                           "}";

    const char *cases[][2] = {
            {program1, "3\n"},
            {program2, "3\n"},
            {program3, "6\n"},
    };
    TEST_PROGRAMS(cases);
}

void setUp() {

}

void tearDown() {

}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testClassDeclaration);
    return UNITY_END();
}