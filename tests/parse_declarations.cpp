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

TEST_CASE("Parse Declarations", "[parser]") {
    Mem::Create();

    SECTION("variableDecl") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDecl(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_variable);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a = b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDecl(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_variable);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->initializer != nullptr);
            REQUIRE(n->initializer->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a = ;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDecl(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.msg == "Expected expression for variable initializer");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 4);

            // Should've skipped to the semicolon
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_variable);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->initializer == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a = ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDecl(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.msg == "Expected expression for variable initializer");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);

            // Should've skipped to the semicolon
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("variableDeclList") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a = b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDeclList(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("const a = b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDeclList(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_variable);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->initializer != nullptr);
            REQUIRE(n->initializer->tp == nt_identifier);
            REQUIRE(n->flags.is_const == 1);
        }
        delete_mocked_source(src);

        src = mock_module_source("let a = b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDeclList(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_variable);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->initializer != nullptr);
            REQUIRE(n->initializer->tp == nt_identifier);
            REQUIRE(n->flags.is_const == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("let ;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = variableDeclList(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.code == pm_expected_variable_decl);
            REQUIRE(msg.msg == "Expected variable declaration");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 4);
        }
        delete_mocked_source(src);
    }
    
    SECTION("classPropertyOrMethod") {
        ModuleSource* src = nullptr;

        // private X
        // public X
        // static X
        // X
        // (X is one of the following)
        //
        // Y : type;
        // constructor : type; <- error
        // constructor () : type {} <- error
        // constructor () {}
        // constructor<T> () {} <- error
        // destructor : type; <- error
        // destructor () {}
        // destructor () : type {} <- error
        // destructor<T> () {} <- error
        // get Y () {}
        // get Y () : type {}
        // set Y () {}
        // set Y () : type {}
        // get operator () {} <- error
        // set operator () {} <- error
        // operator : type; <- error
        // operator Z () {}
        // operator Z () : type {}
        // operator type () {}
        // operator type () : type {} <- error
        // Y () {}
        // Y () : type {}
        // Y<T> () {}
        // Y<T> () : type {}

        /*
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
        */

        SECTION("Properties") {
            // private Y : type;
            // public Y : type;
            // static Y : type;
            // Y : type;
            // Y; <- error
            // Y : ; <- error
            // constructor : type; <- error
            // destructor : type; <- error

            src = mock_module_source("Y : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 0);
                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "Y");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("public Y : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 0);
                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "Y");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("private Y : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 0);
                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "Y");
                REQUIRE(n->flags.is_private == 1);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("static Y : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 0);
                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "Y");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 1);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("Y;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 1);
                const auto& msg = log.getMessages()[0];
                REQUIRE(msg.type == lt_error);
                REQUIRE(msg.code == pm_malformed_class_element);
                REQUIRE(msg.msg == "Expected property type specifier or method parameter list after identifier");
                REQUIRE(msg.src.getLine() == 0);
                REQUIRE(msg.src.getCol() == 1);

                REQUIRE(n != nullptr);
                REQUIRE(isError(n));
            }
            delete_mocked_source(src);

            src = mock_module_source("Y : ;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 1);
                const auto& msg = log.getMessages()[0];
                REQUIRE(msg.type == lt_error);
                REQUIRE(msg.code == pm_expected_type_specifier);
                REQUIRE(msg.msg == "Expected type specifier for class property");
                REQUIRE(msg.src.getLine() == 0);
                REQUIRE(msg.src.getCol() == 4);

                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type == nullptr);
                REQUIRE(n->str() == "Y");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("constructor : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 1);
                const auto& msg = log.getMessages()[0];
                REQUIRE(msg.type == lt_error);
                REQUIRE(msg.code == pm_reserved_word);
                REQUIRE(msg.msg == "Cannot name a class property 'constructor', which is a reserved word");
                REQUIRE(msg.src.getLine() == 0);
                REQUIRE(msg.src.getCol() == 0);

                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "constructor");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);

            src = mock_module_source("destructor : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 1);
                const auto& msg = log.getMessages()[0];
                REQUIRE(msg.type == lt_error);
                REQUIRE(msg.code == pm_reserved_word);
                REQUIRE(msg.msg == "Cannot name a class property 'destructor', which is a reserved word");
                REQUIRE(msg.src.getLine() == 0);
                REQUIRE(msg.src.getCol() == 0);

                REQUIRE(n != nullptr);
                REQUIRE(n->tp == nt_property);
                REQUIRE(n->data_type != nullptr);
                REQUIRE(n->data_type->body->str() == "tp");
                REQUIRE(n->str() == "destructor");
                REQUIRE(n->flags.is_private == 0);
                REQUIRE(n->flags.is_static == 0);
                REQUIRE(n->flags.is_getter == 0);
                REQUIRE(n->flags.is_setter == 0);
            }
            delete_mocked_source(src);
        }

        SECTION("Constructors") {
            // constructor () : type {} <- error
            // constructor () {}
            // constructor<T> () {} <- error
        }

        SECTION("Destructors") {
            // destructor () {}
            // destructor () : type {} <- error
            // destructor<T> () {} <- error
        }
        
        SECTION("getters/setters") {
            // get Y () {}
            // get Y () : type {}
            // set Y () {}
            // set Y () : type {}
            // get operator () {} <- error
            // set operator () {} <- error
            // get : type; <- error
            // set : type; <- error
        }
        
        SECTION("Operators") {
            // operator Z () {}
            // operator Z () : type {}
            // operator type () {}
            // operator type () : type {} <- error
            // operator : type; <- error

            src = mock_module_source("operator : tp;");
            {
                Logger log;
                Lexer l(src);
                Parser p(&l, &log);

                ParseNode* n = classPropertyOrMethod(&p);
                REQUIRE(log.getMessages().size() == 1);
                const auto& msg = log.getMessages()[0];
                REQUIRE(msg.type == lt_error);
                REQUIRE(msg.code == pm_expected_operator_override_target);
                REQUIRE(msg.msg == "Expected operator or type specifier after 'operator' keyword");
                REQUIRE(msg.src.getLine() == 0);
                REQUIRE(msg.src.getCol() == 9);

                REQUIRE(n != nullptr);
                REQUIRE(isError(n));
            }
            delete_mocked_source(src);
        }
        
        SECTION("Methods") {
            // Y () {}
            // Y () : type {}
            // Y<T> () {}
            // Y<T> () : type {}
        }
    }
    
    SECTION("classDef") {
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
    
    SECTION("typeDef") {
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
    
    SECTION("functionDecl") {
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
    
    SECTION("functionDef") {
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
    
    SECTION("declaration") {
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
