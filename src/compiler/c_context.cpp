#include <gjs/compiler/context.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/errors.h>
#include <gjs/common/warnings.h>
#include <gjs/compiler/compile.h>
#include <gjs/builtin/script_buffer.h>
#include <gjs/common/script_module.h>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;

    namespace compile {
        context::context(compilation_output& _out, script_context* _env) : out(_out), symbols(this), env(_env) {
            next_reg_id = 0;
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
            return var(this, s);
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
            var v = var(this, next_reg_id++, type);
            v.m_name = name;
            block()->named_vars.push_back(v);
            symbols.set(name, &block()->named_vars.back());
            return block()->named_vars.back();
        }

        var context::empty_var(script_type* type) {
            return var(this, next_reg_id++, type);
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
                for (auto s = syms->symbols.rbegin();s != syms->symbols.rend();s++) {
                    switch (s->sym_type()) {
                        case symbol::st_var: {
                            if (s->scope_idx() != 0 || !compiling_function) {
                                return *s->get_var();
                            }
                            break;
                        }
                        case symbol::st_modulevar: {
                            if (mv) break;
                            mv = s->get_modulevar();
                            break;
                        }
                        default: { }
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
            }

            log()->err(ec::c_undefined_identifier, node()->ref, name.c_str());
            return error_var();
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

        script_type* context::type(parse::ast* ntype) {
            // todo: imported subtype classes
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
            return nullptr;
        }

        tac_wrapper context::add(operation op) {
            if (!compiling_function) {
                // global code
                if (compiling_static) {
                    compilation_output::insert(global_code, global_statics_end, tac_instruction(op, node()->ref));
                    return tac_wrapper(this, global_statics_end++, true);
                } else {
                    global_code.push_back(tac_instruction(op, node()->ref));
                    return tac_wrapper(this, global_code.size() - 1, true);
                }
            }

            out.code.push_back(tac_instruction(op, node()->ref));
            return tac_wrapper(this, out.code.size() - 1, false);
        }

        u64 context::code_sz() const {
            if (!compiling_function) return global_code.size();
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
            block()->func = f;

            if (f) {
                out.funcs.push_back({ f, gjs::func_stack(), out.code.size(), 0, register_allocator(out) });
                compiling_function = true;
            }
        }

        void context::pop_block() {
            bool lastIsRet = code_sz() > 0 && (compiling_function ? out.code : global_code).back().op == operation::ret;
            u64 postRet = code_sz();

            bool func_ended = false;
            if (block_stack.back()) {
                script_function* f = block_stack.back()->func;
                if (f) {
                    for (u16 i = 0;i < out.funcs.size();i++) {
                        if (out.funcs[i].func == f) {
                            out.funcs[i].end = code_sz() - 1;
                            break;
                        }
                    }
                    func_ended = true;
                }

                block_context* b = block_stack.back();

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

                delete b;
            }

            block_stack.pop_back();

            if (lastIsRet && postRet != code_sz()) {
                // extra code was automatically added when popping the last block,
                // return statement must be moved to be at the end
                compilation_output::ir_code& code = compiling_function ? out.code : global_code;
                auto ret = code[postRet - 1];
                code.erase(code.begin() + (postRet - 1));
                code.push_back(ret);
            }

            if (func_ended) compiling_function = false;

            if (block_stack.size() == 0) {
                // global function concluded, combine code and assign __init__ function's range
                out.funcs[0].begin = out.code.size();
                out.code.insert(out.code.end(), global_code.begin(), global_code.end());
                out.funcs[0].end = out.code.size() - 1;

                // update global code jump/branch addresses
                for (u64 c = out.funcs[0].begin;c <= out.funcs[0].end;c++) {
                    tac_instruction& i = out.code[c];
                    switch(i.op) {
                        case operation::jump: {
                            i.operands[0].m_imm.u += out.funcs[0].begin;
                            break;
                        }
                        case operation::branch: {
                            i.operands[1].m_imm.u += out.funcs[1].begin;
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
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
                        if (v.reg_id() == preserve.reg_id()) {
                            block_stack[block_stack.size() - 2]->stack_objs.push_back(v);
                            continue;
                        }
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

                        if (!v->is_imm() && v->reg_id() == preserve.reg_id()) {
                            block_stack[block_stack.size() - 2]->named_vars.push_back(*v);
                            syms->add(&block_stack[block_stack.size() - 2]->named_vars.back());
                        }
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