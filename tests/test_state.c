#include "common.h"
#include "unity.h"
#include "../src/compiler.h"

void testGlobalVariables() {
    const char *program1 = "var v = \"global\";"
                           "print v;"
                           "{"
                           "    print v;"
                           "{"
                           "    print v;"
                           "}"
                           "}"
                           "print v;";

    const char *program2 = "var v = \"global\";"
                           "print v;"
                           "{"
                           "    print v;"
                           "    v = \"inner\";"
                           "    {"
                           "        print v;"
                           "    }"
                           "}"
                           "print v;";

    const char *cases[][2] = {
            {program1, "global\nglobal\nglobal\nglobal\n"},
            {program2, "global\nglobal\ninner\ninner\n"}
    };
    testPrograms(cases, sizeof(cases) / sizeof(cases[0]));
}

void testAssignment() {
    const char *program1 = "var v = 2;"
                           "print v;"
                           "v = v * 2;"
                           "print v;"
                           "v = v * 2;"
                           "print v;"
                           "var v2 = v * 3;"
                           "v = v2;"
                           "print v;"
                           "print v2;";

    const char *program2 = "{"
                           "    var v = 2;"
                           "    print v;"
                           "    v = v * 2;"
                           "    print v;"
                           "    var v2 = v * 3;"
                           "    v = v2;"
                           "    print v;"
                           "    print v2;"
                           "}";

    const char *cases[][2] = {
            {program1, "2\n4\n8\n24\n24\n"},
            {program2, "2\n4\n12\n12\n"},
    };
    testPrograms(cases, sizeof(cases) / sizeof(cases[0]));
}

void testScope() {
    const char *program1 = "var volume = 11;"
                           "volume = 0;"
                           "{"
                           "    var volume = 3 * 4 * 5;"
                           "    print volume;"
                           "}"
                           "print volume;";

    const char *program2 = "var a = \"global a\";"
                           "var b = \"global b\";"
                           "var c = \"global c\";"
                           "{"
                           "    var a = \"outer a\";"
                           "    var b = \"outer b\";"
                           "   {"
                           "       var a = \"inner a\";"
                           "       print a;"
                           "       print b;"
                           "       print c;"
                           "   }"
                           "   print a;"
                           "   print b;"
                           "   print c;"
                           "}"
                           "print a;"
                           "print b;"
                           "print c;";

    const char *cases[][2] = {
            {program1, "60\n0\n"},
            {program2, "inner a\nouter b\nglobal c\nouter a\nouter b\nglobal c\nglobal a\nglobal b\nglobal c\n"},
    };
    testPrograms(cases, sizeof(cases) / sizeof(cases[0]));
}

void setUp() {
    initVM();
}

void tearDown() {
    freeVM();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testGlobalVariables);
    RUN_TEST(testAssignment);
    RUN_TEST(testScope);
    return UNITY_END();
}
