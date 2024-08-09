#include "common.h"
#include "unity.h"

void testIfStatement() {
    const char *program1 = "var num = 32;"
                           "if (num % 2 == 0) {"
                           "    num = num / 16;"
                           "}"
                           "print num;"
                           "var prime = 15;"
                           "if (prime % 2 == 0) {"
                           "    prime = prime / 2;"
                           "} else {"
                           "    prime = prime / 3;"
                           "}"
                           "print prime;";

    const char *program2 = "var num = 3;"
                           "if (num == 0) {"
                           "    print 0;"
                           "} else if (num == 1) {"
                           "    print 1;"
                           "} else if (num == 2) {"
                           "    print 2;"
                           "} else {"
                           "    print 3;"
                           "}";

    const char *cases[][2] = {
            {program1, "2\n5\n"},
            {program2, "3\n"},
    };
    TEST_PROGRAMS(cases);
}

void testWhileStatement() {
    const char *program1 = "var i = 0;"
                           "while (i < 10) {"
                           "    i = i + 1;"
                           "}"
                           "print i;";

    const char *program2 = "var a = 0;"
                           "var b = 1;"
                           "while (true) {"
                           "    if (a > 200) {"
                           "        break;"
                           "    }"
                           "    var c = a + b;"
                           "    a = b;"
                           "    b = c;"
                           "}"
                           "print a;";

    const char *program3 = "var i = 0;"
                           "while (true) {"
                           "    i = i + 1;"
                           "    if (i >= 5 and i < 10) {"
                           "        print \"Finished for real\";"
                           "        continue;"
                           "    }"
                           "    if (i == 10) {"
                           "        print \"Finished for real now\";"
                           "        break;"
                           "    }"
                           "    while (true) {"
                           "        if (i > 5) {"
                           "            print \"Finished\";"
                           "            break;"
                           "        }"
                           "        i = i + 1;"
                           "    }"
                           "}";

    const char *program4 = "var i = -1;"
                           "while (i < 5) {"
                           "    i = i + 1;"
                           "    var a = 1; var b = 2; var c = 3; var d = 4;"
                           "    if (i == 2) {"
                           "        continue;"
                           "    }"
                           "    if (i == 3) {"
                           "        continue;"
                           "    }"
                           "    var j = -1;"
                           "    while (j < 10) {"
                           "        j = j + 1;"
                           "        var a = 5; var b = 6; var c = 7; var d = 8;"
                           "        if (j == 5) {"
                           "            continue;"
                           "        }"
                           "        if (j == 3) {"
                           "            continue;"
                           "        }"
                           "        if (j == 7) {"
                           "            continue;"
                           "        }"
                           "        print \"Inner\";"
                           "        print a * b * c * d * j;"
                           "    }"
                           "    print \"Outer\";"
                           "    print a * b * c * d * i;"
                           "}";

    const char *cases[][2] = {
            {program1, "10\n"},
            {program2, "233\n"},
            {program3, "Finished\nFinished for real\nFinished for real\nFinished for real\nFinished for real now\n"},
            {program4, "Inner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nInner\n16800\nOuter\n0\nInner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nInner\n16800\nOuter\n24\nInner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nInner\n16800\nOuter\n96\nInner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nInner\n16800\nOuter\n120\n"},
    };
    TEST_PROGRAMS(cases);
}

void testForStatement() {
    const char *program1 = "for (var i = 0; i < 2; i = i + 1) {"
                           "    for (var j = 0; j < 2; j = j + 1) {"
                           "        print \"[\" + str(i) + \"]\" + \"[\" + str(j) + \"]\";"
                           "    }"
                           "}";

    const char *program2 = "var i = 1;"
                           "for (;;) {"
                           "    if (i == 32) {"
                           "        break;"
                           "    }"
                           "    i = i * 2;"
                           "}"
                           "print i;"
                           "for (var i = 1; ;) {"
                           "    if (i == 32) {"
                           "        break;"
                           "    }"
                           "    i = i * 2;"
                           "}"
                           "print i;"
                           "for (var i = 1; i < 32;) {"
                           "    i = i * 2;"
                           "}"
                           "print i;"
                           "for (var i = 1; i < 32; i = i * 2) {}"
                           "print i;";

    const char *program3 = "for (var i = 0; i < 5; i = i + 1) {"
                           "    var a = 1; var b = 2; var c = 3; var d = 4;"
                           "    if (i == 2) {"
                           "        continue;"
                           "    }"
                           "    if (i == 3) {"
                           "        continue;"
                           "    }"
                           "    for (var j = 0; j < 10; j = j + 1) {"
                           "        var a = 5; var b = 6; var c = 7; var d = 8;"
                           "        if (j == 5) {"
                           "            continue;"
                           "        }"
                           "        if (j == 3) {"
                           "            continue;"
                           "        }"
                           "        if (j == 7) {"
                           "            continue;"
                           "        }"
                           "        print \"Inner\";"
                           "        print a * b * c * d * j;"
                           "    }"
                           "    print \"Outer\";"
                           "    print a * b * c * d * i;"
                           "}";

    const char *program4 = "var globalOne;"
                           "var globalTwo;"
                           "fun main() {"
                           "    for (var a = 1; a <= 2; a = a + 1) {"
                           "        fun closure() {"
                           "            print a;"
                           "        }"
                           "        if (globalOne == nil) {"
                           "            globalOne = closure;"
                           "        } else {"
                           "            globalTwo = closure;"
                           "        }"
                           "    }"
                           "}"
                           "main();"
                           "globalOne();"
                           "globalTwo();";

    const char *cases[][2] = {
            {program1, "[0][0]\n[0][1]\n[1][0]\n[1][1]\n"},
            {program2, "32\n32\n32\n32\n"},
            {program3, "Inner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nOuter\n0\nInner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nOuter\n24\nInner\n0\nInner\n1680\nInner\n3360\nInner\n6720\nInner\n10080\nInner\n13440\nInner\n15120\nOuter\n96\n"},
            {program4, "1\n2\n"},
    };
    TEST_PROGRAMS(cases);
}

void testSwitchStatement() {
    const char *program1 = "var a = 1;"
                           "switch(a) {"
                           "    case 1:"
                           "        print 1;"
                           "    case 2:"
                           "        print 2;"
                           "    case 3:"
                           "        print 3;"
                           "        break;"
                           "    default:"
                           "        print \"default\";"
                           "}"
                           "a = 5;"
                           "switch(a) {"
                           "    case 1:"
                           "        print 1;"
                           "        break;"
                           "    case 2:"
                           "        print 2;"
                           "        break;"
                           "    case 3:"
                           "        print 3;"
                           "        break;"
                           "    default:"
                           "        print \"default\";"
                           "}";

    const char *program2 = "var a = 2;"
                           "switch(a) {"
                           "    case 1:"
                           "        print 1;"
                           "        break;"
                           "    case 2:"
                           "        print 2;"
                           "        for (var i = 2; i < 10; i = i + 1) {"
                           "            if (i == 4) {"
                           "                continue;"
                           "            }"
                           "            if (i == 5) {"
                           "                break;"
                           "            }"
                           "            for (var j = 2; j < 6; j = j + 1) {"
                           "                if (j == 5) {"
                           "                    break;"
                           "                }"
                           "                print \"J\";"
                           "                print j;"
                           "            }"
                           "            print \"I\";"
                           "            print i;"
                           "        }"
                           "    case 3:"
                           "        print 3;"
                           "        for (var i = 3; i < 10; i = i + 1) {"
                           "            if (i == 4) {"
                           "                continue;"
                           "            }"
                           "            if (i == 5) {"
                           "                break;"
                           "            }"
                           "            for (var j = 3; j < 6; j = j + 1) {"
                           "                if (j == 5) {"
                           "                    break;"
                           "                }"
                           "                print \"J\";"
                           "                print j;"
                           "            }"
                           "            print \"I\";"
                           "            print i;"
                           "        }"
                           "    default:"
                           "        print \"default\";"
                           "        switch (a) {"
                           "            case 4:"
                           "                for (var i = 3; i < 10; i = i + 1) {"
                           "                    if (i == 4) {"
                           "                        continue;"
                           "                    }"
                           "                    if (i == 5) {"
                           "                        break;"
                           "                    }"
                           "                    for (var j = 3; j < 6; j = j + 1) {"
                           "                        if (j == 5) {"
                           "                            break;"
                           "                        }"
                           "                        print \"J\";"
                           "                        print j;"
                           "                    }"
                           "                    print \"I\";"
                           "                    print i;"
                           "                }"
                           "        }"
                           "}";

    const char *cases[][2] = {
            {program1, "1\n2\n3\ndefault\n"},
            {program2, "2\nJ\n2\nJ\n3\nJ\n4\nI\n2\nJ\n2\nJ\n3\nJ\n4\nI\n3\n3\nJ\n3\nJ\n4\nI\n3\ndefault\n"},
    };
    TEST_PROGRAMS(cases);
}

void setUp() {

}

void tearDown() {

}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testIfStatement);
    RUN_TEST(testWhileStatement);
    RUN_TEST(testForStatement);
    RUN_TEST(testSwitchStatement);
    return UNITY_END();
}
