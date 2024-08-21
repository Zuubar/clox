#include "common.h"
#include "../src/compiler.h"
#include "unity_internals.h"

void testFunctions() {
    const char *program1 = "fun sum(a, b) {"
                           "   return a + b;"
                           "}"
                           "print sum(17, 3);"
                           "print sum(17, 3) + sum(10, 5);";

    const char *program2 = "fun fib(n) {"
                           "    if (n <= 1) return n;"
                           "    return fib(n - 2) + fib(n - 1);"
                           "}"
                           "print fib(5);";

    const char *program3 = "{"
                           "    var a = 1;"
                           "    fun f() {"
                           "        print a;"
                           "    }"
                           "    var b = 2;"
                           "    fun g() {"
                           "        print b;"
                           "    }"
                           "    var c = 3;"
                           "    fun h() {"
                           "        print c;"
                           "    }"
                           "    f(); g(); h();"
                           "}";

    const char *program4 = "fun adder() {"
                           "    var i = 0;"
                           "    fun closure(x) {"
                           "        i = i + x;"
                           "        print i;"
                           "    }"
                           "    return closure;"
                           "}"
                           "var pos = adder();"
                           "var neg = adder();"
                           "for (var i = 0; i < 5; i = i + 1) {"
                           "    pos(1);"
                           "    neg(-1);"
                           "}";

    const char *cases[][2] = {
            {program1, "20\n35\n"},
            {program2, "5\n"},
            {program3, "1\n2\n3\n"},
            {program4, "1\n-1\n2\n-2\n3\n-3\n4\n-4\n5\n-5\n"},
    };
    TEST_PROGRAMS(cases);
}

void testNativeStrFunction() {
    const char *program1 = "fun fib(n) {"
                           "    if (n < 2) {"
                           "        return n;"
                           "    }"
                           "    return fib(n - 2) + fib(n - 1);"
                           "}"
                           "fun QDkwKxRmhgZhrwnMnOzjkgVHmfxVbboRVhawfCMQjcpVDFnAlNjuYBADQFX() {"
                           "    return nil;"
                           "}"
                           "fun very_very_very_very_very_very_very_very_very_very_long_function() {"
                           "    return nil;"
                           "}"
                           "class Doughnut {"
                           "    cook() {"
                           "        return \"Fry until golden brown.\" + \"Place in a nice box.\";"
                           "    }"
                           "}"
                           "print str(false);"
                           "print str(true);"
                           "print str(nil);"
                           "print str(128);"
                           "print str(99999999999999999999999999999999999999999999999999999999999999 * 99999999999999999999999999999999999999999999999999999999999999);"
                           "print str(QDkwKxRmhgZhrwnMnOzjkgVHmfxVbboRVhawfCMQjcpVDFnAlNjuYBADQFX);"
                           "print str(str(str(very_very_very_very_very_very_very_very_very_very_long_function)));"
                           "print str(str(fib));"
                           "print str(str(clock));"
                           "print str(Doughnut);"
                           "print str(Doughnut());"
                           "print str(Doughnut().cook);";

    const char *cases[][2] = {
            {program1,
             "false\ntrue\nnil\n128\n1e+124\n<fn QDkwKxRmhgZhrwnMnOzjkgVHmfxVbboRVhawfCMQjcpVDFnAlNjuYBADQFX\n<fn very_very_very_very_very_very_very_very_very_very_long_func\n<fn fib>\n<native fn>\nDoughnut\nDoughnut instance\n<fn cook>\n"},
    };
    TEST_PROGRAMS(cases);
}

void setUp() {

}

void tearDown() {

}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testFunctions);
    RUN_TEST(testNativeStrFunction);
    return UNITY_END();
}