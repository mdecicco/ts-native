#include <common/types.h>
#include <common/source_ref.h>
#include <string>

namespace gjs {
    namespace lex {
        struct token;
    };

    namespace parse {
        struct ast {
            enum class node_type {
                root = 0,
                empty,
                variable_declaration,
                function_declaration,
                class_declaration,
                format_declaration,
                variable_initializer,
                class_property,
                format_property,
                function_arguments,
                if_statement,
                for_loop,
                while_loop,
                do_while_loop,
                import_statement,
                export_statement,
                return_statement,
                delete_statement,
                object,
                call,
                expression,
                conditional,
                constant,
                identifier,
                type_identifier,
                operation,
                scoped_block,
                context_type,
                context_function
            };
            enum class constant_type {
                none,
                integer,
                f32,
                f64,
                string
            };
            enum class operation_type {
                invalid = 0,
                add,
                sub,
                mul,
                div,
                mod,
                shiftLeft,
                shiftRight,
                land,
                lor,
                band,
                bor,
                bxor,
                addEq,
                subEq,
                mulEq,
                divEq,
                modEq,
                shiftLeftEq,
                shiftRightEq,
                landEq,
                lorEq,
                bandEq,
                borEq,
                bxorEq,
                preInc,
                preDec,
                postInc,
                postDec,
                less,
                greater,
                lessEq,
                greaterEq,
                notEq,
                isEq,
                eq,
                not,
                negate,
                member,
                index,
                newObj,
                stackObj
            };

            ast();
            ~ast();
            void debug_print(u32 tab_level, bool dontIndentFirst = false, bool inArray = false);

            node_type type;
            source_ref ref;
            ast* next;

            ast* data_type;
            ast* identifier;

            ast* body;
            ast* else_body;
            ast* property;
            ast* arguments;

            ast* initializer;
            ast* lvalue;
            ast* rvalue;
            ast* callee;
            ast* constructor;
            ast* destructor;
            ast* condition;
            ast* modifier;

            constant_type c_type;
            union {
                i64 i;
                f32 f_32;
                f64 f_64;
                char* s;
            } value;

            void set(i64 v);
            void set(f32 v);
            void set(f64 v);
            void set(const std::string& v);
            void src(const lex::token& tk);

            operator std::string() const;
            operator i64();
            operator f32();
            operator f64();

            // per-type values
            operation_type op;
            bool is_const;
            bool is_static;
        };
    };
};