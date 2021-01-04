#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/compile_log.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/vm/register.h>
#include <gjs/bind/bind.h>

namespace gjs {
    using namespace parse;
    using ec = error::ecode;
    using nt = ast::node_type;
    using ot = ast::operation_type;

    namespace compile {
        script_function* function_declaration(context& ctx, ast* n) {
            ctx.push_node(n);
            script_type* ret = nullptr;
            std::vector<script_type*> arg_types;
            std::vector<std::string> arg_names;

            bool is_ctor = false;
            bool is_dtor = false;

            script_type* method_of = ctx.class_tp();
            if (method_of && !n->is_static) {
                arg_types.push_back(method_of);
                arg_names.push_back("this");

                if (is_ctor = (std::string(*n->identifier) == "constructor")) ret = method_of;
                else if (is_dtor = (std::string(*n->identifier) == "destructor")) ret = ctx.type("void");
                else {
                    ctx.push_node(n->data_type);
                    ret = ctx.type(n->data_type);
                    ctx.pop_node();
                }
            } else {
                ctx.push_node(n->data_type);
                ret = ctx.type(n->data_type);
                ctx.pop_node();
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

            std::string fname = *n->identifier;
            if (method_of) fname = method_of->name + "::" + fname;

            if (!n->body) {
                // forward declaration
                script_function* f = ctx.find_func(fname, ret, arg_types);
                if (f) {
                    ctx.log()->err(ec::c_function_already_declared, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
                    ctx.pop_node();
                    return nullptr;
                }
                
                f = new script_function(ctx.env, fname, 0);
                f->owner = ctx.out.mod;
                f->signature.return_type = ret;
                f->signature.return_loc = vm_register::v0;
                f->is_method_of = method_of;
                f->is_static = n->is_static;
                f->access.entry = 0;

                ctx.symbols.set(fname, f);
                
                for (u8 i = 0;i < arg_types.size();i++) {
                    f->arg(arg_types[i]);
                }
                ctx.new_functions.push_back(f);
                ctx.pop_node();
                return f;
            }
            
            script_function* f = ctx.find_func(fname, ret, arg_types);
            if (f) {
                if (f->is_host || f->access.entry) {
                    ctx.log()->err(ec::c_function_already_defined, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
                    ctx.pop_node();
                    return nullptr;
                }
                
                ctx.push_block(f);
                for (u8 i = 0;i < arg_types.size();i++) {
                    var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                    arg.set_arg_idx(i);
                }
            } else {
                f = new script_function(ctx.env, fname, 0);
                f->owner = ctx.out.mod;
                f->signature.return_type = ret;
                f->is_method_of = method_of;
                f->is_static = n->is_static;

                ctx.symbols.set(fname, f);

                ctx.new_functions.push_back(f);
                ctx.push_block(f);

                for (u8 i = 0;i < arg_types.size();i++) {
                    f->arg(arg_types[i]);
                    var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                    arg.set_arg_idx(i);
                }
            }

            f->access.entry = ctx.code_sz();

            block(ctx, n->body);

            if (ctx.out.code.size() == 0 || ctx.out.code.back().op != operation::ret) {
                if (is_ctor) ctx.add(operation::ret).operand(ctx.get_var("this"));
                else if (is_dtor || f->signature.return_type->size == 0) ctx.add(operation::ret);
                else ctx.log()->err(ec::c_missing_return_value, n->ref, f->name.c_str());
            }

            ctx.pop_block();
            ctx.pop_node();

            return f;
        }

        inline bool is_member_accessor(ast* n) {
            return n->type == nt::operation && n->op == ot::member;
        }

        inline bool is_identifier(ast* n) {
            return n->type == nt::identifier;
        }

        struct call_info {
            script_function* callee;
            std::vector<var> args;
            std::vector<script_type*> arg_types;
            var this_obj;
        };

        bool resolve_function(context& ctx, ast* call, call_info& out) {
            ast* callee = call->callee;
            std::vector<script_function*> funcs;
            bool was_ambiguous = false;

            // possibilities:
            // [module].[function]
            // [module].[type].[static function]
            // [type].[static function]
            // [expression | var].[function]
            // [function]

            if (is_member_accessor(callee)) {
                ast* left = callee->lvalue;
                ast* right = callee->rvalue;
                std::string func_name = *right;
                
                if (is_member_accessor(left)) {
                    ast* lleft = left->lvalue;
                    ast* lright = left->rvalue;

                    if (is_identifier(lleft) && is_identifier(lright)) {
                        std::string mod_name = *lleft;
                        std::string type_name = *lright;

                        symbol_table* mod = ctx.symbols.get_module(*lleft);
                        if (mod) {
                            symbol_table* type = mod->get_type(*lright);
                            if (type) {
                                // [module].[type].[static function]
                                out.callee = type->get_func(type->type->name + "::" + func_name, nullptr, out.arg_types);
                                return out.callee != nullptr;
                            }
                        }
                    }
                } else if (is_identifier(left)) {
                    // [type].[static function]
                    symbol_table* type = ctx.symbols.get_type(*left);
                    if (type) {
                        out.callee = type->get_func(type->type->name + "::" + func_name, nullptr, out.arg_types);
                        return out.callee != nullptr;
                    }

                    // [module].[function]
                    symbol_table* mod = ctx.symbols.get_module(*left);
                    if (mod) {
                        out.callee = mod->get_func(func_name, nullptr, out.arg_types);
                        return out.callee != nullptr;
                    }
                } else if (left->type == nt::type_identifier) {
                    script_type* type = ctx.type(left);
                    if (type) {
                        var dummy = ctx.dummy_var(type);
                        out.callee = dummy.method(func_name, nullptr, out.arg_types);
                        return out.callee != nullptr;
                    }
                }

                // [expression | var].[method]
                var expr = expression(ctx, left);
                if (expr.type()->name != "error_type") {
                    out.args.insert(out.args.begin(), expr);
                    out.arg_types.insert(out.arg_types.begin(), expr.type());
                    out.callee = expr.method(func_name, nullptr, out.arg_types);
                    out.this_obj = expr;
                    return out.callee != nullptr;
                }
            } else if (is_identifier(callee)) {
                // [function]
                out.callee = ctx.symbols.get_func(*callee, nullptr, out.arg_types);
                return out.callee != nullptr;
            }

            return false;
        }

        var function_call(context& ctx, ast* n) {
            call_info cinfo = { nullptr, {}, {}, ctx.error_var() };

            ast* a = n->arguments;
            while (a) {
                var arg = expression(ctx, a);
                cinfo.args.push_back(arg);
                cinfo.arg_types.push_back(arg.type());
                a = a->next;
            }

            if (resolve_function(ctx, n, cinfo)) {
                return call(ctx, cinfo.callee, cinfo.args);
            }

            ctx.push_node(n->callee);
            var ret = ctx.error_var();
            ctx.pop_node();
            return ret;
        }

        std::string arg_tp_str(const std::vector<script_type*> types) {
            std::string o = "(";
            for (u8 i = 0;i < types.size();i++) {
                if (i > 0) o += ", ";
                o += types[i]->name;
            }
            o += ")";

            return o;
        }

        var call(context& ctx, script_function* func, const std::vector<var>& args) {
            std::vector<var> c_args;
            for (u8 i = 0;i < args.size();i++) {
                if (i == 1 && func->is_method_of && func->is_method_of->requires_subtype && func->name.find("constructor") != std::string::npos) {
                    // don't try to convert moduletype id (u64) to subtype (data)
                    c_args.push_back(args[i]);
                } else if (func->signature.arg_types[i]->name == "subtype") {
                    c_args.push_back(args[i].convert(args[0].type()->sub_type));
                } else {
                    c_args.push_back(args[i].convert(func->signature.arg_types[i]));
                }
            }

            tac_wrapper stack_ret;
            if (func->signature.returns_on_stack) {
                stack_ret = ctx.add(operation::stack_alloc);
            }
            for (u8 i = 0;i < args.size();i++) ctx.add(operation::param).operand(c_args[i]);
            
            if (func->signature.return_type->name == "subtype") {
                if (func->is_method_of) {
                    script_type* this_tp = args[0].type();

                    if (func->signature.returns_pointer) {
                        var result = ctx.empty_var(ctx.type("data"));
                        ctx.add(operation::call).operand(result).func(func);

                        var ret = ctx.empty_var(this_tp->sub_type);
                        ret.set_mem_ptr(result);
                        ctx.add(operation::load).operand(ret).operand(result);
                        return ret;
                    } else if (func->signature.returns_on_stack) {
                        var ret = ctx.empty_var(this_tp->sub_type);
                        ret.raise_stack_flag();
                        stack_ret.operand(ret).operand(ctx.imm((u64)this_tp->sub_type->size));
                        ctx.add(operation::call).operand(ret).func(func);
                        return ret;
                    } else {
                        var ret = ctx.empty_var(this_tp->sub_type);
                        ctx.add(operation::call).operand(ret).func(func);
                        return ret;
                    }
                } else {
                    // subtype functions (global functions) are not supported yet
                    return ctx.error_var();
                }
            } else if (func->signature.return_type->size == 0) {
                ctx.add(operation::call).func(func);
                return ctx.empty_var(func->signature.return_type);
            } else {
                if (func->signature.returns_pointer) {
                    var result = ctx.empty_var(ctx.type("data"));
                    ctx.add(operation::call).operand(result).func(func);

                    var ret = ctx.empty_var(func->signature.return_type);
                    ret.set_mem_ptr(result);
                    if (func->signature.return_type->is_primitive) ctx.add(operation::load).operand(ret).operand(result);
                    else ctx.add(operation::eq).operand(ret).operand(result);
                    return ret;
                } else if (func->signature.returns_on_stack) {
                    var ret = ctx.empty_var(func->signature.return_type);
                    ret.raise_stack_flag();
                    stack_ret.operand(ret).operand(ctx.imm((u64)func->signature.return_type->size));
                    ctx.add(operation::call).operand(ret).func(func);
                    return ret;
                } else {
                    var ret = ctx.empty_var(func->signature.return_type);
                    ctx.add(operation::call).operand(ret).func(func);
                    return ret;
                }
            }

            return ctx.error_var();
        }
    };
};