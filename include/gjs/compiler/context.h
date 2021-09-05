#pragma once
#include <gjs/compiler/variable.h>
#include <gjs/compiler/tac.h>
#include <gjs/common/pipeline.h>
#include <gjs/compiler/symbol_table.h>

namespace gjs {
    class script_context;
    class type_manager;
    class compile_log;
    class script_enum;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct block_context {
            script_function* func = nullptr;
            std::list<var> named_vars;
            std::vector<var> stack_objs;
            std::vector<capture*> captures;
            parse::ast* input_ref = nullptr;
            bool is_lambda = false;
            u32 next_reg_id = 0;
            u16 func_idx = -1;
        };

        struct capture {
            u32 offset;
            var* src;
        };

        struct deferred_node {
            parse::ast* node = 0;
            script_type* subtype_replacement = 0;
        };

        struct context {
            context(compilation_output& out, script_context* env);
            ~context();

            // immediate values as vars
            var imm(u64 u);
            var imm(i64 i);
            var imm(f32 f);
            var imm(f64 d);
            var imm(const std::string& s);

            // immediate integers of specific types
            var imm(u64 u, script_type* tp);
            var imm(i64 i, script_type* tp);

            // named empty var with a new register ID (submitted to symbol table)
            var& empty_var(script_type* type, const std::string& name);

            // unnamed empty var with a new register ID
            var empty_var(script_type* type);

            // unnamed empty var with no register ID or immediate value (useful
            // for determining if a type has a method without having to use a
            // live variable)
            // ... should probably move the var::method methods to gjs::compile
            var dummy_var(script_type* type);

            // a var with no name, register id, type, or any metadata. invalid
            var null_var();

            // reference to a variable of 'error_type' to return from compilation
            // functions which encounter an error but need to return a var
            var& error_var();

            // get a named variable (or imported global variable)
            var& get_var(const std::string& name);

            // promote an unnamed variable to a named variable
            void promote_var(var& v, const std::string& name);

            // forces a var of one type to be treated as another
            var force_cast_var(const var& v, script_type* as);

            // use only to retrieve a function to be called (logs errors when not
            // found or ambiguous)
            script_function* function(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only to retrieve a function to be called (logs errors when not
            // found or ambiguous)
            script_function* function(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if
            // an identical function exists (does not log errors)
            script_function* find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // use only when searching for a forward declaration or to determine if
            // an identical function exists (does not log errors)
            script_function* find_func(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args);

            // get a declared, imported, or implicit (global) data type by name
            script_type* type(const std::string& name, bool do_throw = true);

            // get a declared, imported, or implicit (global) data type by type
            // identifier AST node. This function is also used for getting subtype
            // data types (eg. array<f32>) since type identifier AST nodes may be
            // nested to indicate such
            script_type* type(parse::ast* type_identifier);

            // returns class type if compilation is currently nested within a class
            script_type* class_tp();

            // returns current function if compilation is currently nested within a
            // function
            script_function* current_function();

            // submit a new instruction (returns fake pointer to new instruction)
            // note: don't use compilation functions as inputs for the 'operand'
            // method directly, any code they add would come /after/ the instruction
            // that the result would be an operand for
            tac_wrapper add(operation op);

            // Adds a label at the current code position and returns the label id
            label_id label();

            // number of instructions in the current instruction list (either global
            // code or function code)
            u64 code_sz() const;

            // pushes an AST node onto the node stack when it's being compiled
            void push_node(parse::ast* node);

            // pops an AST node from the node stack when it's done being compiled
            void pop_node();

            // push a new scope block onto the block stack when the scope should be
            // nested
            void push_block(script_function* f = nullptr);

            // push a new scope block onto the block stack for a lambda function
            script_function* push_lambda_block(script_type* sigTp, std::vector<var*>& captures);

            // pop the most recent scope block from the stack and destructs any stack
            // variables declared within it
            void pop_block();

            // pop the most recent scope block from the stack and destructs any stack
            // variables declared within it, except for v, which is added to the scope
            // block directly 'above' the one being popped
            void pop_block(const var& v);

            // current node being compiled
            parse::ast* node();

            // current scope block
            block_context* block();

            // shortcut to the log interface of the compilation pipeline
            compile_log* log();

            // current function block (not necessarily current block)
            block_context* cur_func_block;

            // target script context
            script_context* env;

            // abstract syntax tree to compile
            parse::ast* input;

            // types that were declared by the module being
            // compiled
            type_manager* new_types;

            // functions that were declared by the module
            // being compiled
            std::vector<script_function*> new_functions;

            // enums that were declared by the module being
            // compiled
            std::vector<script_enum*> new_enums;

            // compilation output
            compilation_output& out;

            // next lambda function id
            u32 next_lambda_id;

            // next code label id
            u32 next_label_id;

            // 'stack' of AST nodes that is kept updated with
            // the compiler's current 'position' in the AST
            std::vector<parse::ast*> node_stack;

            // 'stack' of objects that hold scope metadata.
            // new blocks are added and removed as the compiler
            // traverses nested blocks of code
            std::vector<block_context*> block_stack;

            // code that should be compiled as the last
            // step in the larger compilation process
            std::vector<deferred_node> deferred;

            // subtype classes that should be compiled
            // upon instantiation with a new type
            std::vector<parse::ast*> subtype_types;

            // subtype classes are compiled when something
            // instantiates the class with a new type, this
            // represents that type and is used while the
            // class is being compiled
            script_type* subtype_replacement;

            // whether or not a function is being compiled
            // (not including global code in the __init__
            // function)
            bool compiling_function;

            // whether or not deferred code is being compiled
            // (class methods)
            bool compiling_deferred;

            // whether or not static class properties are
            // being compiled
            bool compiling_static;

            // offset into the global code where the static
            // class properties are done being initialized
            u64 global_statics_end;

            // returned from compilation functions when an
            // error occurrs
            var error_v;

            // table for looking up symbols by name
            symbol_table symbols;
        };
    };
};