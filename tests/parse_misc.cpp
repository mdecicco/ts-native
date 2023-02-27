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

TEST_CASE("Misc. Parse Functions", "[parser]") {
    Mem::Create();

    SECTION("eos") {
        ModuleSource* src = nullptr;

        src = mock_module_source(";");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = eos(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_eos);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = eos(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }

    SECTION("eosRequired") {
        ModuleSource* src = nullptr;

        src = mock_module_source(";");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = eos(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_eos);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = eosRequired(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_eos);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ';'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 0);
            REQUIRE(isError(n));

            // Should have skipped to next semicolon
            REQUIRE(p.get().tp == tt_semicolon);
        }
        delete_mocked_source(src);

        src = mock_module_source("a ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = eosRequired(&p);

            // Should not have skipped ahead, no semicolon to find
            REQUIRE(p.get().tp == tt_identifier);
        }
        delete_mocked_source(src);
    }

    SECTION("identifier") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = identifier(&p);
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

            ParseNode* n = identifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }

    SECTION("typedIdentifier") {
        ModuleSource* src = nullptr;
        
        src = mock_module_source("a : tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedIdentifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "tp");
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedIdentifier(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().tp == tt_identifier);
            REQUIRE(p.get().text == "a");
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a : 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedIdentifier(&p);
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
    }
    
    SECTION("objectDecompositorProperty") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositorProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a : tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositorProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "tp");
        }
        delete_mocked_source(src);

        src = mock_module_source("5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositorProperty(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }

    SECTION("objectDecompositor") {
        ModuleSource* src = nullptr;

        src = mock_module_source("{ a, b: i32 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositor(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_decompositor);
            
            ParseNode* prop = n->body;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "a");
            REQUIRE(prop->data_type == nullptr);

            prop = prop->next;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "b");
            REQUIRE(prop->data_type != nullptr);
            REQUIRE(prop->data_type->tp == nt_type_specifier);
            REQUIRE(prop->data_type->body->tp == nt_identifier);
            REQUIRE(prop->data_type->body->str() == "i32");

            REQUIRE(prop->next == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a, b: i32, }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositor(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_object_property);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected property after ','");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 13);
            

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_decompositor);
            
            ParseNode* prop = n->body;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "a");
            REQUIRE(prop->data_type == nullptr);

            prop = prop->next;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "b");
            REQUIRE(prop->data_type != nullptr);
            REQUIRE(prop->data_type->tp == nt_type_specifier);
            REQUIRE(prop->data_type->body->tp == nt_identifier);
            REQUIRE(prop->data_type->body->str() == "i32");

            REQUIRE(prop->next == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a, b: i32, 444 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = objectDecompositor(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_object_property);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected property after ','");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 13);
            

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_object_decompositor);
            
            ParseNode* prop = n->body;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "a");
            REQUIRE(prop->data_type == nullptr);

            prop = prop->next;
            REQUIRE(prop != nullptr);
            REQUIRE(prop->tp == nt_identifier);
            REQUIRE(prop->str() == "b");
            REQUIRE(prop->data_type != nullptr);
            REQUIRE(prop->data_type->tp == nt_type_specifier);
            REQUIRE(prop->data_type->body->tp == nt_identifier);
            REQUIRE(prop->data_type->body->str() == "i32");

            REQUIRE(prop->next == nullptr);
        }
        delete_mocked_source(src);
    }

    Mem::Destroy();
}
