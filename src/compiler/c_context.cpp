#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/context.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/errors.h>
#include <common/warnings.h>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;

    namespace compile {
        context::context(compilation_output& _out) : out(_out) {
            next_reg_id = 0;
            env = nullptr;
            input = nullptr;
            new_types = nullptr;
            push_block();
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
            return var(this, s);
        }

        var context::empty_var(script_type* type, const std::string& name) {
            var v = var(this, next_reg_id++, type);
            v.m_name = name;
            block_stack[block_stack.size() - 1]->named_vars.push_back(v);
            return v;
        }

        var context::empty_var(script_type* type) {
            return var(this, next_reg_id++, type);
        }

        var context::dummy_var(script_type* type, const std::string& name) {
            var v = var();
            v.m_ctx = this;
            v.m_instantiation = node()->ref;
            v.m_name = name;
            v.m_type = type;
            block_stack[block_stack.size() - 1]->named_vars.push_back(v);
            return v;
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

        var context::error_var() {
            return dummy_var(type("error_type"));
        }

        var context::get_var(const std::string& name) {
            for (u8 i = block_stack.size() - 1;i > 0;i--) {
                for (u16 v = 0;v < block_stack[i]->named_vars.size();v++) {
                    if (block_stack[i]->named_vars[v].name() == name) return block_stack[i]->named_vars[v];
                }
            }

            log()->err(ec::c_undefined_identifier, node()->ref, name.c_str());
            return error_var();
        }

        script_function* context::function(const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            std::vector<script_function*> matches;

            std::vector<script_function*> all = env->all_functions();
            for (u16 f = 0;f < new_functions.size();f++) all.push_back(new_functions[f]);
            for (u16 f = 0;f < all.size();f++) {
                // match name
                script_function* func = all[f];

                if (func->name != name) continue;

                // match return type
                if (ret && !has_valid_conversion(*this, func->signature.return_type, ret)) continue;
                bool ret_tp_strict = ret ? func->signature.return_type->id() == ret->id() : false;

                // match argument types
                if (func->signature.arg_types.size() != args.size()) continue;

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->signature.arg_types[i]->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }

                if (match && ret_tp_strict) return func;

                if (!match) {
                    // check if the arguments are at least convertible
                    match = true;
                    for (u8 i = 0;i < args.size();i++) {
                        if (!has_valid_conversion(*this, args[i], func->signature.arg_types[i])) {
                            match = false;
                            break;
                        }
                    }

                    if (!match) continue;
                }

                matches.push_back(func);
            }

            if (matches.size() > 1) {
                log()->err(ec::c_ambiguous_function, node()->ref, name.c_str(), name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
                return nullptr;
            }

            if (matches.size() == 1) {
                return matches[0];
            }

            log()->err(ec::c_no_such_function, node()->ref, name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
            return nullptr;
        }

        script_function* context::find_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            std::vector<script_function*> all = env->all_functions();
            for (u16 f = 0;f < new_functions.size();f++) all.push_back(new_functions[f]);
            for (u16 f = 0;f < all.size();f++) {
                // match name
                script_function* func = all[f];

                if (func->name != name) continue;

                // match return type
                if (ret && func->signature.return_type->id() != ret->id()) continue;

                // match argument types
                if (func->signature.arg_types.size() != args.size()) continue;

                // only strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->signature.arg_types[i]->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }

                if (match) return func;
            }
            return nullptr;
        }

        script_type* context::type(const std::string& name) {
            script_type* t = env->types()->get(hash(name));
            if (t) return t;
            t = new_types->get(name);

            if (!t) {
                log()->err(ec::c_no_such_type, node()->ref, name.c_str());
                return env->types()->get("error_type");
            }

            return t;
        }

        script_type* context::type(parse::ast* type_identifier) {
            script_type* t = type(*type_identifier);
            if (!t) return nullptr;

            if (type_identifier->data_type) {
                if (!t->requires_subtype) {
                    // t is not a subtype class (error)
                } else {
                    script_type* st = type(type_identifier->data_type);
                    std::string ctn = t->name + "<" + st->name + ">";
                    script_type* ct = env->types()->get(hash(ctn));
                    if (!ct) ct = new_types->get(ctn);
                    if (!ct) {
                        ct = new_types->add(ctn, ctn);
                        if (t->is_host) {
                            // just copy the details
                            ct->destructor = t->destructor;
                            ct->methods = t->methods;
                            ct->properties = t->properties;
                            ct->base_type = t;
                            ct->sub_type = st;
                            ct->is_host = true;
                            ct->is_builtin = t->is_builtin;
                            ct->size = t->size;
                            out.types.push_back(ct);
                        } else {
                            // recompile the type with all occurrences of "subtype"
                            // as a data type changed to refer to the actual subtype
                            // todo:
                            //  - Decide syntax for declaring subtype classes in scripts
                            //  - Store subtype class declaration nodes in context to be
                            //    used here later
                            //  - Add compiler function for compiling classes, formats,
                            //    functions, methods that use subtypes
                            //
                        }
                    }

                    return ct;
                }
            }

            return t;
        }

        script_type* context::class_tp() {
            for (u32 i = node_stack.size() - 1;i > 0;i--) {
                if (node_stack[i]->type == parse::ast::node_type::class_declaration) {
                    return type(node_stack[i]->identifier);
                }
            }
            return nullptr;
        }

        script_function* context::current_function() {
            for (u32 i = node_stack.size() - 1;i > 0;i--) {
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
            return nullptr;
        }

        tac_instruction& context::add(operation op) {
            out.code.push_back(tac_instruction(op, node()->ref));
            return out.code.back();
        }

        void context::ensure_code_ref() {
            if (out.code.size() >= out.code.capacity() - 32) {
                // prevent vector resizing before some instruction is fully defined
                out.code.reserve(out.code.capacity() + 32);
            }
        }

        u64 context::code_sz() const {
            return out.code.size();
        }

        void context::push_node(parse::ast* node) {
            node_stack.push_back(node);
        }

        void context::pop_node() {
            node_stack.pop_back();
        }

        void context::push_block(script_function* f) {
            block_stack.push_back(new block_context);
            block_stack[block_stack.size() - 1]->func = f;

            if (f) out.funcs.push_back({ f, gjs::func_stack(), code_sz(), 0 });
        }

        void context::pop_block() {
            if (block_stack.back()) {
                script_function* f = block_stack.back()->func;
                if (f) {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        if (out.funcs[i].func == f) {
                            out.funcs[i].end = code_sz() - 1;
                        }
                    }
                }

                block_context* b = block_stack.back();

                if (b->stack_objs.size() > 0) {
                    script_type* void_tp = type("void");
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];
                        if (v.type()->destructor) {
                            call(*this, v.type()->destructor, { v });
                        }

                        add(operation::stack_free).operand(v);
                    }
                }

                delete b;
            }
            block_stack.pop_back();
        }

        void context::pop_block(const var& preserve) {
            if (block_stack.back()) {
                script_function* f = block_stack.back()->func;
                if (f) {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        if (out.funcs[i].func == f) {
                            out.funcs[i].end = code_sz() - 1;
                        }
                    }
                }

                block_context* b = block_stack.back();

                if (b->stack_objs.size() > 0) {
                    script_type* void_tp = type("void");
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];
                        if (v.reg_id() == preserve.reg_id()) continue;
                        if (v.type()->destructor) {
                            call(*this, v.type()->destructor, { v });
                        }

                        add(operation::stack_free).operand(v);
                    }
                }

                delete b;
            }
            block_stack.pop_back();
        }

        parse::ast* context::node() {
            return node_stack[node_stack.size() - 1];
        }

        context::block_context* context::block() {
            return block_stack[block_stack.size() - 1];
        }

        compile_log* context::log() {
            return env->compiler()->log();
        }
    };
};