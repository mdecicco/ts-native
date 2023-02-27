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

TEST_CASE("Parse parameters/arguments", "[parser]") {
    Mem::Create();

    SECTION("templateArgs") {
        ModuleSource* src = nullptr;

        src = mock_module_source("<a, { b: i32; }>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateArgs(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            // Object type specifier will be tested elsewhere
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<a,>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateArgs(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected template argument");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->body->str() == "a");

            // Should have skipped to and read the '>' token
            REQUIRE(p.get().tp == tt_eof);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateArgs(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_symbol);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected '>' after template argument list");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);

            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
            REQUIRE(p.get().tp == tt_eof);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateArgs(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected template argument");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);

            // Not enough info to know if it should be template arguments, not an errorNode
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("templateParams") {
        ModuleSource* src = nullptr;

        src = mock_module_source("<a, b>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateParams(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "b");
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<a,>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateParams(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected template parameter");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);

            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateParams(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected template parameter");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);

            // Not enough info to know if it should be template arguments, not an errorNode
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("<a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = templateParams(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_symbol);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected '>' after template parameter list");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);

            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("parameter") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = parameter(&p);
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

            ParseNode* n = parameter(&p);
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

            ParseNode* n = parameter(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("typedParameter") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedParameter(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a : tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = typedParameter(&p);
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

            ParseNode* n = typedParameter(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }
    
    SECTION("maybeTypedParameterList") {
        ModuleSource* src = nullptr;

        src = mock_module_source("()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = maybeTypedParameterList(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = maybeTypedParameterList(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32, b: f32)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = maybeTypedParameterList(&p);
            REQUIRE(log.getMessages().size() == 0);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "i32");

            n = n->next;
            
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "b");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "f32");
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32,)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = maybeTypedParameterList(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_parameter);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected typed parameter after ','");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->body != nullptr);
            REQUIRE(n->data_type->body->tp == nt_identifier);
            REQUIRE(n->data_type->body->str() == "i32");
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32 ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = maybeTypedParameterList(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_parenth);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ')' after typed parameter list");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);

            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("maybeParameterList") {
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
    }
    
    SECTION("parameterList") {
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
    }
    
    SECTION("typedParameterList") {
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
    }
    
    SECTION("arguments") {
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
    }

    Mem::Destroy();
}
