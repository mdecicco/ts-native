#include <compiler/context.h>
#include <compiler/compile.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <compile_log.h>
#include <errors.h>
#include <vm_function.h>
#include <vm_type.h>
#include <register.h>

namespace gjs {
    using namespace parse;
    using ec = error::ecode;
    using nt = ast::node_type;
    using ot = ast::operation_type;

    namespace compile {
        void function_declaration(context& ctx, ast* n) {
            ctx.push_node(n);
            ctx.push_node(n->data_type);
            vm_type* ret = ctx.type(n->data_type);
            ctx.pop_node();
            std::vector<vm_type*> arg_types;
            std::vector<std::string> arg_names;

            vm_type* method_of = ctx.class_tp();
            if (method_of) {
                arg_types.push_back(method_of);
                arg_names.push_back("this");
            }

            ast* a = n->arguments->body;
            while (a) {
                ctx.push_node(a->data_type);
                arg_types.push_back(ctx.type(a->data_type));
                ctx.pop_node();

                ctx.push_node(a->identifier);
                arg_names.push_back(*a->identifier);
                ctx.pop_node();

                a = a->next;
            }

            if (!n->body) {
                // forward declaration
                vm_function* f = ctx.find_func(*n->identifier, ret, arg_types);
                if (f) {
                    ctx.log()->err(ec::c_function_already_declared, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
                    return;
                }

                ctx.new_functions.push_back(new vm_function(ctx.env, *n->identifier, 0));
                return;
            }
            
            vm_function* f = ctx.find_func(*n->identifier, ret, arg_types);
            if (f) {
                if (f->is_host || f->access.entry) {
                    ctx.log()->err(ec::c_function_already_defined, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
                    return;
                }
            } else {
                f = new vm_function(ctx.env, *n->identifier, 0);
                u16 gp = (u16)vm_register::a0;
                u16 fp = (u16)vm_register::f0;
                for (u8 i = 0;i < arg_types.size();i++) {
                    f->arg(arg_types[i]);
                    f->signature.arg_locs.push_back((vm_register)(arg_types[i]->is_floating_point ? fp++ : gp++));
                }

                f->signature.return_type = ret;
                ctx.new_functions.push_back(f);
            }
            
            f->access.entry = ctx.code.size();

            block(ctx, n->body);

            ctx.pop_node();
        }

        var evaluate_member(context& ctx, ast* n) {
            if (n->lvalue->type == nt::identifier) {
                return ctx.get_var(*n->lvalue);
            }
        }

        var function_call(context& ctx, ast* n) {
            // callee could be either an identifier or a member operation that ultimately refers to a function
            if (n->callee->type == nt::operation && n->callee->op == ot::member) {
                if (n->callee->lvalue->type == nt::type_identifier) {
                    ctx.push_node(n->callee->lvalue);
                    vm_type* t = ctx.type(n->callee->lvalue);
                    ctx.pop_node();

                    var dummy = ctx.dummy_var(t);
                    std::vector<var> args;
                    std::vector<vm_type*> arg_types;
                    ast* a = n->arguments;
                    while (a) {
                        var arg = expression(ctx, a);
                        args.push_back(arg);
                        arg_types.push_back(arg.type());
                        a = a->next;
                    }
                    vm_function* func = dummy.method(*n->callee->rvalue, nullptr, arg_types);
                    if (func) return call(ctx, func, args);
                } else {
                    var obj = expression(ctx, n->callee->lvalue);

                    std::vector<var> args = { obj };
                    std::vector<vm_type*> arg_types = { obj.type() };
                    if (obj.type()->sub_type && obj.type()->is_host) {
                        // second parameter should be type id. This should
                        // only happen for host calls, script subtype calls
                        // should be compiled as if all occurrences of 'subtype'
                        // are the subtype used by the related instantiation
                        args.push_back(ctx.imm((u64)obj.type()->sub_type->id()));
                        arg_types.push_back(ctx.type("subtype"));
                    }

                    ast* a = n->arguments;
                    while (a) {
                        var arg = expression(ctx, a);
                        args.push_back(arg);
                        arg_types.push_back(arg.type());
                        a = a->next;
                    }
                    vm_function* func = obj.method(*n->callee->rvalue, nullptr, arg_types);
                    if (func)  return call(ctx, func, args);
                }
            } else if (n->callee->type == nt::identifier) {
                std::vector<var> args;
                std::vector<vm_type*> arg_types;
                ast* a = n->arguments;
                while (a) {
                    var arg = expression(ctx, a);
                    args.push_back(arg);
                    arg_types.push_back(arg.type());
                    a = a->next;
                }
                vm_function* func = ctx.function(*n->callee, nullptr, arg_types);
                if (func)  return call(ctx, func, args);
            }

            ctx.push_node(n->callee);
            var ret = ctx.error_var();
            ctx.pop_node();
            return ret;
        }

        std::string arg_tp_str(const std::vector<vm_type*> types) {
            std::string o = "(";
            for (u8 i = 0;i < types.size();i++) {
                if (i > 0) o += ", ";
                o += types[i]->name;
            }
            o += ")";

            return o;
        }

        var call(context& ctx, vm_function* func, const std::vector<var>& args) {
            return ctx.dummy_var(func->signature.return_type);
        }

    };
};