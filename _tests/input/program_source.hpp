#include "../catch.hpp"

#include <tsn/utils/ModuleSource.h>
using namespace tsn;

TEST_CASE("ModuleSource", "[input]") {
    SECTION("Input has muiltiple lines") {
        const char* testSource =
            "import { nonsense } from 'notreal';\n\r"
            "\r\n"
            "void test() {\n\r"
            "    nonsense(1, 2, 3);\r\n"
            "}"
        ;
        ModuleSource src = ModuleSource("LineSplit.tsn", testSource);

        REQUIRE(src.getLineCount() == 5);

        const auto& lines = src.getLines();
        REQUIRE(lines[0] == "import { nonsense } from 'notreal';\n\r");
        REQUIRE(lines[1] == "\r\n");
        REQUIRE(lines[2] == "void test() {\n\r");
        REQUIRE(lines[3] == "    nonsense(1, 2, 3);\r\n");
        REQUIRE(lines[4] == "}");
    }
}