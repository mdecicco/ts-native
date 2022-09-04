#pragma once
#include <gs/common/types.h>
#include <gs/compiler/Lexer.h>

#include <utils/String.h>
#include <utils/Array.h>
#include <utils/Allocator.h>

namespace std {
    template <typename T>
    class initializer_list;
};

namespace gs {
    namespace compiler {
        enum node_type {
            nt_empty = 0,
            nt_root,
            nt_eos,

            nt_break,
            nt_catch,
            nt_class,
            nt_continue,
            nt_export,
            nt_expression,
            nt_function,
            nt_function_type,
            nt_identifier,
            nt_if,
            nt_import,
            nt_import_module,
            nt_import_symbol,
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
            nt_variable,
            nt_cast,
            nt_sizeof
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
            op_bitAnd,
            op_bitAndEq,
            op_bitOr,
            op_bitOrEq,
            op_bitInv,
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
            op_orderUnknownInc,
            op_preInc,
            op_postInc,
            op_orderUnknownDec,
            op_preDec,
            op_postDec,
            op_negate,
            op_member,
            op_index,
            op_new,
            op_placementNew,
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
            lt_array,

            // value not stored
            lt_null,
            lt_true,
            lt_false
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
            pec_expected_expgroup,
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
            pec_expected_parameter_list,
            pec_expected_single_template_arg,
            pec_expected_operator_override_target,
            pec_malformed_class_element,
            pec_empty_class,
            pec_reserved_word
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
            ast_node* clone(bool copyNext = false);

            struct {
                unsigned is_const   : 1;
                unsigned is_static  : 1;
                unsigned is_private : 1;
                unsigned is_array   : 1;
                unsigned is_pointer : 1;
                unsigned is_getter  : 1;
                unsigned is_setter  : 1;

                // for do ... while loops
                unsigned defecond : 1;
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

            // functions, classes, types
            ast_node* template_parameters;

            // for loops, type specifiers, type_modifiers
            ast_node* modifier;

            // imported identifiers, object deconstruction identifiers
            ast_node* alias;

            // classes
            ast_node* inheritance;

            // anything sequential
            ast_node* next;

            static void destroyDetachedAST(ast_node* n);
        };

        struct parse_error {
            parse_error_code code;
            utils::String text;
            token src;
        };

        class Parser {
            public:
                Parser(Lexer* l);
                ~Parser();

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
        // Helpers
        //
        typedef ast_node* (*parsefn)(Parser*);
        ast_node* array_of                      (Parser* ps, parsefn fn);
        ast_node* list_of                       (
                                                     Parser* ps, parsefn fn,
                                                     parse_error_code before_comma_err, const utils::String& before_comma_msg,
                                                     parse_error_code after_comma_err, const utils::String& after_comma_msg
                                                );
        ast_node* one_of                        (Parser* ps, std::initializer_list<parsefn> rules);
        ast_node* all_of                        (Parser* ps, std::initializer_list<parsefn> rules);
        bool isAssignmentOperator               (Parser* ps);
        expr_operator getOperatorType           (Parser* ps);
        //
        // Misc
        //
        ast_node* eos                           (Parser* ps);
        ast_node* eosRequired                   (Parser* ps);
        ast_node* identifier                    (Parser* ps);
        ast_node* typedIdentifier               (Parser* ps);
        ast_node* objectDecompositorProperty    (Parser* ps);
        ast_node* objectDecompositor            (Parser* ps);
        //
        // Parameters / Arguments
        //
        ast_node* templateArgs                  (Parser* ps);
        ast_node* templateParams                (Parser* ps);
        ast_node* parameter                     (Parser* ps);
        ast_node* typedParameter                (Parser* ps);
        ast_node* maybeTypedParameterList       (Parser* ps);
        ast_node* maybeParameterList            (Parser* ps);
        ast_node* parameterList                 (Parser* ps);
        ast_node* typedParameterList            (Parser* ps);
        ast_node* arguments                     (Parser* ps);
        //
        // Types
        //
        ast_node* typeModifier                  (Parser* ps);
        ast_node* typeProperty                  (Parser* ps);
        ast_node* parenthesizedTypeSpecifier    (Parser* ps);
        ast_node* typeSpecifier                 (Parser* ps);
        ast_node* assignable                    (Parser* ps);
        ast_node* typedAssignable               (Parser* ps);
        //
        // Literals
        //
        ast_node* numberLiteral                 (Parser* ps);
        ast_node* stringLiteral                 (Parser* ps);
        ast_node* templateStringLiteral         (Parser* ps);
        ast_node* arrayLiteral                  (Parser* ps);
        ast_node* objectLiteralProperty         (Parser* ps, bool expected);
        ast_node* objectLiteral                 (Parser* ps);
        //
        // Expressions
        //
        ast_node* primaryExpression             (Parser* ps);
        ast_node* functionExpression            (Parser* ps);
        ast_node* memberExpression              (Parser* ps);
        ast_node* callExpression                (Parser* ps);
        ast_node* leftHandSideExpression        (Parser* ps);
        ast_node* postfixExpression             (Parser* ps);
        ast_node* unaryExpression               (Parser* ps);
        ast_node* multiplicativeExpression      (Parser* ps);
        ast_node* additiveExpression            (Parser* ps);
        ast_node* shiftExpression               (Parser* ps);
        ast_node* relationalExpression          (Parser* ps);
        ast_node* equalityExpression            (Parser* ps);
        ast_node* bitwiseAndExpression          (Parser* ps);
        ast_node* XOrExpression                 (Parser* ps);
        ast_node* bitwiseOrExpression           (Parser* ps);
        ast_node* logicalAndExpression          (Parser* ps);
        ast_node* logicalOrExpression           (Parser* ps);
        ast_node* conditionalExpression         (Parser* ps);
        ast_node* assignmentExpression          (Parser* ps);
        ast_node* singleExpression              (Parser* ps);
        ast_node* expression                    (Parser* ps);
        ast_node* expressionSequence            (Parser* ps);
        ast_node* expressionSequenceGroup       (Parser* ps);
        //
        // Declarations
        //
        ast_node* variableDecl                  (Parser* ps);
        ast_node* variableDeclList              (Parser* ps);
        ast_node* classPropertyOrMethod         (Parser* ps);
        ast_node* classDef                      (Parser* ps);
        ast_node* typeDef                       (Parser* ps);
        ast_node* functionDecl                  (Parser* ps);
        ast_node* functionDef                   (Parser* ps);
        ast_node* declaration                   (Parser* ps);
        //
        // Statements
        //
        ast_node* variableStatement             (Parser* ps);
        ast_node* emptyStatement                (Parser* ps);
        ast_node* expressionStatement           (Parser* ps);
        ast_node* ifStatement                   (Parser* ps);
        ast_node* continueStatement             (Parser* ps);
        ast_node* breakStatement                (Parser* ps);
        ast_node* iterationStatement            (Parser* ps);
        ast_node* returnStatement               (Parser* ps);
        ast_node* switchCase                    (Parser* ps);
        ast_node* switchStatement               (Parser* ps);
        ast_node* throwStatement                (Parser* ps);
        ast_node* catchBlock                    (Parser* ps);
        ast_node* tryStatement                  (Parser* ps);
        ast_node* placementNewStatement         (Parser* ps);
        ast_node* symbolImport                  (Parser* ps);
        ast_node* importList                    (Parser* ps);
        ast_node* importModule                  (Parser* ps);
        ast_node* importStatement               (Parser* ps);
        ast_node* exportStatement               (Parser* ps);
        ast_node* statement                     (Parser* ps);
        ast_node* statementList                 (Parser* ps);
        ast_node* block                         (Parser* ps);
        //
        // Entry
        //
        ast_node* program                       (Parser* ps);
    };
};