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

TEST_CASE("Parse Functions", "[parser]") {
    Mem::Create();

    SECTION("errorNode") {
        ModuleSource* src = mock_module_source("test");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* e = errorNode(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(e->tp == nt_error);
        }
        delete_mocked_source(src);
    }

    SECTION("isError") {
        ModuleSource* src = mock_module_source("test");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* e = errorNode(&p);
            ParseNode* n = p.newNode(nt_empty);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(isError(e) == true);
            REQUIRE(isError(n) == false);
            REQUIRE(isError(nullptr) == false);
        }
        delete_mocked_source(src);
    }

    SECTION("array_of") {
        ModuleSource* src = nullptr;
        
        src = mock_module_source("a b c d e 3 f");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = array_of(&p, identifier);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "a");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "b");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "c");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "d");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "e");
            REQUIRE(n->next == nullptr);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("5 a b c d e 3 f");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = array_of(&p, identifier);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);

            // Check that on failure it reverts any changes to the parse state
            REQUIRE(p.get().tp == tt_number);
            REQUIRE(p.get().text == "5");
        }
        delete_mocked_source(src);
    }

    SECTION("list_of") {
        ModuleSource* src = nullptr;
        
        src = mock_module_source("a,b,c,d,e");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = list_of(&p, identifier, pm_none, "", pm_none, "");
            REQUIRE(log.getMessages().size() == 0);
            
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "a");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "b");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "c");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "d");
            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->str() == "e");
            REQUIRE(n->next == nullptr);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a, 3");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = list_of(
                &p,
                identifier,
                pm_expected_colon, // can be anything other than lm_generic. Testing that the second code is used.
                "",
                lm_generic,
                "test"
            );
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 1);
            REQUIRE(msgs[0].code == lm_generic);
            REQUIRE(msgs[0].type == lt_error);
            REQUIRE(msgs[0].msg == "test");
            REQUIRE(msgs[0].src.getLine() == 0);
            REQUIRE(msgs[0].src.getCol() == 3);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
        
        src = mock_module_source(" 3,3");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = list_of(
                &p,
                identifier,
                lm_generic,
                "test",
                pm_expected_colon, // can be anything other than lm_generic. Testing that the first code is used
                ""
            );
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 1);
            REQUIRE(msgs[0].code == lm_generic);
            REQUIRE(msgs[0].type == lt_error);
            REQUIRE(msgs[0].msg == "test");
            REQUIRE(msgs[0].src.getLine() == 0);
            REQUIRE(msgs[0].src.getCol() == 1);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
        
        src = mock_module_source("3,3");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = list_of(
                &p,
                identifier,
                pm_none,
                "",
                lm_generic,
                "test"
            );
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);
    }

    SECTION("one_of") {
        ModuleSource* src = nullptr;
        
        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = one_of(&p, {
                numberLiteral,
                functionDecl,
                identifier
            });
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = one_of(&p, {
                identifier,
                numberLiteral,
                functionDecl
            });
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("[5]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = one_of(&p, {
                identifier,
                numberLiteral,
                functionDecl
            });
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n == nullptr);

            // Check that on failure it reverts any changes to the parse state
            REQUIRE(p.get().tp == tt_open_bracket);
        }
        delete_mocked_source(src);
    }

    SECTION("all_of") {
        ModuleSource* src = nullptr;
        
        src = mock_module_source("a 1 a 'test' [3]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = all_of(&p, {
                identifier,
                numberLiteral,
                identifier,
                stringLiteral,
                arrayLiteral
            });
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
        }
        delete_mocked_source(src);
        
        src = mock_module_source("a 1 a 4 'test' [3]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = all_of(&p, {
                identifier,
                numberLiteral,
                identifier,
                stringLiteral,
                arrayLiteral
            });
            const auto& msgs = log.getMessages();
            REQUIRE(msgs.size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().tp == tt_identifier);
            REQUIRE(p.get().text == "a");
        }
        delete_mocked_source(src);
    }

    SECTION("isAssignmentOperator") {
        ModuleSource* src = nullptr;
        
        const char* code =
            "+  -  *   /  ~   !  %  ^  <  >  =  &  &&  |  || "
            "+= -= *=  /=     != %= ^= <= >= == &= &&= |= ||="
            "?  << <<= >> >>="
        ;
        bool isAssignment[] = {
            //  +      -      *      /      ~      !      %      ^      <      >      =      &     &&      |     ||
            false, false, false, false, false, false, false, false, false, false,  true, false, false, false, false,
            // +=     -=     *=     /=            !=     %=     ^=     <=     >=     ==     &=    &&=     |=    ||=
            true , true , true , true ,        false, true , true , false, false, false, true , true , true , true ,
            //  ?     <<    <<=     >>    >>=
            false, false, true , false, true ,
        };
        
        src = mock_module_source(code);
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            for (u32 i = 0;i < sizeof(isAssignment);i++) {
                REQUIRE(isAssignmentOperator(&p) == isAssignment[i]);
                p.consume();
            }

        }
        delete_mocked_source(src);
    }

    SECTION("getOperatorType") {
        ModuleSource* src = nullptr;
        
        const char* code =
            "+  -  *   /  ~   !  %  ^  <  >  =  &  &&  |  || "
            "+= -= *=  /=     != %= ^= <= >= == &= &&= |= ||="
            "?  << <<= >> >>= -- ++ new ."
        ;

        src = mock_module_source(code);
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            REQUIRE(getOperatorType(&p) == op_add);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_sub);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_mul);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_div);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_bitInv);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_not);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_mod);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_xor);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_lessThan);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_greaterThan);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_assign);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_bitAnd);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_logAnd);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_bitOr);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_logOr);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_addEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_subEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_mulEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_divEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_notEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_modEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_xorEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_lessThanEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_greaterThanEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_compare);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_bitAndEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_logAndEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_bitOrEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_logOrEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_conditional);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_shLeft);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_shLeftEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_shRight);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_shRightEq);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_undefined);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_undefined);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_undefined);
            p.consume();
            REQUIRE(getOperatorType(&p) == op_undefined);
            p.consume();

        }
        delete_mocked_source(src);
    }



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



    SECTION("templateArgs") {
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
    
    SECTION("templateParams") {
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
    
    SECTION("parameter") {
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
    
    SECTION("typedParameter") {
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
    
    SECTION("maybeTypedParameterList") {
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
    


    SECTION("typeModifier") {
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
    
    SECTION("typeProperty") {
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
    
    SECTION("parenthesizedTypeSpecifier") {
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
    
    SECTION("identifierTypeSpecifier") {
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
    
    SECTION("typeSpecifier") {
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
    
    SECTION("assignable") {
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
    
    SECTION("typedAssignable") {
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



    SECTION("variableDecl") {
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
    
    SECTION("variableDeclList") {
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
    
    SECTION("classPropertyOrMethod") {
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



    SECTION("variableStatement") {
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
    
    SECTION("emptyStatement") {
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
    
    SECTION("expressionStatement") {
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
    
    SECTION("ifStatement") {
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
    
    SECTION("continueStatement") {
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
    
    SECTION("breakStatement") {
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
    
    SECTION("iterationStatement") {
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
    
    SECTION("deleteStatement") {
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
    
    SECTION("returnStatement") {
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
    
    SECTION("switchCase") {
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
    
    SECTION("switchStatement") {
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
    
    SECTION("throwStatement") {
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
    
    SECTION("catchBlock") {
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
    
    SECTION("tryStatement") {
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
    
    SECTION("placementNewStatement") {
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
    
    SECTION("symbolImport") {
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
    
    SECTION("importList") {
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
    
    SECTION("importModule") {
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
    
    SECTION("importStatement") {
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
    
    SECTION("exportStatement") {
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
    
    SECTION("statement") {
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
    
    SECTION("statementList") {
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
    
    SECTION("block") {
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
