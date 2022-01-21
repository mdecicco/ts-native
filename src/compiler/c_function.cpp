#include <gjs/compiler/context.h>
#include <gjs/compiler/compile.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/compile_log.h>
#include <gjs/common/errors.h>
#include <gjs/vm/register.h>

#include <gjs/gjs.hpp>

namespace gjs {
    using namespace parse;
    using ec = error::ecode;
    using nt = ast::node_type;
    using ot = ast::operation_type;

    namespace compile {
        bool is_var_captured(const std::string& vname, parse::ast* root, bool in_lambda) {
            if (root->type == nt::lambda_expression) {
                // if lambda has an argument with a name that shadows decl, then decl is not captured
                ast* a = root->arguments->body;
                while (a) {
                    if (std::string(*a->identifier) == vname) {
                        return false;
                    }
                    a = a->next;
                }

                // scan lambda for usages of decl->identifier
                return is_var_captured(vname, root->body, true);
            }

            if (root->type == nt::variable_declaration) {
                // if a variable is declared with a name that shadows decl, then decl is not captured
                if (std::string(*root->identifier) == vname) {
                    return false;
                }

                if (root->initializer) return is_var_captured(vname, root->initializer, in_lambda);
                return false;
            }

            if (in_lambda && root->type == nt::identifier) {
                // decl is used inside a lambda, it is captured
                if (std::string(*root) == vname) {
                    return true;
                }
            }

            if (in_lambda && root->arguments && is_var_captured(vname, root->arguments, in_lambda)) return true;
            if (in_lambda && root->initializer && is_var_captured(vname, root->initializer, in_lambda)) return true;
            if (in_lambda && root->condition && is_var_captured(vname, root->condition, in_lambda)) return true;
            if (in_lambda && root->modifier && is_var_captured(vname, root->modifier, in_lambda)) return true;
            if (in_lambda && root->lvalue && is_var_captured(vname, root->lvalue, in_lambda)) return true;
            if (in_lambda && root->rvalue && is_var_captured(vname, root->rvalue, in_lambda)) return true;

            parse::ast* n = nullptr;

            n = root->body;
            while (n) {
                if (is_var_captured(vname, n, in_lambda)) return true;
                n = n->next;
            }

            n = root->else_body;
            while (n) {
                if (is_var_captured(vname, n, in_lambda)) return true;
                n = n->next;
            }

            n = root->next;
            while (n) {
                if (is_var_captured(vname, n, in_lambda)) return true;
                n = n->next;
            }

            return false;
        }

        script_function* function_declaration(context& ctx, ast* n) {
            ctx.push_node(n);
            script_type* ret = nullptr;
            std::vector<script_type*> arg_types;
            std::vector<std::string> arg_names;

            bool is_ctor = false;
            bool is_dtor = false;

            script_type* method_of = ctx.class_tp();
            if (method_of && !n->is_static) {
                if ((is_ctor = (std::string(*n->identifier) == "constructor"))) ret = ctx.type("void");
                else if ((is_dtor = (std::string(*n->identifier) == "destructor"))) ret = ctx.type("void");
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
                    ctx.log()->err(ec::c_function_already_declared, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types).c_str());
                    ctx.pop_node();
                    return nullptr;
                }

                function_signature sig(ctx.env, ret, is_ctor, arg_types.data(), (u8)arg_types.size(), method_of, is_ctor, n->is_static);
                script_type* tp = ctx.type(sig.to_string(), false);
                if (!tp) {
                    tp = ctx.new_types->get(sig);
                    ctx.symbols.set(tp->name, tp);
                }
                
                f = new script_function(ctx.env, fname, 0, tp, method_of, ctx.out.mod);
                f->owner = ctx.out.mod;
                f->is_thiscall = method_of != nullptr;
                f->is_method_of = method_of;
                f->is_static = n->is_static;
                f->access.entry = 0;

                ctx.symbols.set(fname, f);
                ctx.new_functions.push_back(f);
                ctx.pop_node();
                return f;
            }
            
            script_function* f = ctx.find_func(fname, ret, arg_types);
            var* self = nullptr;
            if (f) {
                if (f->is_host || f->access.entry) {
                    ctx.log()->err(ec::c_function_already_defined, n->ref, ret->name.c_str(), f->name.c_str(), arg_tp_str(arg_types).c_str());
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

                if (f->type->signature->returns_on_stack) {
                    // implicit return pointer argument
                    var& ret_ptr = ctx.empty_var(f->type->signature->return_type, "@ret");
                    ret_ptr.set_arg_idx(base_idx++);
                }

                for (u8 i = 0;i < arg_types.size();i++) {
                    var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                    arg.set_arg_idx(base_idx + i);
                }
            } else {
                function_signature sig(ctx.env, ret, is_ctor, arg_types.data(), (u8)arg_types.size(), method_of, is_ctor, n->is_static);
                script_type* tp = ctx.type(sig.to_string(), false);
                if (!tp) {
                    tp = ctx.new_types->get(sig);
                    ctx.symbols.set(tp->name, tp);
                }

                f = new script_function(ctx.env, fname, ctx.code_sz(), tp, method_of, ctx.out.mod);
                f->owner = ctx.out.mod;
                f->is_thiscall = method_of != nullptr;
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

                if (f->type->signature->returns_on_stack) {
                    // implicit return pointer argument
                    var& ret_ptr = ctx.empty_var(f->type->signature->return_type, "@ret");
                    ret_ptr.set_arg_idx(base_idx++);
                }

                for (u8 i = 0;i < arg_types.size();i++) {
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
                            if (p.type->methods[m]->type->signature->explicit_argc == 0) {
                                found_default = true;
                                break;
                            }
                        }
                    }
                    */

                    if (!p.type->is_primitive) {
                        if (!p.type->method<void>("constructor")) {
                            ctx.log()->err(ec::c_prop_has_no_default_ctor, n->initializer->ref, p.name.c_str(), p.type->name.c_str(), f->is_method_of->name.c_str());
                        }
                    }
                }
            }

            block(ctx, n->body, false);

            if (ctx.out.funcs[ctx.cur_func_block->func_idx].code.size() == 0 || ctx.out.funcs[ctx.cur_func_block->func_idx].code.back().op != operation::ret) {
                if (is_dtor) {
                    add_implicit_destructor_code(ctx, n, f->is_method_of);
                }

                if (f->type->signature->return_type->size == 0) {
                    ctx.add(operation::ret);
                }
                else ctx.log()->err(ec::c_missing_return_value, n->ref, f->name.c_str());
            }

            ctx.pop_block();
            ctx.pop_node();

            return f;
        }

        var lambda_expression(context& ctx, parse::ast* n) {
            ctx.push_node(n);
            script_type* ret = nullptr;
            std::vector<script_type*> arg_types;
            std::vector<std::string> arg_names;
            std::vector<var*> captures;
            robin_hood::unordered_flat_set<std::string> captured;
            u64 captureDataSize = 0;
            if (ctx.cur_func_block) {
                for (auto b = ctx.block_stack.rbegin();b != ctx.block_stack.rend();b++) {
                    for (auto& nv : (*b)->named_vars) {
                        if (captured.count(nv.name())) {
                            // already captured from more recent scope
                            continue;
                        }

                        if (is_var_captured(nv.name(), n, false)) {
                            captures.push_back(&nv);
                            captured.insert(nv.name());
                            captureDataSize += nv.type()->size;
                        }
                    }

                    if (!(*b)->is_lambda && (*b)->func) {
                        // going beyond this point would allow capturing global variables, which
                        // is not necessary since they can be accessed already
                        break;
                    }
                }
            }

            ctx.push_node(n->data_type);
            ret = ctx.type(n->data_type);
            ctx.pop_node();

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

            function_signature sig(ctx.env, ret, !ret->is_primitive, arg_types.data(), (u8)arg_types.size(), nullptr, false, false, true);
            script_type* sigTp = ctx.env->types()->get(sig);

            script_function* fn = ctx.push_lambda_block(sigTp, captures);

            u8 base_idx = 1; // because '$ctx' is always the first argument of a lambda
            for (u8 i = 0;i < arg_types.size();i++) {
                var& arg = ctx.empty_var(arg_types[i], arg_names[i]);
                arg.set_arg_idx(base_idx + i);
            }

            block(ctx, n->body, false);

            if (ctx.out.funcs[ctx.cur_func_block->func_idx].code.size() == 0 || ctx.out.funcs[ctx.cur_func_block->func_idx].code.back().op != operation::ret) {
                if (fn->type->signature->return_type->size == 0) {
                    ctx.add(operation::ret);
                }
                else ctx.log()->err(ec::c_missing_return_value, n->ref, fn->name.c_str());
            }
            ctx.pop_block();

            var out = ctx.empty_var(sigTp);
            out.raise_stack_flag();
            ctx.add(operation::stack_alloc).operand(out).operand(ctx.imm(out.size()));

            if (captures.size() > 0) {
                auto dt = ctx.type("data");
                auto u32t = ctx.type("u32");
                auto u64t = ctx.type("u64");

                captureDataSize += sizeof(u32); // number of captures
                captureDataSize += sizeof(u64) * captures.size(); // moduletype ids of captures
                var dataSz = ctx.imm((u64)captureDataSize);
                var ctxPtr = call(
                    ctx,
                    ctx.function("alloc", dt, { u32t }),
                    { dataSz }
                );

                u32 off = (u32)sizeof(u32) + (u32)(sizeof(u64) * captures.size());
                ctx.add(operation::store).operand(ctx.force_cast_var(ctxPtr, u32t)).operand(ctx.imm((u64)captures.size()));

                var copy = ctx.empty_var(dt);
                var typeIdPtr = ctx.empty_var(dt);
                for (u16 c = 0;c < captures.size();c++) {
                    script_type* tp = captures[c]->type();
                    var copyTyped = ctx.force_cast_var(copy, tp);
                    var typeIdu32 = ctx.force_cast_var(typeIdPtr, u64t);

                    ctx.add(operation::uadd).operand(typeIdu32).operand(ctxPtr).operand(ctx.imm((u64)(sizeof(u32) + (sizeof(u64) * c))));
                    ctx.add(operation::store).operand(typeIdu32).operand(ctx.imm(join_u32(tp->owner->id(), tp->id())));

                    ctx.add(operation::uadd).operand(copyTyped).operand(ctxPtr).operand(ctx.imm((u64)off));
                    construct_in_place(ctx, copyTyped, { *captures[c] });
                    off += tp->size;
                }

                var ptr = call(
                    ctx,
                    ctx.function("$makefunc", dt, { u32t, dt, u64t }),
                    { ctx.imm((u64)fn->id()), ctxPtr, dataSz }, // function id, context data size
                    nullptr
                );
                ctx.add(operation::store).operand(out).operand(ptr);
            } else {
                auto dt = ctx.type("data");
                var dataPtr = ctx.empty_var(dt);
                ctx.add(operation::eq).operand(dataPtr).operand(ctx.imm((u64)0));
                var ptr = call(
                    ctx,
                    ctx.function("$makefunc", dt, { ctx.type("u32"), dt, ctx.type("u64") }),
                    { ctx.imm((u64)fn->id()), dataPtr, ctx.imm((u64)0) }, // function id, context data size
                    nullptr
                );
                ctx.add(operation::store).operand(out).operand(ptr);
            }

            ctx.pop_node();
            return out;
        }

        inline bool is_member_accessor(ast* n) {
            return n->type == nt::operation && n->op == ot::member;
        }

        inline bool is_identifier(ast* n) {
            return n->type == nt::identifier;
        }

        struct call_info {
            script_function* callee;
            var callee_v;
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
            // [module].[function pointer]
            // [module].[type].[static function]
            // [module].[type].[static function pointer]
            // [type].[static function]
            // [type].[static function pointer]
            // [expression | var].[function]
            // [expression | var].[function pointer]
            // [function]
            // [function pointer]

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
                                // [module].[type].[static function pointer]
                                var* cv = type->get_var(func_name);
                                if (cv) out.callee_v = *cv;

                                // [module].[type].[static function]
                                out.callee = type->get_func(type->type->name + "::" + func_name, nullptr, out.arg_types, cv == nullptr);
                                return out.callee != nullptr || cv != nullptr;
                            }
                        }
                    }
                } else if (is_identifier(left)) {
                    symbol_table* type = ctx.symbols.get_type(*left);
                    if (type) {
                        // [type].[static function pointer]
                        var* cv = type->get_var(func_name);
                        if (cv) out.callee_v = *cv;

                        // [type].[static function]
                        out.callee = type->get_func(type->type->name + "::" + func_name, nullptr, out.arg_types, cv == nullptr);
                        return out.callee != nullptr || cv != nullptr;
                    }

                    symbol_table* mod = ctx.symbols.get_module(*left);
                    if (mod) {
                        // [module].[function pointer]
                        var* cv = mod->get_var(func_name);
                        if (cv) out.callee_v = *cv;

                        // [module].[function]
                        out.callee = mod->get_func(func_name, nullptr, out.arg_types, cv == nullptr);
                        return out.callee != nullptr || cv != nullptr;
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
                    // [expression | var].[function pointer]
                    out.callee_v = expr.prop(func_name, false);
                    bool found = out.callee_v.type() != ctx.type("error_type");

                    if (!expr.has_unambiguous_method(func_name, nullptr, out.arg_types)) {
                        auto arg_types = out.arg_types;
                        arg_types.insert(arg_types.begin(), expr.type());
                        if (expr.has_unambiguous_method(func_name, nullptr, arg_types)) {
                            out.callee = expr.method(func_name, nullptr, arg_types, !found);
                            if (out.callee && out.callee->is_static) {
                                out.arg_types.insert(out.arg_types.begin(), expr.type());
                                out.args.insert(out.args.begin(), expr);
                                return true;
                            }
                            out.callee = nullptr;
                        }
                    }

                    out.callee = expr.method(func_name, nullptr, out.arg_types, !found);
                    out.this_obj = expr;
                    return out.callee != nullptr || found;
                }
            } else if (is_identifier(callee)) {
                // [function]
                out.callee = ctx.symbols.get_func(*callee, nullptr, out.arg_types, false);

                if (!out.callee) {
                    // [function pointer]
                    out.callee_v = ctx.get_var(*callee);
                    bool found = out.callee_v.type() != ctx.type("error_type");
                    if (!found) {
                        out.callee = ctx.symbols.get_func(*callee, nullptr, out.arg_types, true);
                        return out.callee != nullptr;
                    }
                    return true;
                }

                return out.callee != nullptr;
            }

            return false;
        }

        var function_call(context& ctx, ast* n) {
            call_info cinfo = { nullptr, ctx.error_var(), {}, {}, ctx.error_var() };

            ast* a = n->arguments;
            while (a) {
                var arg = expression(ctx, a);
                cinfo.args.push_back(arg);
                cinfo.arg_types.push_back(arg.type());
                a = a->next;
            }

            if (resolve_function(ctx, n, cinfo)) {
                var* self = cinfo.this_obj.type()->name == "error_type" ? nullptr : &cinfo.this_obj;
                if (cinfo.callee) return call(ctx, cinfo.callee, cinfo.args, self);
                else return call(ctx, cinfo.callee_v, cinfo.args, self);
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

        var call(context& ctx, script_function* func, const std::vector<var>& args, const var* self) {
            // get return value
            script_type* rtp = func->type->signature->return_type;
            if (rtp->name == "subtype") rtp = self->type()->sub_type;
            var stack_ret = ctx.empty_var(rtp);

            if (func->type->signature->returns_on_stack) {
                stack_ret.raise_stack_flag();
                ctx.add(operation::stack_alloc).operand(stack_ret).operand(ctx.imm((u64)rtp->size));
            }

            // pass args
            for (u8 i = 0;i < func->type->signature->args.size();i++) {
                if (func->type->signature->args[i].implicit == function_signature::argument::implicit_type::this_ptr) {
                    ctx.add(operation::param).operand(*self).func(func);
                    continue;
                }
                if (func->type->signature->args[i].implicit == function_signature::argument::implicit_type::moduletype_id) {
                    u64 moduletype = join_u32(self->type()->sub_type->owner->id(), self->type()->sub_type->id());
                    ctx.add(operation::param).operand(ctx.imm(moduletype)).func(func);
                    continue;
                }
                if (func->type->signature->args[i].implicit == function_signature::argument::implicit_type::ret_addr) {
                    ctx.add(operation::param).operand(stack_ret).func(func);
                    continue;
                }

                if (func->type->signature->args[i].tp->name == "subtype") {
                    var p = args[i - func->type->signature->implicit_argc].convert(self->type()->sub_type);
                    ctx.add(operation::param).operand(p).func(func);
                } else {
                    var p = args[i - func->type->signature->implicit_argc].convert(func->type->signature->args[i].tp);
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
            if (func->type->signature->returns_pointer) {
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
            if (func->type->signature->returns_on_stack) {
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

        var call(context& ctx, var func, const std::vector<var>& args, const var* self) {
            function_signature* sig = func.type()->signature;
            if (sig) {
                // validate call signature
                
                // match argument types
                if (sig->explicit_argc != args.size()) {
                    ctx.log()->err(ec::c_call_signature_mismatch, ctx.node()->ref, sig->to_string().c_str());
                    return ctx.error_var();
                }

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (sig->explicit_arg(i).tp->id() != args[i].type()->id()) {
                        match = false;
                        break;
                    }
                }

                if (!match) {
                    // check if the arguments are at least convertible
                    match = true;
                    for (u8 i = 0;i < args.size();i++) {
                        if (!has_valid_conversion(ctx, args[i].type(), sig->explicit_arg(i).tp)) {
                            match = false;
                            break;
                        }
                    }

                    if (!match) {
                        ctx.log()->err(ec::c_call_signature_mismatch, ctx.node()->ref, sig->to_string().c_str());
                        return ctx.error_var();
                    }
                }
            } else {
                ctx.log()->err(ec::c_object_not_callable, ctx.node()->ref, func.type()->name.c_str());
                return ctx.error_var();
            }

            // get return value
            script_type* rtp = sig->return_type;
            if (rtp->name == "subtype") rtp = self->type()->sub_type;
            var stack_ret = ctx.empty_var(rtp);

            if (sig->returns_on_stack) {
                stack_ret.raise_stack_flag();
                ctx.add(operation::stack_alloc).operand(stack_ret).operand(ctx.imm((u64)rtp->size));
            }

            // pass args
            for (u8 i = 0;i < sig->args.size();i++) {
                if (sig->args[i].implicit == function_signature::argument::implicit_type::capture_data_ptr) {
                    // func = raw_pointer**
                    var captureDataPtr = ctx.empty_var(ctx.type("data"));
                    
                    // captureDataPtr = *func;
                    ctx.add(operation::load).operand(captureDataPtr).operand(func);

                    // captureDataPtr = (function_pointer**)&((raw_callback*)captureDataPtr)->ptr
                    ctx.add(operation::uadd).operand(captureDataPtr).operand(captureDataPtr).operand(ctx.imm((u64)offsetof(raw_callback, ptr)));

                    // captureDataPtr = (function_pointer*)*captureDataPtr
                    ctx.add(operation::load).operand(captureDataPtr).operand(captureDataPtr);

                    // captureDataPtr = (void**)&((function_pointer*)captureDataPtr)->data
                    ctx.add(operation::uadd).operand(captureDataPtr).operand(captureDataPtr).operand(ctx.imm((u64)offsetof(function_pointer, data)));

                    // captureDataPtr = (void*)*captureDataPtr
                    ctx.add(operation::load).operand(captureDataPtr).operand(captureDataPtr);

                    // ayyyy
                    ctx.add(operation::param).operand(captureDataPtr).func(func);
                    continue;
                }
                if (sig->args[i].implicit == function_signature::argument::implicit_type::this_ptr) {
                    ctx.add(operation::param).operand(*self).func(func);
                    continue;
                }
                if (sig->args[i].implicit == function_signature::argument::implicit_type::moduletype_id) {
                    u64 moduletype = join_u32(self->type()->sub_type->owner->id(), self->type()->sub_type->id());
                    ctx.add(operation::param).operand(ctx.imm(moduletype)).func(func);
                    continue;
                }
                if (sig->args[i].implicit == function_signature::argument::implicit_type::ret_addr) {
                    ctx.add(operation::param).operand(stack_ret).func(func);
                    continue;
                }

                if (sig->args[i].tp->name == "subtype") {
                    var p = args[i - sig->implicit_argc].convert(self->type()->sub_type);
                    ctx.add(operation::param).operand(p).func(func);
                } else {
                    var p = args[i - sig->implicit_argc].convert(sig->args[i].tp);
                    ctx.add(operation::param).operand(p).func(func);
                }
            }

            // void return
            if (rtp->size == 0) {
                // no return
                ctx.add(operation::call).operand(func).func(func);
                return ctx.empty_var(rtp);
            }

            // pointer return
            if (sig->returns_pointer) {
                var result = ctx.empty_var(rtp);
                ctx.add(operation::call).operand(func).operand(result).func(func);

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
            if (sig->returns_on_stack) {
                // callee will construct return value in $stack_ret (or copy it to $stack_ret),
                // which is passed as an argument
                ctx.add(operation::call).operand(func).func(func);
                return stack_ret;
            }

            // basic return
            var ret = ctx.empty_var(rtp);
            ctx.add(operation::call).operand(func).operand(ret).func(func);
            return ret;
        }
    };
};
