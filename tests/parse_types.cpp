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

TEST_CASE("Parse Types", "[parser]") {
    Mem::Create();

    SECTION("typeModifier") {
        ModuleSource* src = nullptr;

        src = mock_module_source("[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 1);
            REQUIRE(n->flags.is_pointer == 0);
        }

        src = mock_module_source("[ ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("[1]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);

            // Error recovery
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 1);
            REQUIRE(n->flags.is_pointer == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("*");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 0);
            REQUIRE(n->flags.is_pointer == 1);
        }

        src = mock_module_source("**");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 0);
            REQUIRE(n->flags.is_pointer == 1);

            n = n->modifier;
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 0);
            REQUIRE(n->flags.is_pointer == 1);
        }

        src = mock_module_source("[]*");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeModifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 1);
            REQUIRE(n->flags.is_pointer == 0);

            n = n->modifier;
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_modifier);
            REQUIRE(n->flags.is_array == 0);
            REQUIRE(n->flags.is_pointer == 1);
        }
    }
    
    SECTION("typeProperty") {
        ModuleSource* src = nullptr;

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            // Not a typeProperty, no error
            ParseNode* n = typeProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            // Still not a typeProperty (no type specifier), no error
            ParseNode* n = typeProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a : 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            // Type property, invalid type specifier though (for now)
            ParseNode* n = typeProperty(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected type specifier after ':'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 4);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a : i32");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n->tp == nt_type_property);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("a : { b: i32; }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeProperty(&p);

            // Just check that it's valid, object type specifiers
            // will be tested later
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n->tp == nt_type_property);
            REQUIRE(n->str() == "a");
        }
        delete_mocked_source(src);

        src = mock_module_source("a : i32[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeProperty(&p);

            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n->tp == nt_type_property);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->modifier != nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("parenthesizedTypeSpecifier") {
        ModuleSource* src = nullptr;

        src = mock_module_source("i32");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("(5)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("(5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("(i32 ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_parenth);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ')'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 4);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("(i32)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("(i32[])");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->modifier != nullptr);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("({ a: i32 })");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parenthesizedTypeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("identifierTypeSpecifier") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            ParseNode* id = identifier(&p);
            ParseNode* n = identifierTypeSpecifier(&p, id);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body == id);
            REQUIRE(n->template_parameters == nullptr);
            REQUIRE(n->modifier == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a<b, { c: i32; }>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            ParseNode* id = identifier(&p);
            ParseNode* n = identifierTypeSpecifier(&p, id);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body == id);
            REQUIRE(n->template_parameters != nullptr);
            REQUIRE(n->modifier == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a<b, { c: i32; }>[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            ParseNode* id = identifier(&p);
            ParseNode* n = identifierTypeSpecifier(&p, id);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body == id);
            REQUIRE(n->template_parameters != nullptr);
            REQUIRE(n->modifier != nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            ParseNode* n = identifierTypeSpecifier(&p, nullptr);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("typeSpecifier") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "a");
            REQUIRE(n->template_parameters == nullptr);
            REQUIRE(n->modifier == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a<b, { c: i32; }>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "a");
            REQUIRE(n->template_parameters != nullptr);
            REQUIRE(n->modifier == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a<b, { c: i32; }>[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "a");
            REQUIRE(n->template_parameters != nullptr);
            REQUIRE(n->modifier != nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32; b: f32; c: { d: boolean; }; }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "b");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "f32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "c");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);

            b = b->data_type->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "d");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "boolean");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32; b: f32; c: { d: boolean; }; }[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->modifier != nullptr);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "b");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "f32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "c");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);

            b = b->data_type->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "d");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "boolean");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_eos);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ';' after type property");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 9);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32, }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_eos);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ';' after type property");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32; b: i32 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_eos);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ';' after type property");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 17);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "b");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32; b: i32, }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_eos);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ';' after type property");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 16);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");

            b = b->next;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "b");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32, ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 2);
            const auto& msg0 = log.getMessages()[0];
            REQUIRE(msg0.code == pm_expected_eos);
            REQUIRE(msg0.type == lt_error);
            REQUIRE(msg0.msg == "Expected ';' after type property");
            REQUIRE(msg0.src.getLine() == 0);
            REQUIRE(msg0.src.getCol() == 8);

            const auto& msg1 = log.getMessages()[1];
            REQUIRE(msg1.code == pm_expected_closing_brace);
            REQUIRE(msg1.type == lt_error);
            REQUIRE(msg1.msg == "Expected '}' to close object type definition");
            REQUIRE(msg1.src.getLine() == 0);
            REQUIRE(msg1.src.getCol() == 9);

            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: i32, 5 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 2);
            const auto& msg0 = log.getMessages()[0];
            REQUIRE(msg0.code == pm_expected_eos);
            REQUIRE(msg0.type == lt_error);
            REQUIRE(msg0.msg == "Expected ';' after type property");
            REQUIRE(msg0.src.getLine() == 0);
            REQUIRE(msg0.src.getCol() == 8);

            const auto& msg1 = log.getMessages()[1];
            REQUIRE(msg1.code == pm_expected_closing_brace);
            REQUIRE(msg1.type == lt_error);
            REQUIRE(msg1.msg == "Expected '}' to close object type definition");
            REQUIRE(msg1.src.getLine() == 0);
            REQUIRE(msg1.src.getCol() == 10);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);

            ParseNode* b = n->body;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_type_property);
            REQUIRE(b->str() == "a");
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
            REQUIRE(b->data_type->body != nullptr);
            REQUIRE(b->data_type->body->tp == nt_identifier);
            REQUIRE(b->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("() => void");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32) => void");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);

            ParseNode* b = n->parameters;
            REQUIRE(b != nullptr);
            REQUIRE(b->tp == nt_identifier);
            REQUIRE(b->data_type != nullptr);
            REQUIRE(b->data_type->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("() => ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected return type specifier");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 5);

            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("() ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);
            
            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_symbol);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected '=>' to indicate return type specifier after parameter list");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);

            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("(a)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("({ a: i32; })");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typeSpecifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
        }
        delete_mocked_source(src);
    }
    
    SECTION("assignable") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = assignable(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
        }
        delete_mocked_source(src);

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = assignable(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("typedAssignable") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedAssignable(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a : i32");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedAssignable(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a : ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedAssignable(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected type specifier after ':'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }

    Mem::Destroy();
}
