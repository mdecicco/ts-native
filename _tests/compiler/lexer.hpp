#include "../catch.hpp"

#include <gs/utils/ProgramSource.h>
#include <gs/compiler/Lexer.h>

#include <utils/Array.hpp>

using namespace gs;
using namespace compiler;
using namespace utils;

TEST_CASE("Lexer", "[lexer]") {
    SECTION("Lexer Strings") {
        SECTION("Normal strings") {
            const char* testSource =
                "'test' \"test\" `test`"
            ;
            ProgramSource src = ProgramSource("Strings.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 4);
            REQUIRE(out[0].tp == tt_string);
            REQUIRE(out[0].text == "test");
            REQUIRE(out[1].tp == tt_string);
            REQUIRE(out[1].text == "test");
            REQUIRE(out[2].tp == tt_template_string);
            REQUIRE(out[2].text == "test");
            REQUIRE(out[3].tp == tt_eof);
        }
        
        SECTION("Strings with escaped quotes") {
            const char* testSource =
                "'te\\'st' \"te\\\"st\" `te\\`st`"
            ;
            ProgramSource src = ProgramSource("EscapedStrings.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 4);
            REQUIRE(out[0].tp == tt_string);
            REQUIRE(out[0].text == "te'st");
            REQUIRE(out[1].tp == tt_string);
            REQUIRE(out[1].text == "te\"st");
            REQUIRE(out[2].tp == tt_template_string);
            REQUIRE(out[2].text == "te`st");
            REQUIRE(out[3].tp == tt_eof);
        }

        SECTION("Unterminated string exception") {
            const char* testSource =
                "'test"
            ;
            ProgramSource src = ProgramSource("BadString.gs", testSource);
            Lexer l(&src);

            REQUIRE_THROWS_WITH(l.tokenize(), "Encountered unexpected end of input while scanning string literal");
        }
    }

    SECTION("Lexer Numbers") {
        SECTION("Integers") {
            const char* testSource =
                "12345 1 0 -1 -12345"
            ;
            ProgramSource src = ProgramSource("Integers.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 6);
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
            REQUIRE(out[5].tp == tt_eof);
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
            ProgramSource src = ProgramSource("SuffixedIntegers.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 49);
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
            
            REQUIRE(out[i  ].tp == tt_eof);
        }

        SECTION("Floating point") {
            const char* testSource =
                "12345. 1.0 0.4532. -0.1 -123.45"
            ;
            ProgramSource src = ProgramSource("FloatingPoint.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 7);
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
            REQUIRE(out[6].tp == tt_eof);
        }

        SECTION("Floating point with suffixes") {
            const char* testSource =
                "12345.0f 1.0ull 0.4532.f"
            ;
            ProgramSource src = ProgramSource("SuffixedFloatingPoint.gs", testSource);
            Lexer l(&src);
            const Array<token>& out = l.tokenize();

            REQUIRE(out.size() == 8);
            REQUIRE(out[0].tp == tt_number);
            REQUIRE(out[0].text == "12345.0");
            REQUIRE(out[1].tp == tt_number_suffix);
            REQUIRE(out[1].text == "f");
            REQUIRE(out[2].tp == tt_number);
            REQUIRE(out[2].text == "1.0");
            REQUIRE(out[3].tp == tt_number_suffix);
            REQUIRE(out[3].text == "ull");
            REQUIRE(out[4].tp == tt_number);
            REQUIRE(out[4].text == "0.4532");
            REQUIRE(out[5].tp == tt_dot);
            REQUIRE(out[5].text == ".");
            REQUIRE(out[6].tp == tt_identifier);
            REQUIRE(out[6].text == "f");
            REQUIRE(out[7].tp == tt_eof);
        }
    }

    SECTION("Lexer Keywords") {
        const char* testSource =
            "import test from 'test'\n"
            "importe test as export from5 'test'\n"
            "if else do while for break continue "
            "type enum class extends public private "
            "import export from as operator static "
            "const get set null return switch case "
            "default true false this function let "
            "new try throw catch"
        ;
        ProgramSource src = ProgramSource("Keywords.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 47);
        u32 i = 0;
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "import");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "from");
        REQUIRE(out[i  ].tp == tt_string);
        REQUIRE(out[i++].text == "test");
        
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "importe");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "as");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "export");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "from5");
        REQUIRE(out[i  ].tp == tt_string);
        REQUIRE(out[i++].text == "test");

        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "if");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "else");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "do");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "while");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "for");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "break");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "continue");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "type");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "enum");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "class");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "extends");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "public");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "private");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "import");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "export");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "from");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "as");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "operator");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "static");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "const");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "get");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "set");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "null");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "return");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "switch");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "case");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "default");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "true");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "false");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "this");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "function");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "let");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "new");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "try");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "throw");
        REQUIRE(out[i  ].tp == tt_keyword);
        REQUIRE(out[i++].text == "catch");
        
        REQUIRE(out[i  ].tp == tt_eof);
    }

    SECTION("Lexer Identifiers") {
        const char* testSource =
            "test _test 0test test0 _0test test_0"
        ;
        ProgramSource src = ProgramSource("Identifiers.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 8);
        u32 i = 0;
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "_test");
        REQUIRE(out[i  ].tp == tt_number);
        REQUIRE(out[i++].text == "0");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test0");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "_0test");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "test_0");
        
        REQUIRE(out[i  ].tp == tt_eof);
    }

    SECTION("Lexer Symbols") {
        const char* testSource =
            "+  -  *  /  ~  !  %  ^  <  >  <<  >>  =  &  &&  |  ||  "
            "+= -= *= /= ~= != %= ^= <= >= <<= >>= == &= &&= |= ||= "
            "?"
            "a+a a+=a a&&=a"
        ;
        ProgramSource src = ProgramSource("Symbols.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 45);
        u32 i = 0;
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "+");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "-");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "*");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "/");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "~");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "!");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "%");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "^");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "<");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == ">");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "<<");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == ">>");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "&");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "&&");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "|");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "||");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "+=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "-=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "*=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "/=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "~=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "!=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "%=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "^=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "<=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == ">=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "<<=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == ">>=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "==");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "&=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "&&=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "|=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "||=");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "?");

        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "+");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");
        
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "+=");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");

        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");
        REQUIRE(out[i  ].tp == tt_symbol);
        REQUIRE(out[i++].text == "&&=");
        REQUIRE(out[i  ].tp == tt_identifier);
        REQUIRE(out[i++].text == "a");
        
        REQUIRE(out[i  ].tp == tt_eof);
    }

    SECTION("Lexer No Input") {
        const char* testSource = "";
        ProgramSource src = ProgramSource("NoInput.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 1);
        REQUIRE(out[0].tp == tt_eof);
    }

    SECTION("Lexer Source Preservation & Extra Tokens") {
        // Must test other tokens:
        // tt_forward
        // tt_dot
        // tt_comma
        // tt_colon
        // tt_semicolon
        // tt_open_parenth
        // tt_close_parenth
        // tt_open_brace
        // tt_close_brace
        // tt_open_bracket
        // tt_close_bracket
        //
        // As well as source locations

        const char* testSource =
            "(a: i32, arr: i32[]) => {\n"
            "    if (a >= arr.length) return 0;\n"
            "    return a < 0 ? 0 : arr[a];\n"
            "}"
        ;
        ProgramSource src = ProgramSource("OtherTokens.gs", testSource);
        Lexer l(&src);
        const Array<token>& out = l.tokenize();

        REQUIRE(out.size() == 38);
        i32 i = 0;
        #define CHECK_TOK(tok, txt, line, col)   \
        REQUIRE(out[i  ].tp == tok);             \
        REQUIRE(out[i  ].src.getLine() == line); \
        REQUIRE(out[i  ].src.getCol() == col);   \
        REQUIRE(out[i++].text == txt);

        CHECK_TOK(tt_open_parenth  , "("      , 0, 0 );
        CHECK_TOK(tt_identifier    , "a"      , 0, 1 );
        CHECK_TOK(tt_colon         , ":"      , 0, 2 );
        CHECK_TOK(tt_identifier    , "i32"    , 0, 4 );
        CHECK_TOK(tt_comma         , ","      , 0, 7 );
        CHECK_TOK(tt_identifier    , "arr"    , 0, 9 );
        CHECK_TOK(tt_colon         , ":"      , 0, 12);
        CHECK_TOK(tt_identifier    , "i32"    , 0, 14);
        CHECK_TOK(tt_open_bracket  , "["      , 0, 17);
        CHECK_TOK(tt_close_bracket , "]"      , 0, 18);
        CHECK_TOK(tt_close_parenth , ")"      , 0, 19);
        CHECK_TOK(tt_forward       , "=>"     , 0, 21);
        CHECK_TOK(tt_open_brace    , "{"      , 0, 24);

        CHECK_TOK(tt_keyword       , "if"     , 1, 4 );
        CHECK_TOK(tt_open_parenth  , "("      , 1, 7 );
        CHECK_TOK(tt_identifier    , "a"      , 1, 8 );
        CHECK_TOK(tt_symbol        , ">="     , 1, 10);
        CHECK_TOK(tt_identifier    , "arr"    , 1, 13);
        CHECK_TOK(tt_dot           , "."      , 1, 16);
        CHECK_TOK(tt_identifier    , "length" , 1, 17);
        CHECK_TOK(tt_close_parenth , ")"      , 1, 23);
        CHECK_TOK(tt_keyword       , "return" , 1, 25);
        CHECK_TOK(tt_number        , "0"      , 1, 32);
        CHECK_TOK(tt_semicolon     , ";"      , 1, 33);

        CHECK_TOK(tt_keyword       , "return" , 2, 4 );
        CHECK_TOK(tt_identifier    , "a"      , 2, 11);
        CHECK_TOK(tt_symbol        , "<"      , 2, 13);
        CHECK_TOK(tt_number        , "0"      , 2, 15);
        CHECK_TOK(tt_symbol        , "?"      , 2, 17);
        CHECK_TOK(tt_number        , "0"      , 2, 19);
        CHECK_TOK(tt_colon         , ":"      , 2, 21);
        CHECK_TOK(tt_identifier    , "arr"    , 2, 23);
        CHECK_TOK(tt_open_bracket  , "["      , 2, 26);
        CHECK_TOK(tt_identifier    , "a"      , 2, 27);
        CHECK_TOK(tt_close_bracket , "]"      , 2, 28);
        CHECK_TOK(tt_semicolon     , ";"      , 2, 29);

        CHECK_TOK(tt_close_brace   , "}"      , 3, 0 );

        REQUIRE(out[i  ].tp == tt_eof);
    }
}