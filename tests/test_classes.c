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
                           "print pair.first + pair.second;";

    const char *program2 = "class Pair {}"
                           "var pair = Pair();"
                           "var f = \"fir\" + \"st\";"
                           "var s = \"second\";"
                           "setField(pair, f, 1);"
                           "setField(pair, s, 2);"
                           "print pair.first + pair.second;";


    const char *program3 = "{"
                           "    class Pair {}"
                           "    var pair = Pair();"
                           "    pair.first = 1;"
                           "    pair.second = 2;"
                           "    setField(pair, \"thi\", 3);"
                           "    print getField(pair, \"first\") + pair.second + pair.thi;"
                           "}";

    const char *program4 = "class Rectangle {"
                           "    init(width, height) {"
                           "        this.width = width;"
                           "        this.height = height;"
                           "    }"
                           "    area() {"
                           "        return this.width * this.height;"
                           "    }"
                           "}"

                           "var rectangle = Rectangle(7, 8);"
                           "var square = Rectangle(9, 9);"
                           "print rectangle.area();"
                           "print square.area();";

    const char *cases[][2] = {
            {program1, "3\n"},
            {program2, "3\n"},
            {program3, "6\n"},
            {program4, "56\n81\n"},
    };
    TEST_PROGRAMS(cases);
}

void testClassThis() {
    const char *program1 = "class Person {"
                           "    sayName() {"
                           "        print this.name;"
                           "    }"
                           "    saySurname() {"
                           "        print this.surname;"
                           "    }"
                           "}"
                           "var hank = Person();"
                           "hank.name = \"Hank\";"
                           "hank.surname = \"Schrader\";"
                           "var refSurname = hank.saySurname;"
                           "hank.sayName();"
                           "refSurname();";

    const char *program2 = "class Person {"
                           "    sayName() {"
                           "        print this.name;"
                           "    }"
                           "    saySurname() {"
                           "        print this.surname;"
                           "    }"
                           "}"
                           "var jane = Person();"
                           "jane.name = \"Jane\";"
                           "jane.sayName();"
                           "var bill = Person();"
                           "bill.name = \"Bill\";"
                           "bill.sayName();"
                           "bill.sayName = jane.sayName;"
                           "bill.sayName();";

    const char *cases[][2] = {
            {program1, "Hank\nSchrader\n"},
            {program2, "Jane\nBill\nJane\n"},
    };

    TEST_PROGRAMS(cases);
}

void testClassMethod() {
    const char *program1 = "class CoffeeMaker {"
                           "  init(coffee) {"
                           "    this.coffee = coffee;"
                           "  }"
                           "  brew() {"
                           "    print \"Enjoy your cup of \" + this.coffee;"
                           "  }"
                           "  withMuffin() {"
                           "    this.brew();"
                           "    print \"and with muffin\";"
                           "  }"
                           "}"
                           "var maker = CoffeeMaker(\"coffee and chicory\");"
                           "maker.brew();"
                           "maker.withMuffin();";

    const char *program2 = "class Parser {"
                           "    checkGrammar() {"
                           "        return true;"
                           "    }"
                           "    parse() {"
                           "        print \"parsing... Done\";"
                           "    }"
                           "}"
                           "var parser = Parser();"
                           "parser.tokens = \"{NUMBER}{PLUS}{NUMBER}\";"
                           "parser.tokens_eof = parser.tokens + \"{EOF}\";"
                           "print parser.checkGrammar();"
                           "parser.parse();"
                           "print parser.tokens;"
                           "print parser.tokens_eof;";

    const char *cases[][2] = {
            {program1, "Enjoy your cup of coffee and chicory\nEnjoy your cup of coffee and chicory\nand with muffin\n"},
            {program2, "true\nparsing... Done\n{NUMBER}{PLUS}{NUMBER}\n{NUMBER}{PLUS}{NUMBER}{EOF}\n"},
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
    RUN_TEST(testClassFields);
    RUN_TEST(testClassThis);
    RUN_TEST(testClassMethod);
    return UNITY_END();
}