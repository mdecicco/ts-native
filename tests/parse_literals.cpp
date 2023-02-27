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
    
    SECTION("stringLiteral") {
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
    
    SECTION("templateStringLiteral") {
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
    
    SECTION("arrayLiteral") {
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
    
    SECTION("objectLiteralProperty") {
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
    
    SECTION("objectLiteral") {
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
