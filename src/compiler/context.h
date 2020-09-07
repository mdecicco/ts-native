#pragma once
#include <compiler/variable.h>
#include <compiler/tac.h>

namespace gjs {
    class vm_context;
    class type_manager;
    class compile_log;

    namespace parse {
        struct ast;
    };

    namespace compile {
        struct context {
            vm_context* env;
            parse::ast* input;
            type_manager* new_types;
            std::vector<vm_function*> new_functions;
            std::vector<tac_instruction> code;
            u32 next_reg_id;
            std::vector<parse::ast*> node_stack;

            context();
            var imm(u64 u);
            var imm(i64 i);
            var imm(f32 f);
            var imm(f64 d);
            var imm(const std::string& s);
            var clone_var(var v);
            var empty_var(vm_type* type, const std::string& name);
            var empty_var(vm_type* type);
            var dummy_var(vm_type* type, const std::string& name);
            var dummy_var(vm_type* type);
            var null_var();
            var error_var();
            var get_var(const std::string& name);

            // use only to retrieve a function to be called (logs errors when not found or ambiguous)
            vm_function* function(const std::string& name, vm_type* ret, const std::vector<vm_type*>& args);

            // use only when searching for a forward declaration or to determine if an identical function exists (does not log errors)
            vm_function* find_func(const std::string& name, vm_type* ret, const std::vector<vm_type*>& args);

            vm_type* type(const std::string& name);
            vm_type* type(parse::ast* type_identifier);

            // returns class type if compilation is currently nested within a class
            vm_type* class_tp();

            // returns current function if compilation is currently nested within a function
            vm_function* current_function();

            tac_instruction& add(operation op);
            void push_node(parse::ast* node);
            void pop_node();
            parse::ast* node();
            compile_log* log();
        };
    };
};