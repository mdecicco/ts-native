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

        src = mock_module_source("this");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_this);
        }
        delete_mocked_source(src);

        src = mock_module_source("null");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_null);
        }
        delete_mocked_source(src);

        src = mock_module_source("true");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_true);
        }
        delete_mocked_source(src);

        src = mock_module_source("false");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_false);
        }
        delete_mocked_source(src);

        src = mock_module_source("sizeof<tp>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_sizeof);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("sizeof<tp, tp>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_single_template_arg);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected single template parameter after 'sizeof'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 11);

            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_sizeof);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->data_type->next == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("sizeof ");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_single_template_arg);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected single template parameter after 'sizeof'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 6);

            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("1");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_i32);
        }
        delete_mocked_source(src);

        src = mock_module_source("'1'");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
        }
        delete_mocked_source(src);

        src = mock_module_source("`1`");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_string);
            // TODO
            // REQUIRE(n->value_tp == lt_template_string);
        }
        delete_mocked_source(src);

        src = mock_module_source("[1,2,3]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_array);
        }
        delete_mocked_source(src);

        src = mock_module_source("{ a: 1, b: 2, c: 3 }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
            REQUIRE(n->value_tp == lt_object);
        }
        delete_mocked_source(src);

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a<tp>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a < b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(p.get().src.getCol() == 2);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a < b)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = primaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
        }
        delete_mocked_source(src);
    }
    
    SECTION("functionExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("1");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().src.getCol() == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().src.getCol() == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a, b)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().src.getCol() == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32, b: i32)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
            REQUIRE(p.get().src.getCol() == 0);
        }
        delete_mocked_source(src);

        src = mock_module_source("a => a + 1");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_function);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_identifier);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_expression);
        }
        delete_mocked_source(src);

        src = mock_module_source("(a, b) => { return a + b; }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_function);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_scoped_block);
            
            n = n->parameters;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->str() == "b");
        }
        delete_mocked_source(src);

        src = mock_module_source("(a: i32, b: i32) => { return a + b; }");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_function);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_scoped_block);
            
            n = n->parameters;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->str() == "a");

            n = n->next;
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
            REQUIRE(n->str() == "b");
        }
        delete_mocked_source(src);

        src = mock_module_source("a => ;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = functionExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_function_body);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected arrow function body");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 5);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("memberExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a => a + 1");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_function);
        }
        delete_mocked_source(src);

        src = mock_module_source("1");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("++a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("a.b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->str() == "a");
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_identifier);
            REQUIRE(n->rvalue->str() == "b");
        }
        delete_mocked_source(src);

        src = mock_module_source("a.b.c");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);

            // a.b.c
            // ^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_member);
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->str() == "a");
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue->str() == "b");

            // a.b.c
            //    ^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_identifier);
            REQUIRE(n->rvalue->str() == "c");
        }
        delete_mocked_source(src);

        src = mock_module_source("a.5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_identifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected identifier after '.'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a[1]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->str() == "a");
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("a[1][2]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);

            // a[1][2]
            // ^^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_index);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_literal);
            REQUIRE(n->lvalue->rvalue->value.i == 1);

            // a[1][2]
            //     ^^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
            REQUIRE(n->rvalue->value.i == 2);
        }
        delete_mocked_source(src);

        src = mock_module_source("a[;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '['");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '['");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->str() == "a");
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(isError(n->rvalue));
        }
        delete_mocked_source(src);

        src = mock_module_source("a[5;]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->str() == "a");
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("a[5;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 3);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("a.b[1]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);

            // a.b[2]
            // ^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_member);
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->str() == "a");
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue->str() == "b");

            // a.b[2]
            //    ^^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
            REQUIRE(n->rvalue->value.i == 1);
        }
        delete_mocked_source(src);

        src = mock_module_source("a[1].b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = memberExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);

            // a[1].c
            // ^^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_index);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_literal);
            REQUIRE(n->lvalue->rvalue->value.i == 1);

            // a[1].c
            //     ^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_identifier);
            REQUIRE(n->rvalue->str() == "b");
        }
        delete_mocked_source(src);
    }
    
    SECTION("newExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a.b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);
        }
        delete_mocked_source(src);

        src = mock_module_source("new tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->parameters == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("new tp()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("new tp<x>");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_type_specifier);
            REQUIRE(n->parameters == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("new tp<x>()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_type_specifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("new mod.tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_expression);
            REQUIRE(n->body->op == op_member);
            REQUIRE(n->body->lvalue != nullptr);
            REQUIRE(n->body->lvalue->tp == nt_identifier);
            REQUIRE(n->body->rvalue != nullptr);
            REQUIRE(n->body->rvalue->tp == nt_identifier);
            REQUIRE(n->parameters == nullptr);
        }
        delete_mocked_source(src);

        src = mock_module_source("new mod.tp()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = newExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_expression);
            REQUIRE(n->body->op == op_member);
            REQUIRE(n->body->lvalue != nullptr);
            REQUIRE(n->body->lvalue->tp == nt_identifier);
            REQUIRE(n->body->rvalue != nullptr);
            REQUIRE(n->body->rvalue->tp == nt_identifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);
    }
    
    SECTION("callExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("new tp()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_new);
        }
        delete_mocked_source(src);

        src = mock_module_source("new tp()()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_new);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("func(a)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("func(a, b, c)");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_expression_sequence);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->parameters != nullptr);
            REQUIRE(n->parameters->tp == nt_empty);
        }
        delete_mocked_source(src);

        src = mock_module_source("func().a");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("func().a.b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_member);
            REQUIRE(n->lvalue->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue->str() == "a");
            REQUIRE(n->rvalue->tp == nt_identifier);
            REQUIRE(n->rvalue->str() == "b");
        }
        delete_mocked_source(src);

        src = mock_module_source("func().5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_identifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected identifier after '.'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[1]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[1][2]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);

            // func()[1][2]
            // ^^^^^^^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_index);
            
            // func()[1][2]
            // ^^^^^^
            REQUIRE(n->lvalue->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->lvalue->parameters->tp == nt_empty);
            
            // func()[1][2]
            //       ^^^
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_literal);
            REQUIRE(n->lvalue->rvalue->value.i == 1);

            // func()[1][2]
            //          ^^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
            REQUIRE(n->rvalue->value.i == 2);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '['");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '['");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 7);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(isError(n->rvalue));
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[5;]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->parameters->tp == nt_empty);
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[5;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_closing_bracket);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected ']'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 8);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("func().b[1]");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_index);

            // func().b[2]
            // ^^^^^^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_member);
            
            // func().b[2]
            // ^^^^^^
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->lvalue->parameters->tp == nt_empty);

            // func().b[2]
            //       ^^
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->rvalue->str() == "b");

            // func().b[2]
            //         ^^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_literal);
            REQUIRE(n->rvalue->value.i == 1);
        }
        delete_mocked_source(src);

        src = mock_module_source("func()[1].b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = callExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);

            // func()[1].c
            // ^^^^^^^^^
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->op == op_index);

            // func()[1].c
            // ^^^^^^
            REQUIRE(n->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->tp == nt_expression);
            REQUIRE(n->lvalue->lvalue->op == op_call);
            REQUIRE(n->lvalue->lvalue->lvalue != nullptr);
            REQUIRE(n->lvalue->lvalue->lvalue->tp == nt_identifier);
            REQUIRE(n->lvalue->lvalue->parameters != nullptr);
            REQUIRE(n->lvalue->lvalue->parameters->tp == nt_empty);

            // func()[1].c
            //       ^^^
            REQUIRE(n->lvalue->rvalue != nullptr);
            REQUIRE(n->lvalue->rvalue->tp == nt_literal);
            REQUIRE(n->lvalue->rvalue->value.i == 1);

            // func()[1].c
            //          ^^
            REQUIRE(n->rvalue != nullptr);
            REQUIRE(n->rvalue->tp == nt_identifier);
            REQUIRE(n->rvalue->str() == "b");
        }
        delete_mocked_source(src);
    }
    
    SECTION("leftHandSideExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("a()");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = leftHandSideExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_call);
        }
        delete_mocked_source(src);

        src = mock_module_source("a.b");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = leftHandSideExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_member);
        }
        delete_mocked_source(src);

        src = mock_module_source("a as tp");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = leftHandSideExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_cast);
            REQUIRE(n->body != nullptr);
            REQUIRE(n->body->tp == nt_identifier);
            REQUIRE(n->data_type != nullptr);
            REQUIRE(n->data_type->tp == nt_type_specifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("a as 5");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = leftHandSideExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_type_specifier);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected type specifier after 'as' keyword");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 5);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);
    }
    
    SECTION("postfixExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = postfixExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("i++");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = postfixExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_postInc);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("i--");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = postfixExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_postDec);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);
    }
    
    SECTION("unaryExpression") {
        ModuleSource* src = nullptr;

        src = mock_module_source("i++");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_postInc);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("-i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_negate);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("~i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_bitInv);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("!i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_not);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("*i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_dereference);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("++i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_preInc);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("--i");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 0);
            REQUIRE(n != nullptr);
            REQUIRE(n->tp == nt_expression);
            REQUIRE(n->op == op_preDec);
            REQUIRE(n->lvalue != nullptr);
            REQUIRE(n->lvalue->tp == nt_identifier);
        }
        delete_mocked_source(src);

        src = mock_module_source("-;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '-'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("~;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '~'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("!;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '!'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("*;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '*'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 1);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("++;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '++'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
        }
        delete_mocked_source(src);

        src = mock_module_source("--;");
        {
            Logger log;
            Lexer l(src);
            Parser p(&l, &log);

            ParseNode* n = unaryExpression(&p);
            REQUIRE(log.getMessages().size() == 1);
            const auto& msg = log.getMessages()[0];
            REQUIRE(msg.code == pm_expected_expr);
            REQUIRE(msg.type == lt_error);
            REQUIRE(msg.msg == "Expected expression after '--'");
            REQUIRE(msg.src.getLine() == 0);
            REQUIRE(msg.src.getCol() == 2);
            REQUIRE(n != nullptr);
            REQUIRE(isError(n));
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
