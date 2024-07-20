#include "common.h"
#include "unity.h"

void testArithmeticExpressions() {
    const char *cases[][2] = {
            {"5 + 5",       "10"},
            {"5 - 3",       "2"},
            {"-3",          "-3"},
            {"4 * 2",       "8"},
            {"8 / 2",       "4"},
            {"5 + 2 * 3",   "11"},
            {"(5 + 2) * 3", "21"},
            {"5 + 30 / 3",  "15"},
            {"(5 + 3) / 2", "4"},
            {"100 % 19",    "5"},
            {"13 + 15 % 6", "16"},
    };
    TEST_EXPRESSIONS(cases);
}

void testBooleanExpressions() {
    const char *cases[][2] = {
            {"5 > 5",                                    "false"},
            {"5 >= 5",                                   "true"},
            {"7 > 5",                                    "true"},
            {"11 < 11",                                  "false"},
            {"11 <= 11",                                 "true"},
            {"11 <= 11",                                 "true"},
            {"13 != 11",                                 "true"},
            {"13 != 13",                                 "false"},
            {"17 == 13",                                 "false"},
            {"17 == 17",                                 "true"},
            {"nil == nil",                               "true"},
            {"nil or 19",                                "19"},
            {"nil or 19 or false",                       "19"},
            {"nil and 19",                               "nil"},
            {"23 and 19 and 17",                         "17"},
            {"23 or nil or 27",                          "23"},
            {"23 and nil and 29",                        "nil"},
            {"true ? 1 : 0",                             "1"},
            {"29 > 31 ? false : 31 > 29 ? true : false", "true"},
    };
    TEST_EXPRESSIONS(cases);
}

void testStringExpressions() {
    const char *cases[][2] = {
            {"\"\"",                                                   ""},
            {"\"string\"",                                             "string"},
            {"\"str\" + \"ing\"",                                      "string"},
            {"\"st\" + \"ri\" + \"ng\"",                               "string"},
            {"\"s\" + \"t\" + \"r\" + \"i\" + \"n\" + \"g\"",          "string"},
            {"\"clox\" + \" \" + \"-\" + \" \" + \"superset of lox\"", "clox - superset of lox"},

    };
    TEST_EXPRESSIONS(cases);
}

void setUp() {

}

void tearDown() {

}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testArithmeticExpressions);
    RUN_TEST(testBooleanExpressions);
    RUN_TEST(testStringExpressions);
    return UNITY_END();
}