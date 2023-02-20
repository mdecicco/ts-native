#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/Lexer.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/interfaces/IWithLogger.h>
#include <tsn/interfaces/ITransactional.h>

#include <utils/String.h>
#include <utils/Array.h>
#include <utils/Allocator.h>

namespace std {
    template <typename T>
    class initializer_list;
};

namespace tsn {
    namespace compiler {
        enum node_type {
            nt_empty = 0,
            nt_error,
            nt_root,
            nt_eos,
            nt_break,
            nt_cast,
            nt_catch,
            nt_class,
            nt_continue,
            nt_delete,
            nt_export,
            nt_expression,
            nt_expression_sequence,
            nt_function_type,
            nt_function,
            nt_identifier,
            nt_if,
            nt_import_module,
            nt_import_symbol,
            nt_import,
            nt_literal,
            nt_loop,
            nt_object_decompositor,
            nt_object_literal_property,
            nt_parameter,
            nt_property,
            nt_return,
            nt_scoped_block,
            nt_sizeof,
            nt_switch_case,
            nt_switch,
            nt_this,
            nt_throw,
            nt_try,
            nt_type_modifier,
            nt_type_property,
            nt_type_specifier,
            nt_type,
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
            op_preInc,
            op_postInc,
            op_preDec,
            op_postDec,
            op_negate,
            op_dereference,
            op_index,
            op_conditional,
            op_member,
            op_new,
            op_placementNew,
            op_call
        };
        
        enum literal_type {
            // value stored in ParseNode::value
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

            // value stored in ParseNode::body
            lt_object,
            lt_array,

            // value not stored
            lt_null,
            lt_true,
            lt_false
        };

        class ParseNode : IPersistable {
            public:
                ParseNode();

                token tok;
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
                void computeSourceLocationRange();
                void manuallySpecifyRange(const token& end);
                void rehydrateSourceRefs(ModuleSource* src);
                ParseNode* clone(bool copyNext = false);

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

                struct {
                    unsigned is_const    : 1;
                    unsigned is_static   : 1;
                    unsigned is_private  : 1;
                    unsigned is_array    : 1;
                    unsigned is_pointer  : 1;
                    unsigned is_getter   : 1;
                    unsigned is_setter   : 1;

                    // for do ... while loops
                    unsigned defer_cond  : 1;

                    // for the node itself
                    unsigned is_detached : 1;
                } flags;

                ParseNode* data_type;

                // expressions, conditionals
                ParseNode* lvalue;
                ParseNode* rvalue;

                // if statements, conditionals, loops
                ParseNode* cond;

                // just about everything
                ParseNode* body;

                // if statements, try...catch
                ParseNode* else_body;

                // variable decl, loops, object literal properties
                ParseNode* initializer;

                // functions, function calls
                ParseNode* parameters;

                // functions, classes, types
                ParseNode* template_parameters;

                // for loops, type specifiers, type_modifiers
                ParseNode* modifier;

                // imported identifiers, object deconstruction identifiers
                ParseNode* alias;

                // classes
                ParseNode* inheritance;

                // anything sequential
                ParseNode* next;

                static void destroyDetachedAST(ParseNode* n);
        };

        class Parser : public IWithLogger, ITransactional {
            public:
                Parser(Lexer* l, Logger* logs);
                ~Parser();

                virtual void begin();
                virtual void revert();
                virtual void commit();

                void consume();

                const token& get() const;
                const token& getPrev() const;

                ParseNode* parse();

                ParseNode* newNode(node_type tp, const token* src = nullptr);
                void freeNode(ParseNode* n);
                void addType(const utils::String& name, ParseNode* n);
                ParseNode* getType(const utils::String& name);

                // helpers
                bool typeIs(token_type tp) const;
                bool textIs(const char* str) const;
                bool isKeyword(const char* str) const;
                bool isSymbol(const char* str) const;
                void error(log_message_code code, const utils::String& msg);
                void error(log_message_code code, const utils::String& msg, const token& tok);

            private:
                utils::Array<token> m_tokens;
                utils::Array<u32> m_currentIdx;
                utils::PagedAllocator<ParseNode, utils::FixedAllocator<ParseNode>> m_nodeAlloc;
        };
    


        //
        // Helpers
        //
        typedef ParseNode* (*parsefn)(Parser*);
        ParseNode* errorNode                     (Parser* ps);
        bool isError                             (ParseNode* n);
        bool findRecoveryToken                   (Parser* ps, bool consumeRecoveryToken = true);
        ParseNode* array_of                      (Parser* ps, parsefn fn);
        ParseNode* list_of                       (
                                                     Parser* ps, parsefn fn,
                                                     log_message_code before_comma_err, const utils::String& before_comma_msg,
                                                     log_message_code after_comma_err, const utils::String& after_comma_msg
                                                 );
        ParseNode* one_of                        (Parser* ps, std::initializer_list<parsefn> rules);
        ParseNode* all_of                        (Parser* ps, std::initializer_list<parsefn> rules);
        bool isAssignmentOperator                (Parser* ps);
        expr_operator getOperatorType            (Parser* ps);
        //
        // Misc
        //
        ParseNode* eos                           (Parser* ps);
        ParseNode* eosRequired                   (Parser* ps);
        ParseNode* identifier                    (Parser* ps);
        ParseNode* typedIdentifier               (Parser* ps);
        ParseNode* objectDecompositorProperty    (Parser* ps);
        ParseNode* objectDecompositor            (Parser* ps);
        //
        // Parameters / Arguments
        //
        ParseNode* templateArgs                  (Parser* ps);
        ParseNode* templateParams                (Parser* ps);
        ParseNode* parameter                     (Parser* ps);
        ParseNode* typedParameter                (Parser* ps);
        ParseNode* maybeTypedParameterList       (Parser* ps);
        ParseNode* maybeParameterList            (Parser* ps);
        ParseNode* parameterList                 (Parser* ps);
        ParseNode* typedParameterList            (Parser* ps);
        ParseNode* arguments                     (Parser* ps);
        //
        // Types
        //
        ParseNode* typeModifier                  (Parser* ps);
        ParseNode* typeProperty                  (Parser* ps);
        ParseNode* parenthesizedTypeSpecifier    (Parser* ps);
        ParseNode* identifierTypeSpecifier       (Parser* ps, ParseNode* id);
        ParseNode* typeSpecifier                 (Parser* ps);
        ParseNode* assignable                    (Parser* ps);
        ParseNode* typedAssignable               (Parser* ps);
        //
        // Literals
        //
        ParseNode* numberLiteral                 (Parser* ps);
        ParseNode* stringLiteral                 (Parser* ps);
        ParseNode* templateStringLiteral         (Parser* ps);
        ParseNode* arrayLiteral                  (Parser* ps);
        ParseNode* objectLiteralProperty         (Parser* ps, bool expected);
        ParseNode* objectLiteral                 (Parser* ps);
        //
        // Expressions
        //
        ParseNode* primaryExpression             (Parser* ps);
        ParseNode* functionExpression            (Parser* ps);
        ParseNode* memberExpression              (Parser* ps);
        ParseNode* callExpression                (Parser* ps);
        ParseNode* leftHandSideExpression        (Parser* ps);
        ParseNode* postfixExpression             (Parser* ps);
        ParseNode* unaryExpression               (Parser* ps);
        ParseNode* multiplicativeExpression      (Parser* ps);
        ParseNode* additiveExpression            (Parser* ps);
        ParseNode* shiftExpression               (Parser* ps);
        ParseNode* relationalExpression          (Parser* ps);
        ParseNode* equalityExpression            (Parser* ps);
        ParseNode* bitwiseAndExpression          (Parser* ps);
        ParseNode* XOrExpression                 (Parser* ps);
        ParseNode* bitwiseOrExpression           (Parser* ps);
        ParseNode* logicalAndExpression          (Parser* ps);
        ParseNode* logicalOrExpression           (Parser* ps);
        ParseNode* conditionalExpression         (Parser* ps);
        ParseNode* assignmentExpression          (Parser* ps);
        ParseNode* singleExpression              (Parser* ps);
        ParseNode* expression                    (Parser* ps);
        ParseNode* expressionSequence            (Parser* ps);
        ParseNode* expressionSequenceGroup       (Parser* ps);
        //
        // Declarations
        //
        ParseNode* variableDecl                  (Parser* ps);
        ParseNode* variableDeclList              (Parser* ps);
        ParseNode* classPropertyOrMethod         (Parser* ps);
        ParseNode* classDef                      (Parser* ps);
        ParseNode* typeDef                       (Parser* ps);
        ParseNode* functionDecl                  (Parser* ps);
        ParseNode* functionDef                   (Parser* ps);
        ParseNode* declaration                   (Parser* ps);
        //
        // Statements
        //
        ParseNode* variableStatement             (Parser* ps);
        ParseNode* emptyStatement                (Parser* ps);
        ParseNode* expressionStatement           (Parser* ps);
        ParseNode* ifStatement                   (Parser* ps);
        ParseNode* continueStatement             (Parser* ps);
        ParseNode* breakStatement                (Parser* ps);
        ParseNode* iterationStatement            (Parser* ps);
        ParseNode* deleteStatement               (Parser* ps);
        ParseNode* returnStatement               (Parser* ps);
        ParseNode* switchCase                    (Parser* ps);
        ParseNode* switchStatement               (Parser* ps);
        ParseNode* throwStatement                (Parser* ps);
        ParseNode* catchBlock                    (Parser* ps);
        ParseNode* tryStatement                  (Parser* ps);
        ParseNode* placementNewStatement         (Parser* ps);
        ParseNode* symbolImport                  (Parser* ps);
        ParseNode* importList                    (Parser* ps);
        ParseNode* importModule                  (Parser* ps);
        ParseNode* importStatement               (Parser* ps);
        ParseNode* exportStatement               (Parser* ps);
        ParseNode* statement                     (Parser* ps);
        ParseNode* statementList                 (Parser* ps);
        ParseNode* block                         (Parser* ps);
        //
        // Entry
        //
        ParseNode* program                       (Parser* ps);
    };
};