#pragma once
#include <gs/common/types.h>
#include <gs/compiler/lexer.h>

#include <utils/String.h>
#include <utils/Array.h>
#include <utils/Allocator.h>

namespace gs {
    namespace compiler {
        enum node_type {
            nt_empty = 0,
            nt_root,
            nt_break,
            nt_catch,
            nt_class,
            nt_continue,
            nt_eos,
            nt_export,
            nt_expression,
            nt_function,
            nt_function_type,
            nt_identifier,
            nt_if,
            nt_import,
            nt_import_symbol,
            nt_import_module,
            nt_literal,
            nt_loop,
            nt_object_decompositor,
            nt_object_literal_property,
            nt_parameter,
            nt_property,
            nt_return,
            nt_scoped_block,
            nt_switch,
            nt_switch_case,
            nt_this,
            nt_throw,
            nt_try,
            nt_type,
            nt_type_modifier,
            nt_type_property,
            nt_type_specifier,
            nt_variable
        };
        
        enum expr_operator {
            op_undefined = 0,
            op_add,
            op_addEq,
            op_sub,
            op_subEq,
            op_mul,
            op_mulEq,
            op_div,
            op_divEq,
            op_mod,
            op_modEq,
            op_xor,
            op_xorEq,
            op_bitInv,
            op_bitInvEq,
            op_bitAnd,
            op_bitAndEq,
            op_bitOr,
            op_bitOrEq,
            op_shLeft,
            op_shLeftEq,
            op_shRight,
            op_shRightEq,
            op_not,
            op_notEq,
            op_logAnd,
            op_logAndEq,
            op_logOr,
            op_logOrEq,
            op_assign,
            op_compare,
            op_lessThan,
            op_lessThanEq,
            op_greaterThan,
            op_greaterThanEq,
            op_conditional,
            op_preInc,
            op_postInc,
            op_preDec,
            op_postDec,
            op_negate,
            op_member,
            op_index,
            op_new,
            op_call
        };
        
        enum literal_type {
            // value stored in ast_node::value
            lt_u8,
            lt_u16,
            lt_u32,
            lt_u64,
            lt_i8,
            lt_i16,
            lt_i32,
            lt_i64,
            lt_f32,
            lt_f64,
            lt_string,
            lt_template_string,

            // value stored in ast_node::body
            lt_object,
            lt_array
        };

        enum parse_error_code {
            pec_none = 0,
            pec_expected_eof,
            pec_expected_eos,
            pec_expected_open_brace,
            pec_expected_open_parenth,
            pec_expected_closing_brace,
            pec_expected_closing_parenth,
            pec_expected_closing_bracket,
            pec_expected_parameter,
            pec_expected_colon,
            pec_expected_expr,
            pec_expected_variable_decl,
            pec_expected_type_specifier,
            pec_expected_expr_group,
            pec_expected_statement,
            pec_expected_while,
            pec_expected_function_body,
            pec_expected_identifier,
            pec_expected_type_name,
            pec_expected_object_property,
            pec_expected_catch,
            pec_expected_catch_param,
            pec_expected_catch_param_type,
            pec_expected_import_alias,
            pec_expected_import_list,
            pec_expected_import_name,
            pec_expected_export_decl,
            pec_expected_symbol,
            pec_expected_parameter_list
        };

        struct ast_node {
            const token* tok;
            node_type tp;
            literal_type value_tp;
            expr_operator op;
            u32 str_len;
            union {
                u64 u;
                i64 i;
                f64 f;

                // token.text.c_str result is valid until
                // compilation is completed
                const char* s;
            } value;

            utils::String str() const;
            void json(u32 indent = 0, u32 index = 0, bool noIndentOpenBrace = true) const;

            struct {
                unsigned is_const   : 1;
                unsigned is_static  : 1;
                unsigned is_private : 1;
                unsigned is_array   : 1;
                unsigned is_pointer : 1;

                // for do ... while loops
                unsigned defer_cond : 1;
            } flags;

            ast_node* data_type;

            // expressions, conditionals
            ast_node* lvalue;
            ast_node* rvalue;

            // if statements, conditionals, loops
            ast_node* cond;

            // just about everything
            ast_node* body;

            // if statements, try...catch
            ast_node* else_body;

            // variable decl, loops
            ast_node* initializer;

            // functions, function calls
            ast_node* parameters;

            // for loops, type specifiers, type_modifiers
            ast_node* modifier;

            // imported identifiers, object deconstruction identifiers
            ast_node* alias;

            // anything sequential
            ast_node* next;
        };

        struct parse_error {
            parse_error_code code;
            utils::String text;
            token src;
        };

        class ParserState {
            public:
                ParserState(Lexer* l);
                ~ParserState();

                void push();
                void revert();
                void commit();

                void consume();

                const token& get() const;
                const token& getPrev() const;

                ast_node* parse();

                ast_node* newNode(node_type tp, const token* src = nullptr);
                void freeNode(ast_node* n);
                void addType(const utils::String& name, ast_node* n);
                ast_node* getType(const utils::String& name);
                const utils::Array<parse_error>& errors() const;

                // helpers
                bool typeIs(token_type tp) const;
                bool textIs(const char* str) const;
                bool isKeyword(const char* str) const;
                bool isSymbol(const char* str) const;
                void error(parse_error_code code, const utils::String& msg);
                void error(parse_error_code code, const utils::String& msg, const token& tok);

            private:
                utils::Array<token> m_tokens;
                utils::Array<u32> m_currentIdx;
                utils::Array<parse_error> m_errors;
                utils::PagedAllocator<ast_node, utils::FixedAllocator<ast_node>> m_nodeAlloc;
        };
    

        //
        // Exposed for unit test purposes
        //
        ast_node* eos                       (ParserState* ps);
        ast_node* eosRequired               (ParserState* ps);
        ast_node* statementList             (ParserState* ps);
        ast_node* block                     (ParserState* ps);
        ast_node* identifier                (ParserState* ps);
        ast_node* typedIdentifier           (ParserState* ps);
        ast_node* typeModifier              (ParserState* ps);
        ast_node* typeProperty              (ParserState* ps);
        ast_node* parenthesizedTypeSpecifier(ParserState* ps);
        ast_node* typeSpecifier             (ParserState* ps);
        ast_node* assignable                (ParserState* ps);
        ast_node* typedAssignable           (ParserState* ps);
        ast_node* parameter                 (ParserState* ps);
        ast_node* typedParameter            (ParserState* ps);
        ast_node* maybeTypedParameterList   (ParserState* ps);
        ast_node* maybeParameterList        (ParserState* ps);
        ast_node* parameterList             (ParserState* ps);
        ast_node* arguments                 (ParserState* ps);
        ast_node* objectDecompositorProperty(ParserState* ps);
        ast_node* objectDecompositor        (ParserState* ps);
        ast_node* anonymousFunction         (ParserState* ps);
        ast_node* numberLiteral             (ParserState* ps);
        ast_node* stringLiteral             (ParserState* ps);
        ast_node* templateStringLiteral     (ParserState* ps);
        ast_node* arrayLiteral              (ParserState* ps);
        ast_node* objectLiteralProperty     (ParserState* ps, bool expected);
        ast_node* objectLiteral             (ParserState* ps);
        ast_node* primaryExpression         (ParserState* ps);
        ast_node* opAssignmentExpression    (ParserState* ps);
        ast_node* assignmentExpression      (ParserState* ps);
        ast_node* conditionalExpression     (ParserState* ps);
        ast_node* logicalOrExpression       (ParserState* ps);
        ast_node* logicalAndExpression      (ParserState* ps);
        ast_node* bitwiseOrExpression       (ParserState* ps);
        ast_node* bitwiseXOrExpression      (ParserState* ps);
        ast_node* bitwiseAndExpression      (ParserState* ps);
        ast_node* equalityExpression        (ParserState* ps);
        ast_node* relationalExpression      (ParserState* ps);
        ast_node* shiftExpression           (ParserState* ps);
        ast_node* additiveExpression        (ParserState* ps);
        ast_node* multiplicativeExpression  (ParserState* ps);
        ast_node* notExpression             (ParserState* ps);
        ast_node* invertExpression          (ParserState* ps);
        ast_node* negateExpression          (ParserState* ps);
        ast_node* prefixDecExpression       (ParserState* ps);
        ast_node* prefixIncExpression       (ParserState* ps);
        ast_node* postfixDecExpression      (ParserState* ps);
        ast_node* postfixIncExpression      (ParserState* ps);
        ast_node* callExpression            (ParserState* ps);
        ast_node* newExpression             (ParserState* ps);
        ast_node* memberExpression          (ParserState* ps);
        ast_node* indexExpression           (ParserState* ps);
        ast_node* expression                (ParserState* ps);
        ast_node* variableDecl              (ParserState* ps);
        ast_node* variableDeclList          (ParserState* ps);
        ast_node* variableStatement         (ParserState* ps);
        ast_node* emptyStatement            (ParserState* ps);
        ast_node* expressionSequence        (ParserState* ps);
        ast_node* expressionSequenceGroup   (ParserState* ps);
        ast_node* expressionStatement       (ParserState* ps);
        ast_node* ifStatement               (ParserState* ps);
        ast_node* continueStatement         (ParserState* ps);
        ast_node* breakStatement            (ParserState* ps);
        ast_node* iterationStatement        (ParserState* ps);
        ast_node* returnStatement           (ParserState* ps);
        ast_node* switchCase                (ParserState* ps);
        ast_node* switchStatement           (ParserState* ps);
        ast_node* throwStatement            (ParserState* ps);
        ast_node* catchBlock                (ParserState* ps);
        ast_node* tryStatement              (ParserState* ps);
        ast_node* functionDecl              (ParserState* ps);
        ast_node* functionDef               (ParserState* ps);
        ast_node* classDef                  (ParserState* ps);
        ast_node* typeDef                   (ParserState* ps);
        ast_node* symbolImport              (ParserState* ps);
        ast_node* importList                (ParserState* ps);
        ast_node* importModule              (ParserState* ps);
        ast_node* declaration               (ParserState* ps);
        ast_node* importStatement           (ParserState* ps);
        ast_node* exportStatement           (ParserState* ps);
        ast_node* statement                 (ParserState* ps);
        ast_node* program                   (ParserState* ps);
    };
};