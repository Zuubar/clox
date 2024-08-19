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

    const char *program5 = "class Oops {"
                           "  init() {"
                           "    fun f() {"
                           "      print \"not a method\";"
                           "    }"
                           "    this.field = f;"
                           "  }"
                           "}"
                           "var oops = Oops();"
                           "oops.field();";

    const char *cases[][2] = {
            {program1, "3\n"},
            {program2, "3\n"},
            {program3, "6\n"},
            {program4, "56\n81\n"},
            {program5, "not a method\n"},
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

void testClassInheritance() {
    const char *program1 =
            "class Doughnut {"
            "   cook() {"
            "       print \"Fry until golden brown.\";"
            "   }"
            "}"
            "class BostonCream < Doughnut {"
            "   cook() {"
            "       super.cook();"
            "       print \"Pipe full of custard and coat with chocolate.\";"
            "   }"
            "}"
            "BostonCream().cook();";

    const char *program2 =
            "class Car {"
            "   init(name, cc, weight) {"
            "       this.name = name;"
            "       this.cc = cc;"
            "       this.weight = weight;"
            "   }"
            "   wheels() {"
            "       return 4;"
            "   }"
            "   description() {"
            "       return \"{\" + this.name + \", \" + str(this.cc) + \", \" + str(this.weight) + \", \" + str(this.wheels()) + \"}\";"
            "   }"
            "}"
            "class Semi < Car {"
            "   init(name, cc, weight, auxiliary_wheels) {"
            "       super.init(name, cc, weight);"
            "       this.auxiliary_wheels = auxiliary_wheels;"
            "   }"
            "   wheels() {"
            "       return super.wheels() + this.auxiliary_wheels;"
            "   }"
            "   description() {"
            "       return \"Base car: \" + super.description() + \" with base \" + str(super.wheels()) + \" + \" + str(this.auxiliary_wheels) + \" wheels\";"
            "   }"
            "}"
            "var supra = Car(\"Toyota supra\", 2997, 1615);"
            "print supra.description();"
            "var scania = Semi(\"Scania S\", 16000, 9705, 6);"
            "print scania.description();"
            "print scania.wheels();";

    const char *cases[][2] = {
            {program1, "Fry until golden brown.\nPipe full of custard and coat with chocolate.\n"},
            {program2, "{Toyota supra, 2997, 1615, 4}\nBase car: {Scania S, 16000, 9705, 10} with base 4 + 6 wheels\n10\n"},
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
    RUN_TEST(testClassInheritance);
    return UNITY_END();
}