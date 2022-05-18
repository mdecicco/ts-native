#include "../catch.hpp"

#include <gs/utils/ProgramSource.h>
#include <gs/compiler/lexer.h>

#include <utils/Array.hpp>

using namespace gs;
using namespace compiler;
using namespace utils;

TEST_CASE("Lexer Strings", "[lexer]") {
    SECTION("Normal strings") {
        const char* testSource =
            "'test' \"test\" `test`"
        ;
        ProgramSource src = ProgramSource("Strings.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 3);
        REQUIRE(out[0].tp == tt_string);
        REQUIRE(out[0].text == "test");
        REQUIRE(out[1].tp == tt_string);
        REQUIRE(out[1].text == "test");
        REQUIRE(out[2].tp == tt_template_string);
        REQUIRE(out[2].text == "test");
    }
    
    SECTION("Strings with escaped quotes") {
        const char* testSource =
            "'te\\'st' \"te\\\"st\" `te\\`st`"
        ;
        ProgramSource src = ProgramSource("EscapedStrings.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 3);
        REQUIRE(out[0].tp == tt_string);
        REQUIRE(out[0].text == "te'st");
        REQUIRE(out[1].tp == tt_string);
        REQUIRE(out[1].text == "te\"st");
        REQUIRE(out[2].tp == tt_template_string);
        REQUIRE(out[2].text == "te`st");
    }
}

TEST_CASE("Lexer numbers", "[lexer]") {
    SECTION("Integers") {
        const char* testSource =
            "12345 1 0 -1 -12345"
        ;
        ProgramSource src = ProgramSource("Strings.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 5);
        REQUIRE(out[0].tp == tt_number);
        REQUIRE(out[0].text == "12345");
        REQUIRE(out[1].tp == tt_number);
        REQUIRE(out[1].text == "1");
        REQUIRE(out[2].tp == tt_number);
        REQUIRE(out[2].text == "0");
        REQUIRE(out[3].tp == tt_number);
        REQUIRE(out[3].text == "-1");
        REQUIRE(out[4].tp == tt_number);
        REQUIRE(out[4].text == "-12345");
    }

    SECTION("Integers with suffixes") {
        const char* testSource =
            "1b 1B 1s 1S\n"
            "1ub 1UB 1uB 1Ub\n"
            "1us 1US 1uS 1Us\n"
            "1ul 1UL 1uL 1Ul\n"
            "1Ull 1ULl 1ULL 1UlL\n"
            "1ull 1uLl 1uLL 1ulL\n"
        ;
        ProgramSource src = ProgramSource("Strings.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 48);
        u32 i = 0;
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "b");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "B");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "s");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "S");
        
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ub");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "UB");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "uB");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "Ub");
        
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "us");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "US");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "uS");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "Us");
        
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ul");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "UL");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "uL");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "Ul");
        
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "Ull");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ULl");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ULL");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "UlL");
        
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ull");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "uLl");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "uLL");
        REQUIRE(out[i  ].tp   == tt_number);
        REQUIRE(out[i++].text == "1");
        REQUIRE(out[i  ].tp   == tt_number_suffix);
        REQUIRE(out[i++].text == "ulL");
    }

    SECTION("Floating point") {
        const char* testSource =
            "12345. 1.0 0.4532. -0.1 -123.45"
        ;
        ProgramSource src = ProgramSource("Strings.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 6);
        REQUIRE(out[0].tp == tt_number);
        REQUIRE(out[0].text == "12345.");
        REQUIRE(out[1].tp == tt_number);
        REQUIRE(out[1].text == "1.0");
        REQUIRE(out[2].tp == tt_number);
        REQUIRE(out[2].text == "0.4532");
        REQUIRE(out[3].tp == tt_dot);
        REQUIRE(out[3].text == ".");
        REQUIRE(out[4].tp == tt_number);
        REQUIRE(out[4].text == "-0.1");
        REQUIRE(out[5].tp == tt_number);
        REQUIRE(out[5].text == "-123.45");
    }
}

// todo: keywords, identifiers, symbols