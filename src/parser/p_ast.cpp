#include <gjs/lexer/token.h>
#include <gjs/parser/ast.h>

namespace gjs {
    namespace parse {
        static const char* type_names[] = {
            "root",
            "empty",
            "variable_declaration",
            "function_declaration",
            "class_declaration",
            "format_declaration",
            "enum_declaration",
            "variable_initializer",
            "property_initializer",
            "class_property",
            "format_property",
            "enum_value",
            "function_arguments",
            "if_statement",
            "for_loop",
            "while_loop",
            "do_while_loop",
            "import_statement",
            "return_statement",
            "delete_statement",
            "object",
            "call",
            "expression",
            "conditional",
            "constant",
            "identifier",
            "type_identifier",
            "operation",
            "scoped_block",
            "format_expression",
            "context_type",
            "context_function"
        };
        static const char* op_names[] = {
            "invalid",
            "add",
            "sub",
            "mul",
            "div",
            "mod",
            "shiftLeft",
            "shiftRight",
            "land",
            "lor",
            "band",
            "bor",
            "bxor",
            "addEq",
            "subEq",
            "mulEq",
            "divEq",
            "modEq",
            "shiftLeftEq",
            "shiftRightEq",
            "landEq",
            "lorEq",
            "bandEq",
            "borEq",
            "bxorEq",
            "preInc",
            "preDec",
            "postInc",
            "postDec",
            "less",
            "greater",
            "lessEq",
            "greaterEq",
            "notEq",
            "isEq",
            "eq",
            "not",
            "negate",
            "member",
            "index",
            "newObj",
            "stackObj"
        };

        ast::ast() {
            type = node_type::root;
            ref = { "", "", 0, 0 };
            next = nullptr;
            data_type = nullptr;
            identifier = nullptr;
            body = nullptr;
            else_body = nullptr;
            property = nullptr;
            arguments = nullptr;
            initializer = nullptr;
            lvalue = nullptr;
            rvalue = nullptr;
            callee = nullptr;
            destructor = nullptr;
            condition = nullptr;
            modifier = nullptr;
            c_type = constant_type::none;
            op = operation_type::invalid;
            is_const = false;
            is_static = false;
            is_subtype = false;
            memset(&value, 0, sizeof(value));
        }
        
        ast::~ast() {
            if (value.s && c_type == constant_type::string) delete [] value.s;
            if (next) delete next;
            if (data_type) delete data_type;
            if (identifier) delete identifier;
            if (body) delete body;
            if (else_body) delete else_body;
            if (property) delete property;
            if (arguments) delete arguments;
            if (initializer) delete initializer;
            if (lvalue) delete lvalue;
            if (rvalue) delete rvalue;
            if (callee) delete callee;
            if (destructor) delete destructor;
            if (condition) delete condition;
            if (modifier) delete modifier;
        }

        void ast::debug_print(u32 tab_level, bool dontIndentFirst, bool inArray) {
            auto tab = [tab_level](u32 offset) {
                for (u32 i = 0;i < tab_level + offset;i++) printf("    ");
            };

            if (!dontIndentFirst) tab(0);
            u32 indent = 1;
            printf("{\n");

            tab(indent);
            printf("node_type: '%s',\n", type_names[u32(type)]);

            tab(indent);
            printf("source: { line: %d, col: %d, module: '%s' },\n", ref.line + 1, ref.col, ref.module.c_str());

            if (type == node_type::variable_declaration) {
                tab(indent);
                printf("is_const: %s,\n", is_const ? "true" : "false");
                tab(indent);
                printf("is_static: %s,\n", is_static ? "true" : "false");
            }

            if (op != operation_type::invalid) {
                tab(indent);
                printf("operation: '%s',\n", op_names[u32(op)]);
            }

            switch (c_type) {
                case constant_type::integer: { tab(indent); printf("value: %lld,\n", value.i); break; }
                case constant_type::f32: { tab(indent); printf("value: %f,\n", value.f_32); break; }
                case constant_type::f64: { tab(indent); printf("value: %f,\n", value.f_64); break; }
                case constant_type::string: { tab(indent); printf("value: '%s',\n", value.s); break; }
                default: break;
            }

            auto child = [tab_level, tab, indent](ast* n, const char* name) {
                tab(indent);
                printf("%s: ", name);
                if (n->next) printf("[\n");
                n->debug_print(tab_level + indent + (n->next ? 1 : 0), n->next == nullptr);
                if (n->next) {
                    tab(indent);
                    printf("],\n");
                }
            };

            if (data_type) {
                tab(indent);
                printf("data_type: ");
                data_type->debug_print(tab_level + indent, true);
            }
            if (identifier) child(identifier, "identifier");
            if (property) child(property, "property");
            if (arguments) child(arguments, "arguments");
            if (lvalue) child(lvalue, "lvalue");
            if (rvalue) child(rvalue, "rvalue");
            if (callee) child(callee, "callee");
            if (destructor) child(destructor, "destructor");
            if (condition) child(condition, "condition");
            if (modifier) child(modifier, "modifier");
            if (initializer) child(initializer, "initializer");
            if (else_body) child(else_body, "else_body");
            if (body) child(body, "body");

            tab(0);
            printf("},\n");
            if (next) next->debug_print(tab_level, false, true);
        }

        void ast::set(i64 v) {
            value.i = v; c_type = constant_type::integer;
        }

        void ast::set(f32 v) {
            value.f_32 = v; c_type = constant_type::f32;
        }

        void ast::set(f64 v) {
            value.f_64 = v; c_type = constant_type::f64;
        }

        void ast::set(const std::string& v) {
            value.s = new char[v.length() + 1];
            memset(value.s, 0, v.length() + 1);
            snprintf(value.s, v.length() + 1, "%s", v.c_str());
            c_type = constant_type::string;
        }

        void ast::src(const lex::token& tk) {
            ref = tk.src;
        }

        ast::operator std::string() const { return value.s; }
        ast::operator i64() { return value.i; }
        ast::operator f32() { return value.f_32; }
        ast::operator f64() { return value.f_64; }
    };
};