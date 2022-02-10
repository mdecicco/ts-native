#include <gjs/compiler/context.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/type_manager.h>
#include <gjs/common/script_function.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/errors.h>
#include <gjs/common/warnings.h>
#include <gjs/compiler/compile.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/common/script_module.h>
#include <gjs/common/function_pointer.h>
#include <gjs/util/util.h>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;

    namespace compile {
        context::context(compilation_output& _out, script_context* _env) : out(_out), symbols(this), env(_env) {
            next_lambda_id = 0;
            next_label_id = 1;
            cur_func_block = nullptr;
            input = nullptr;
            new_types = nullptr;
            subtype_replacement = nullptr;
            compiling_function = false;
            compiling_deferred = false;
            compiling_static = false;
            global_statics_end = 0;
            error_v = var();
            error_v.m_type = env->global()->types()->get("error_type");
            error_v.m_ctx = this;
        }

        context::~context() {
            symbols.clear();
        }

        var context::imm(u64 u) {
            return var(this, u);
        }

        var context::imm(i64 i) {
            return var(this, i);
        }

        var context::imm(f32 f) {
            return var(this, f);
        }

        var context::imm(f64 d) {
            return var(this, d);
        }

        var context::imm(const std::string& s) {
            u64 off = out.mod->data()->position();
            out.mod->data()->write((void*)s.c_str(), s.length());

            var d = empty_var(type("data"));
            add(operation::module_data).operand(d).operand(imm((u64)out.mod->id())).operand(imm(off));

            var v = empty_var(type("string"));
            v.add_to_stack();
            construct_on_stack(*this, v, { d, imm((u64)s.length()) });

            return v;
            // todo string immediates
            // - Needs to allow string operations to be calculated ahead of time
            // - If used by compiled code, current value of immediate must be constructed
            //   using the above code
            // return var(this, s);
        }

        var context::imm(u64 u, script_type* tp) {
            var v = var(this, u);
            v.m_type = tp;
            return v;
        }

        var context::imm(i64 i, script_type* tp) {
            var v = var(this, i);
            v.m_type = tp;
            return v;
        }

        var& context::empty_var(script_type* type, const std::string& name) {
            var v = var(this, cur_func_block->next_reg_id++, type);
            v.m_name = name;
            block()->named_vars.push_back(v);
            symbols.set(name, &block()->named_vars.back());
            return block()->named_vars.back();
        }

        var context::empty_var(script_type* type) {
            return var(this, cur_func_block->next_reg_id++, type);
        }

        var context::dummy_var(script_type* type) {
            var v = var();
            v.m_ctx = this;
            v.m_instantiation = node()->ref;
            v.m_type = type;
            return v;
        }

        var context::null_var() {
            return var();
        }

        var& context::error_var() {
            return error_v;
        }

        var& context::get_var(const std::string& name) {
            symbol_list* syms = symbols.get(name);
            if (syms) {
                const script_module::local_var* mv = nullptr;
                script_function* fn = nullptr;
                u32 fn_count = 0;

                for (auto s = syms->symbols.rbegin();s != syms->symbols.rend();s++) {
                    switch (s->sym_type()) {
                        case symbol::symbol_type::st_capture: {
                            capture* cap = s->get_capture();
                            var& v = empty_var(cap->src->type(), name);
                            // load the value from the context
                            var& ctx = get_var("$ctx");
                            var ptr = empty_var(cap->src->type());
                            add(operation::uadd).operand(ptr).operand(ctx).operand(imm(u64(cap->offset)));
                            add(operation::load).operand(v).operand(ptr);
                            v.set_mem_ptr(ptr);
                            return v;
                            break;
                        }
                        case symbol::symbol_type::st_var: {
                            if (s->scope_idx() != 0 || !compiling_function || name[0] == '@') {
                                return *s->get_var();
                            }
                            break;
                        }
                        case symbol::symbol_type::st_function: {
                            fn = s->get_func();
                            fn_count++;
                            break;
                        }
                        case symbol::symbol_type::st_modulevar: {
                            if (mv) break;
                            mv = s->get_modulevar();
                            break;
                        }
                        default: { break; }
                    }
                }

                if (mv) {
                    // only declare the module variable as a local variable if it hasn't already been
                    var& v = empty_var(mv->type, name);
                    v.m_instantiation = mv->ref;

                    if (mv->type->is_primitive) {
                        var ptr = empty_var(type("data"));
                        add(operation::module_data).operand(ptr).operand(imm((u64)mv->owner->id())).operand(imm(mv->offset));
                        v.set_mem_ptr(ptr);
                        add(operation::load).operand(v).operand(ptr);
                    } else {
                        add(operation::module_data).operand(v).operand(imm((u64)mv->owner->id())).operand(imm(mv->offset));
                    }

                    return v;
                }

                if (fn) {
                    if (fn_count > 1) {
                        log()->err(ec::c_ambiguous_function_name, node()->ref, name.c_str());
                    }

                    return func_var(fn);
                }
            }

            log()->err(ec::c_undefined_identifier, node()->ref, name.c_str());
            return error_var();
        }

        var& context::func_var(script_function* fn) {
            // check first if the function was already stored in a variable
            symbol_list* syms = symbols.get("@fv_" + fn->name);
            if (syms) {
                const script_module::local_var* mv = nullptr;
                for (auto s = syms->symbols.rbegin();s != syms->symbols.rend();s++) {
                    switch (s->sym_type()) {
                        case symbol::symbol_type::st_var: {
                            // it was
                            return *s->get_var();
                        }
                        default: { break; }
                    }
                }
            }

            var& outv = empty_var(fn->type, "@fv_" + fn->name);
            outv.add_to_stack();
            add(operation::stack_alloc).operand(outv).operand(imm((u64)fn->type->size));
            auto dt = type("data");
            var dataPtr = empty_var(dt);
            add(operation::eq).operand(dataPtr).operand(imm((u64)0));
            var ptr = call(
                *this,
                function("$makefunc", dt, { type("u32"), dt, type("u64") }),
                { imm((u64)fn->id()), dataPtr, imm((u64)0) }, // function id, context data size
                nullptr
            );

            if (fn->id() == 0) {
                // fn->id() will be 0 until the function is added to the context...
                // first param instruction must be found and updated
                auto& code = out.funcs[cur_func_block->func_idx].code;
                u8 pidx = 3;
                for (address c = code.size() - 1;c > 0;c--) {
                    if (code[c].op == operation::param) {
                        pidx--;
                        if (pidx == 0) {
                            // found it
                            code[c].resolve(0, fn);
                            break;
                        }
                    }
                }
            }

            add(operation::store).operand(outv).operand(ptr);

            return outv;
        }
        
        void context::promote_var(var& v, const std::string& name) {
            if (v.name().length() > 0) {
                // exception, programming error
            }

            v.m_name = name;
            block()->named_vars.push_back(v);
            symbols.set(name, &block()->named_vars.back());
        }

        var context::force_cast_var(const var& v, script_type* as) {
            return var(this, v.reg_id(), as);
        }

        script_function* context::function(const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            return symbols.get_func(name, ret, args);
        }

        script_function* context::function(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            symbol_table* mod = symbols.get_module(from_aliased_import);
            if (!mod) return nullptr;
            return mod->get_func(name, ret, args);
        }

        script_function* context::find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            return symbols.get_func_strict(name, ret, args, false);
        }

        script_function* context::find_func(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            symbol_table* mod = symbols.get_module(from_aliased_import);
            if (!mod) return nullptr;
            return mod->get_func_strict(name, ret, args, false);
        }

        script_type* context::type(const std::string& name, bool do_throw) {
            if (name == "subtype") {
                if (subtype_replacement) return subtype_replacement;
                else {
                    log()->err(ec::c_invalid_subtype_use, node()->ref);
                    return env->global()->types()->get("error_type");
                }
            }

            symbol_table* t = symbols.get_type(name);

            if (!t && do_throw) {
                log()->err(ec::c_no_such_type, node()->ref, name.c_str());
                return env->global()->types()->get("error_type");
            }

            return t ? t->type : nullptr;
        }

        script_type* context::type(const std::string& name, script_type* subtype) {
            script_type* base = type(name, true);

            std::string ctn = base->name + "<" + subtype->name + ">";
            script_type* ct = type(ctn, false);
            if (!ct) ct = new_types->get(ctn);
            if (!ct) {
                ct = new_types->add(ctn, ctn);

                // just copy the details
                ct->destructor = base->destructor;
                ct->methods = base->methods;
                ct->properties = base->properties;
                ct->base_type = base;
                ct->sub_type = subtype;
                ct->is_host = true;
                ct->is_builtin = base->is_builtin;
                ct->size = base->size;
                ct->owner = base->owner;
                out.types.push_back(ct);

                for (u32 f = 0;f < ct->methods.size();f++) {
                    script_function* m = ct->methods[f];
                    ct->methods[f] = m->duplicate_with_subtype(subtype);
                }
            }

            return ct;
        }

        script_type* context::type(parse::ast* n) {
            auto get = [this](parse::ast* ntype) -> script_type* {
                if (ntype->type == parse::ast::node_type::type_identifier) {
                    std::string name = *ntype;

                    if (name == "subtype") {
                        if (subtype_replacement) return subtype_replacement;
                        else {
                            log()->err(ec::c_invalid_subtype_use, ntype->ref);
                            return env->global()->types()->get("error_type");
                        }
                    }

                    if (ntype->rvalue) {
                        // todo: change ntype to member accessor for [module].[type]
                        symbol_table* mod = symbols.get_module(name);
                        if (mod) {
                            symbol_table* type = mod->get_type(*ntype->rvalue);
                            if (type) return type->type;
                        }

                        return nullptr;
                    }

                    for (u16 i = 0;i < subtype_types.size();i++) {
                        if (std::string(*subtype_types[i]->identifier) == name) {
                            if (!ntype->data_type) {
                                log()->err(ec::c_instantiation_requires_subtype, ntype->ref, name.c_str());
                                return env->global()->types()->get("error_type");
                            }

                            script_type* st = type(ntype->data_type);

                            std::string full_name = name + "<" + st->name + ">";
                            script_type* t = new_types->get(full_name);
                            if (t) return t;

                            subtype_replacement = st;
                            script_type* new_tp = class_declaration(*this, subtype_types[i]);
                            subtype_replacement = nullptr;

                            return new_tp;
                        }
                    }

                    script_type* t = type(name);
                    if (!t) return env->global()->types()->get("error_type");

                    if (ntype->data_type) {
                        if (!t->requires_subtype) {
                            // t is not a subtype class (error)
                            log()->err(ec::c_unexpected_instantiation_subtype, ntype->data_type->ref, name.c_str());
                            return env->global()->types()->get("error_type");
                        } else {
                            script_type* st = type(ntype->data_type);
                            std::string ctn = t->name + "<" + st->name + ">";
                            script_type* ct = type(ctn, false);
                            if (!ct) ct = new_types->get(ctn);
                            if (!ct) {
                                ct = new_types->add(ctn, ctn);

                                // just copy the details
                                ct->destructor = t->destructor;
                                ct->methods = t->methods;
                                ct->properties = t->properties;
                                ct->base_type = t;
                                ct->sub_type = st;
                                ct->is_host = true;
                                ct->is_builtin = t->is_builtin;
                                ct->size = t->size;
                                ct->owner = t->owner;
                                out.types.push_back(ct);

                                // but also check if a method accepts a subtype
                                for (u32 f = 0;f < ct->methods.size();f++) {
                                    script_function* m = ct->methods[f];
                                    ct->methods[f] = m->duplicate_with_subtype(st);
                                }
                            }

                            return ct;
                        }
                    } else if (t->requires_subtype) {
                        log()->err(ec::c_instantiation_requires_subtype, ntype->ref, name.c_str());
                        return env->global()->types()->get("error_type");
                    }

                    return t;
                } else if (ntype->type == parse::ast::node_type::identifier) {
                    return type(*ntype);
                } else {
                    // todo:
                    // [module].[type]
                }

                return nullptr;
            };

            script_type* tp = get(n);
            if (!tp) return nullptr;

            if (n->arguments) {
                std::vector<script_type*> args;
                parse::ast* a = n->arguments->body;
                while (a) {
                    args.push_back(get(a));
                    a = a->next;
                }
                tp = env->types()->get(function_signature(env, tp, !tp->is_primitive, args.data(), (u8)args.size(), nullptr, false, false, true));
            }

            return tp;
        }

        script_type* context::class_tp() {
            for (u32 i = (u32)node_stack.size() - 1;i > 0;i--) {
                if (node_stack[i]->type == parse::ast::node_type::class_declaration) {
                    if (node_stack[i]->is_subtype) return type(std::string(*node_stack[i]->identifier) + "<" + subtype_replacement->name + ">");
                    return type(node_stack[i]->identifier);
                }
            }
            return nullptr;
        }

        script_function* context::current_function() {
            /*
            for (u32 i = (u32)node_stack.size() - 1;i > 0;i--) {
                if (node_stack[i]->type == parse::ast::node_type::function_declaration) {
                    script_type* ret = type(node_stack[i]->data_type);
                    std::vector<script_type*> args;
                    parse::ast* n = node_stack[i]->arguments->body;
                    while (n) {
                        args.push_back(type(n->data_type));
                        n = n->next;
                    }
                    return find_func(*node_stack[i]->identifier, ret, args);
                }
            }
            */
            if (cur_func_block) return cur_func_block->func;
            return nullptr;
        }

        tac_wrapper context::add(operation op) {
            if (!compiling_function) {
                // global code
                if (compiling_static) {
                    compilation_output::insert(out.funcs[0].code, global_statics_end, tac_instruction(op, node()->ref));
                    return tac_wrapper(this, global_statics_end++, 0);
                } else {
                    out.funcs[0].code.push_back(tac_instruction(op, node()->ref));
                    return tac_wrapper(this, out.funcs[0].code.size() - 1, 0);
                }
            }

            out.funcs[cur_func_block->func_idx].code.push_back(tac_instruction(op, node()->ref));
            return tac_wrapper(this, out.funcs[cur_func_block->func_idx].code.size() - 1, cur_func_block->func_idx);
        }

        label_id context::label() {
            if (!compiling_function) {
                // global code
                if (compiling_static) {
                    if (out.funcs[0].code[global_statics_end - 1].op == operation::label) return out.funcs[0].code[global_statics_end - 1].labels[0];
                } else {
                    if (out.funcs[0].code.back().op == operation::label) return out.funcs[0].code.back().labels[0];
                }
            }

            if (out.funcs[cur_func_block->func_idx].code.back().op == operation::label) return out.funcs[cur_func_block->func_idx].code.back().labels[0];

            u32 lb = next_label_id++;
            add(operation::label).label(lb);
            return lb;
        }

        u64 context::code_sz() const {
            if (!compiling_function) return out.funcs[0].code.size();

            return out.funcs[cur_func_block->func_idx].code.size();
        }

        void context::push_node(parse::ast* node) {
            node_stack.push_back(node);
        }

        void context::pop_node() {
            node_stack.pop_back();
        }

        void context::push_block(script_function* f) {
            block_context* b = new block_context;
            block_stack.push_back(b);
            b->func = f;
            b->input_ref = node();

            if (f) {
                b->func_idx = (u16)out.funcs.size();
                out.funcs.push_back({ f, gjs::func_stack(), {}, register_allocator(out), node()->ref });
                compiling_function = true;
                cur_func_block = b;
            }
        }

        script_function* context::push_lambda_block(script_type* sigTp, std::vector<var*>& captures) {
            block_stack.push_back(new block_context);
            script_function* f = new script_function(env, format("lambda_%d", next_lambda_id++), out.funcs[0].code.size(), sigTp, nullptr, out.mod);
            f->owner = out.mod;
            f->access.entry = 0;
            new_functions.push_back(f);
            block_context* b = block();
            b->func = f;
            b->is_lambda = true;
            b->input_ref = node();
            b->func_idx = (u16)out.funcs.size();
            out.funcs.push_back({ f, gjs::func_stack(), {}, register_allocator(out), node()->ref });
            compiling_function = true;
            cur_func_block = b;

            if (captures.size() > 0) {
                var& ctx = empty_var(type("data"), "$ctx");
                ctx.set_arg_idx(0);

                u64 off = sizeof(u32) + (sizeof(u64) * captures.size());
                for (u16 c = 0;c < captures.size();c++) {
                    b->captures.push_back(new capture({ u32(off), captures[c] }));
                    //symbols.set(captures[c]->name(), b->captures.back());

                    var& v = empty_var(captures[c]->type(), captures[c]->name());

                    if (captures[c]->type()->is_primitive) {
                        // load the value from the context
                        var ptr = empty_var(captures[c]->type());
                        add(operation::uadd).operand(force_cast_var(ptr, ctx.type())).operand(ctx).operand(imm(off));
                        add(operation::load).operand(v).operand(ptr);
                        v.set_mem_ptr(ptr);
                    } else {
                        add(operation::uadd).operand(v).operand(ctx).operand(imm(off));
                    }
                    off += captures[c]->type()->size;
                }
            }
            return f;
        }

        void context::pop_block() {
            bool lastIsRet = code_sz() > 0 && (compiling_function ? out.funcs[cur_func_block->func_idx].code : out.funcs[0].code).back().op == operation::ret;
            u64 postRet = code_sz();

            bool func_ended = false;
            u32 fidx = -1;
            var& nodeCountPtr = get_var("@t_nc_p");

            if (block_stack.back()) {
                block_context* b = block_stack.back();
                script_function* f = block_stack.back()->func;
                if (f) {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        if (out.funcs[i].func == f) {
                            fidx = i;
                            break;
                        }
                    }
                    func_ended = true;
                }

                if (b->stack_objs.size() > 0) {
                    script_type* void_tp = type("void");
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];

                        // only destruct objects if this isn't the global scope
                        // global objects will be destructed with the module
                        if (v.type()->destructor && block_stack.size() > 1) {
                            call(*this, v.type()->destructor, {}, &v);
                        }

                        add(operation::stack_free).operand(v);
                    }
                }

                // remove named vars from symbol table
                for (auto v = b->named_vars.begin();v != b->named_vars.end();v++) {
                    symbols.get(v->name())->remove(&(*v));
                }
            }

            if (lastIsRet) {
                // add code to remove the current function from the tracer
                pop_trace_node(*this, nodeCountPtr);
            }

            if (lastIsRet && postRet != code_sz()) {
                // extra code was automatically added when popping the last block,
                // return statement must be moved to be at the end
                compilation_output::ir_code& code = compiling_function ? out.funcs[cur_func_block->func_idx].code : out.funcs[0].code;
                auto ret = code[postRet - 1];
                code.erase(code.begin() + (postRet - 1));
                code.push_back(ret);
            }

            delete block_stack.back();
            block_stack.pop_back();

            if (func_ended) compiling_function = false;

            for (auto i = block_stack.rbegin();i != block_stack.rend();i++) {
                if ((*i)->func) {
                    cur_func_block = (*i);
                    compiling_function = true;
                    break;
                }
            }
        }

        void context::pop_block(const var& preserve) {
            if (block_stack.back()) {
                block_context* b = block_stack.back();

                std::vector<u32> regs_to_preserve;
                if (preserve.is_reg()) regs_to_preserve.push_back(preserve.reg_id());

                u32 last_parent_id = preserve.parent_reg_id();
                while (last_parent_id != u32(-1)) {
                    regs_to_preserve.push_back(last_parent_id);

                    // see if the register is a stack object
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];
                        if (v.reg_id() == last_parent_id) {
                            // it is, continue to check the next parent, if one exists
                            last_parent_id = v.parent_reg_id();
                            break;
                        }
                    }
                }

                if (b->stack_objs.size() > 0) {
                    script_type* void_tp = type("void");
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];

                        bool do_preserve = false;
                        for (u32 p = 0;p < regs_to_preserve.size();p++) {
                            if (v.reg_id() == regs_to_preserve[p]) {
                                block_stack[block_stack.size() - 2]->stack_objs.push_back(v);
                                do_preserve = true;
                                break;
                            }
                        }

                        if (do_preserve) continue;

                        if (v.type()->destructor) {
                            call(*this, v.type()->destructor, {}, &v);
                        }

                        add(operation::stack_free).operand(v);
                    }
                }

                if (b->named_vars.size() > 0) {
                    for (auto v = b->named_vars.begin();v != b->named_vars.end();v++) {
                        symbol_list* syms = symbols.get(v->name());
                        syms->remove(&(*v));

                        if (!v->is_imm()) {
                            for (u32 p = 0;p < regs_to_preserve.size();p++) {
                                if (v->reg_id() == regs_to_preserve[p]) {
                                    block_stack[block_stack.size() - 2]->named_vars.push_back(*v);
                                    syms->add(&block_stack[block_stack.size() - 2]->named_vars.back());
                                    break;
                                }
                            }
                        }
                    }
                }

                delete b;
            }
            block_stack.pop_back();

            for (auto i = block_stack.rbegin();i != block_stack.rend();i++) {
                if ((*i)->func) {
                    cur_func_block = (*i);
                    break;
                }
            }
        }

        parse::ast* context::node() {
            return node_stack[node_stack.size() - 1];
        }

        block_context* context::block() {
            return block_stack[block_stack.size() - 1];
        }

        compile_log* context::log() {
            return env->compiler()->log();
        }
    };
};