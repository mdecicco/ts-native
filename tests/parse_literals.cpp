#include <catch2/catch_test_macros.hpp>
#include <utils/Allocator.hpp>
#include <utils/Array.hpp>

#include <mock/ModuleSource.h>

#include <tsn/compiler/Lexer.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Logger.h>

using namespace tsn;
using namespace compiler;
using namespace utils;

TEST_CASE("Parse Literals", "[parser]") {
    Mem::Create();

    SECTION("numberLiteral") {
        ModuleSource* src = nullptr;
        // todo: parse error on number literal out of range for storage type

        src = mock_module_source("18446744073709551615ull");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u64);
            REQUIRE(n->value.u == 18446744073709551615);
        }
        delete_mocked_source(src);

        src = mock_module_source("0ull");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u64);
            REQUIRE(n->value.u == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("9223372036854775807ll");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i64);
            REQUIRE(n->value.i == 9223372036854775807i64);
        }
        delete_mocked_source(src);

        src = mock_module_source("-9223372036854775808ll");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i64);
            REQUIRE(n->value.i == -9223372036854775808i64);
        }
        delete_mocked_source(src);

        src = mock_module_source("4294967295ul");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u32);
            REQUIRE(n->value.i == 4294967295);
        }
        delete_mocked_source(src);

        src = mock_module_source("0ul");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u32);
            REQUIRE(n->value.i == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("2147483647");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
            REQUIRE(n->value.i == 2147483647);
        }
        delete_mocked_source(src);

        src = mock_module_source("-2147483648");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
            REQUIRE(n->value.i == -2147483648);
        }
        delete_mocked_source(src);

        src = mock_module_source("65535us");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u16);
            REQUIRE(n->value.i == 65535);
        }
        delete_mocked_source(src);

        src = mock_module_source("0us");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u16);
            REQUIRE(n->value.i == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("32767s");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i16);
            REQUIRE(n->value.i == 32767);
        }
        delete_mocked_source(src);

        src = mock_module_source("-32768s");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i16);
            REQUIRE(n->value.i == -32768);
        }
        delete_mocked_source(src);

        src = mock_module_source("255ub");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u8);
            REQUIRE(n->value.i == 255);
        }
        delete_mocked_source(src);

        src = mock_module_source("0ub");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_u8);
            REQUIRE(n->value.i == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("127b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i8);
            REQUIRE(n->value.i == 127);
        }
        delete_mocked_source(src);

        src = mock_module_source("-128b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i8);
            REQUIRE(n->value.i == -128);
        }
        delete_mocked_source(src);

        src = mock_module_source("12.3456789f");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_f32);
            REQUIRE(n->value.f == 12.3456789);
        }
        delete_mocked_source(src);

        src = mock_module_source("1234.5678910111213");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = numberLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_f64);
            REQUIRE(n->value.f == 1234.5678910111213);
        }
        delete_mocked_source(src);
    }
    
    SECTION("stringLiteral") {
        ModuleSource* src = nullptr;

        src = mock_module_source("''");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("'abcdefg'");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 7);
            REQUIRE(n->str() == "abcdefg");
        }
        delete_mocked_source(src);

        src = mock_module_source("\"abcdefg\"");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 7);
            REQUIRE(n->str() == "abcdefg");
        }
        delete_mocked_source(src);

        src = mock_module_source("\"a'b'c'd'e'f'g\"");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a'b'c'd'e'f'g");
        }
        delete_mocked_source(src);

        src = mock_module_source("'a\\'b\\'c\\'d\\'e\\'f\\'g'");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a'b'c'd'e'f'g");
        }
        delete_mocked_source(src);

        src = mock_module_source("'a\"b\"c\"d\"e\"f\"g'");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a\"b\"c\"d\"e\"f\"g");
        }
        delete_mocked_source(src);

        src = mock_module_source("\"a\\\"b\\\"c\\\"d\\\"e\\\"f\\\"g\"");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a\"b\"c\"d\"e\"f\"g");
        }
        delete_mocked_source(src);

        src = mock_module_source("'abcdefg\n'");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = stringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 8);
            REQUIRE(n->str() == "abcdefg\n");
        }
        delete_mocked_source(src);
    }
    
    SECTION("templateStringLiteral") {
        ModuleSource* src = nullptr;

        // todo

        src = mock_module_source("``");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("`abcdefg`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 7);
            REQUIRE(n->str() == "abcdefg");
        }
        delete_mocked_source(src);

        src = mock_module_source("`abcdefg`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 7);
            REQUIRE(n->str() == "abcdefg");
        }
        delete_mocked_source(src);

        src = mock_module_source("`a'b'c'd'e'f'g`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a'b'c'd'e'f'g");
        }
        delete_mocked_source(src);

        src = mock_module_source("`a\\'b\\'c\\'d\\'e\\'f\\'g`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a'b'c'd'e'f'g");
        }
        delete_mocked_source(src);

        src = mock_module_source("`a\"b\"c\"d\"e\"f\"g`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a\"b\"c\"d\"e\"f\"g");
        }
        delete_mocked_source(src);

        src = mock_module_source("`a\\\"b\\\"c\\\"d\\\"e\\\"f\\\"g`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 13);
            REQUIRE(n->str() == "a\"b\"c\"d\"e\"f\"g");
        }
        delete_mocked_source(src);

        src = mock_module_source("`abcdefg\n`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateStringLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            REQUIRE(n->str_len == 8);
            REQUIRE(n->str() == "abcdefg\n");
        }
        delete_mocked_source(src);
    }
    
    SECTION("arrayLiteral") {
        ModuleSource* src = nullptr;

        src = mock_module_source("[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = arrayLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_array);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("[5 ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = arrayLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']' to close array literal");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("[5]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = arrayLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_array);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_literal);
            REQUIRE(n->body->value_tp == lt_i32);
            REQUIRE(n->body->value.i == 5);
        }
        delete_mocked_source(src);

        src = mock_module_source("[1,2,3]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = arrayLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_array);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_expression_sequence);

            n = n->body->body;        

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
            REQUIRE(n->value.i == 1);

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
            REQUIRE(n->value.i == 2);

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
            REQUIRE(n->value.i == 3);
        }
        delete_mocked_source(src);
    }
    
    SECTION("objectLiteralProperty") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a: 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteralProperty(&p, false);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "a");
            REQUIRE(n->initializer != nullptr);
            REQUIRE(n->initializer->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("a 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteralProperty(&p, false);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteralProperty(&p, true);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_colon);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ':' after 'a'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a: ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteralProperty(&p, true);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'a:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a: ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteralProperty(&p, false);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'a:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("objectLiteral") {
        ModuleSource* src = nullptr;

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);

            n = n->body;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "a");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5, b: 6, c: 7 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);

            n = n->body;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "b");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "c");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'a:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 4);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: , b: 1 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'a:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 5);
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);

            n = n->body;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "b");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5, b: , c: 7 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'b:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 11);
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);

            n = n->body;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "c");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: } ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after 'a:'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 5);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));

            // It should have skipped to after the closing brace
            REQUIRE(p.get().src.getCol() == 6);    
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5, } ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_object_property);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected object literal property after ','");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));

            // It should have skipped to after the closing brace
            REQUIRE(p.get().src.getCol() == 9);    
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5, 6");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_object_property);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected object literal property after ','");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));

            // It should not have skipped anything
            REQUIRE(p.get().src.getCol() == 8);    
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5 6 } ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_brace);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected '}' to close object literal");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);

            n = n->body;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_literal_property);
            REQUIRE(n->str() == "a");

            // It should have skipped to after the closing brace
            REQUIRE(p.get().src.getCol() == 10);    
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 5 6");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectLiteral(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_brace);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected '}' to close object literal");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));

            // It should not have skipped anything
            REQUIRE(p.get().src.getCol() == 7);    
        }
        delete_mocked_source(src);
    }

    Mem::Destroy();
}
