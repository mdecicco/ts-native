#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/compile_log.h>
#include <gjs/common/errors.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_context.h>
#include <gjs/util/util.h>
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
                if (is_ctor = (std::string(*n->identifier) == "constructor")) ret = ctx.type("void");
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
                f->signature.is_thiscall = method_of != nullptr;
                f->signature.return_type = ret;
                f->signature.returns_pointer = is_ctor;
                f->signature.returns_on_stack = !is_ctor && !ret->is_primitive && ret->size > 0;
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
            var* self = nullptr;
            if (f) {
                if (f->is_host || f->access.entry) {
                    ctx.log()->err(ec::c_function_already_defined, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types));
                    ctx.pop_node();
                    return nullptr;
                }
                
                ctx.push_block(f);

                u32 base_idx = 0;
                if (f->is_method_of) {
                    // implicit 'this' argument
                    self = &ctx.empty_var(f->is_method_of, "this");
                    self->set_arg_idx(base_idx++);
                }

                if (f->signature.returns_on_stack) {
                    // implicit return pointer argument
                    var& ret_ptr = ctx.empty_var(f->signature.return_type, "@ret");
                    ret_ptr.set_arg_idx(base_idx++);
                }

                for (u8 i = 0;i < arg_types.size();i++) {
                    var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                    arg.set_arg_idx(base_idx + i);
                }
            } else {
                f = new script_function(ctx.env, fname, ctx.code_sz());
                f->owner = ctx.out.mod;
                f->signature.is_thiscall = method_of != nullptr;
                f->signature.return_type = ret;
                f->signature.returns_pointer = is_ctor;
                f->signature.returns_on_stack = !is_ctor && !ret->is_primitive && ret->size > 0;
                f->is_method_of = method_of;
                f->is_static = n->is_static;
                f->access.entry = 0;

                ctx.symbols.set(fname, f);

                ctx.new_functions.push_back(f);
                ctx.push_block(f);

                u32 base_idx = 0;
                if (f->is_method_of) {
                    // implicit 'this' argument
                    self = &ctx.empty_var(f->is_method_of, "this");
                    self->set_arg_idx(base_idx++);
                }

                if (f->signature.returns_on_stack) {
                    // implicit return pointer argument
                    var& ret_ptr = ctx.empty_var(f->signature.return_type, "@ret");
                    ret_ptr.set_arg_idx(base_idx++);
                }

                for (u8 i = 0;i < arg_types.size();i++) {
                    f->arg(arg_types[i]);
                    var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                    arg.set_arg_idx(base_idx + i);
                }
            }


            if (is_ctor) {
                std::vector<std::string> initializedNames;
                if (n->initializer) {
                    ast* i = n->initializer;
                    while (i) {
                        ctx.push_node(i);
                        std::string pname = *i->identifier;
                        bool found = false;
                        for (u32 in = 0;in < initializedNames.size();in++) {
                            if (initializedNames[in] == pname) {
                                ctx.log()->err(ec::c_prop_already_initialized, i->ref, pname.c_str());
                                found = true;
                                break;
                            }
                        }

                        if (found) continue;

                        initializedNames.push_back(pname);
                        var ptr = self->prop_ptr(pname);
                        std::vector<var> args;
                        ast* arg = i->arguments;
                        while (arg) {
                            args.push_back(expression(ctx, arg));
                            arg = arg->next;
                        }

                        if (ptr.type()->is_primitive && args.size() == 1) {
                            ctx.add(operation::store).operand(ptr).operand(args[0]);
                        } else {
                            construct_in_place(ctx, ptr, args);
                        }
                        i = i->next;
                        ctx.pop_node();
                    }

                    for (u32 i = 0;i < f->is_method_of->properties.size();i++) {
                        auto& p = f->is_method_of->properties[i];
                        bool found = false;
                        for (u32 in = 0;in < initializedNames.size();in++) {
                            if (initializedNames[in] == p.name) {
                                found = true;
                                break;
                            }
                        }

                        if (found) continue;

                        /*
                        bool found_default = false;
                        for (u32 m = 0;m < p.type->methods.size();m++) {
                            if (p.type->methods[m]->name.find("::constructor") != std::string::npos) {
                                if (p.type->methods[m]->signature.arg_types.size() == 0) {
                                    found_default = true;
                                    break;
                                }
                            }
                        }
                        */

                        if (!p.type->method<void>("constructor")) {
                            ctx.log()->err(ec::c_prop_has_no_default_ctor, n->initializer->ref, p.name.c_str(), p.type->name.c_str(), f->is_method_of->name.c_str());
                        }
                    }
                }
            }

            block(ctx, n->body);

            if (ctx.out.code.size() == 0 || ctx.out.code.back().op != operation::ret) {
                if (is_dtor) {
                    add_implicit_destructor_code(ctx, n, f->is_method_of);
                }

                if (f->signature.return_type->size == 0) {
                    ctx.add(operation::ret);
                }
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
                    if (!expr.has_unambiguous_method(func_name, nullptr, out.arg_types)) {
                        auto arg_types = out.arg_types;
                        arg_types.insert(arg_types.begin(), expr.type());
                        if (expr.has_unambiguous_method(func_name, nullptr, arg_types)) {
                            out.callee = expr.method(func_name, nullptr, arg_types);
                            if (out.callee && out.callee->is_static) {
                                out.arg_types.insert(out.arg_types.begin(), expr.type());
                                out.args.insert(out.args.begin(), expr);
                                return true;
                            }
                            out.callee = nullptr;
                        }
                    }

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
                var* self = cinfo.this_obj.type()->name == "error_type" ? nullptr : &cinfo.this_obj;
                return call(ctx, cinfo.callee, cinfo.args, self);
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

        // todo: change the following
        // public host call interface [script_context::call]
        // function binding
        var call(context& ctx, script_function* func, const std::vector<var>& args, const var* self) {
            // get return value
            script_type* rtp = func->signature.return_type;
            if (rtp->name == "subtype") rtp = self->type()->sub_type;
            var stack_ret = ctx.empty_var(rtp);

            if (self && !func->is_static) {
                // 'this' always gets passed first
                ctx.add(operation::param).operand(*self).func(func);
            }

            if (func->signature.is_subtype_obj_ctor) {
                // pass moduletype after 'this' to represent subtype
                u64 moduletype = join_u32(self->type()->sub_type->owner->id(), self->type()->sub_type->id());
                ctx.add(operation::param).operand(ctx.imm(moduletype)).func(func);
            }

            if (func->signature.returns_on_stack) {
                // pass pointer to return value
                ctx.add(operation::stack_alloc).operand(stack_ret).operand(ctx.imm((u64)rtp->size));
                stack_ret.raise_stack_flag();
                ctx.add(operation::param).operand(stack_ret).func(func);
            }

            // pass args
            for (u8 i = 0;i < args.size();i++) {
                if (func->signature.arg_types[i]->name == "subtype") {
                    var p = args[i].convert(self->type()->sub_type);
                    ctx.add(operation::param).operand(p).func(func);
                } else {
                    var p = args[i].convert(func->signature.arg_types[i]);
                    ctx.add(operation::param).operand(p).func(func);
                }
            }

            // void return
            if (rtp->size == 0) {
                // no return
                ctx.add(operation::call).func(func);
                return ctx.empty_var(rtp);
            }
            
            // pointer return
            if (func->signature.returns_pointer) {
                var result = ctx.empty_var(rtp);
                ctx.add(operation::call).operand(result).func(func);

                if (rtp->is_primitive) {
                    // load value from pointer
                    var ret = ctx.empty_var(rtp);
                    ret.set_mem_ptr(result);
                    ctx.add(operation::load).operand(ret).operand(result);
                    return ret;
                }
                else {
                    // just return the pointer
                    return result;
                }
            }

            // stack return
            if (func->signature.returns_on_stack) {
                // callee will construct return value in $stack_ret (or copy it to $stack_ret),
                // which is passed as an argument
                ctx.add(operation::call).func(func);
                return stack_ret;
            }

            // basic return
            var ret = ctx.empty_var(rtp);
            ctx.add(operation::call).operand(ret).func(func);
            return ret;
        }
    };
};