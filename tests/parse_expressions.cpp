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

TEST_CASE("Parse Expressions", "[parser]") {
    Mem::Create();

    SECTION("primaryExpression") {
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
    
    SECTION("functionExpression") {
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
    
    SECTION("memberExpression") {
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
    
    SECTION("newExpression") {
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
    
    SECTION("callExpression") {
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
    
    SECTION("leftHandSideExpression") {
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
    
    SECTION("postfixExpression") {
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
    
    SECTION("unaryExpression") {
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
    
    SECTION("multiplicativeExpression") {
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
    
    SECTION("additiveExpression") {
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
    
    SECTION("shiftExpression") {
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
    
    SECTION("relationalExpression") {
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
    
    SECTION("equalityExpression") {
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
    
    SECTION("bitwiseAndExpression") {
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
    
    SECTION("XOrExpression") {
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
    
    SECTION("bitwiseOrExpression") {
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
    
    SECTION("logicalAndExpression") {
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
    
    SECTION("logicalOrExpression") {
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
    
    SECTION("conditionalExpression") {
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
    
    SECTION("assignmentExpression") {
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
    
    SECTION("singleExpression") {
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
    
    SECTION("expression") {
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
    
    SECTION("expressionSequence") {
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
    
    SECTION("expressionSequenceGroup") {
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
