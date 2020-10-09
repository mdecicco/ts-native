#include <compiler/context.h>
#include <compiler/function.h>
#include <parser/ast.h>
#include <common/context.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <common/errors.h>
#include <common/warnings.h>
#include <compiler/compile.h>
#include <builtin/script_buffer.h>
#include <common/module.h>

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
            subtype_replacement = nullptr;
            compiling_function = false;
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

        var& context::empty_var(script_type* type, const std::string& name) {
            var v = var(this, next_reg_id++, type);
            v.m_name = name;
            block_stack[block_stack.size() - 1]->named_vars.push_back(v);
            return block_stack[block_stack.size() - 1]->named_vars.back();
        }

        var context::empty_var(script_type* type) {
            return var(this, next_reg_id++, type);
        }

        var& context::dummy_var(script_type* type, const std::string& name) {
            var v = var();
            v.m_ctx = this;
            v.m_instantiation = node()->ref;
            v.m_name = name;
            v.m_type = type;
            block_stack[block_stack.size() - 1]->named_vars.push_back(v);
            return block_stack[block_stack.size() - 1]->named_vars.back();
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
            static var dv = dummy_var(type("error_type"));
            return dv;
        }

        var& context::get_var(const std::string& name) {
            for (auto i = block_stack.rbegin();i != block_stack.rend();i++) {
                for (u16 v = 0;v < (*i)->named_vars.size();v++) {
                    if ((*i)->named_vars[v].name() == name) return (*i)->named_vars[v];
                }

                // don't check beyond the most recent function block
                // (function compilation can be nested, if a subtype class has to be compiled and that subtype has methods)
                if ((*i)->func) break;
            }

            if (out.mod->has_local(name)) {
                auto global = out.mod->local(name);
                var v = empty_var(global.type, name);
                v.m_instantiation = global.ref;

                if (global.type->is_primitive) {
                    var ptr = empty_var(type("data"));
                    add(operation::module_data).operand(ptr).operand(imm((u64)out.mod->id())).operand(imm(global.offset));
                    v.set_mem_ptr(ptr);
                    add(operation::load).operand(v).operand(ptr);
                } else {
                    add(operation::module_data).operand(v).operand(imm((u64)out.mod->id())).operand(imm(global.offset));
                }

                auto& vec = block()->named_vars;
                vec.push_back(v);
                return vec.back();
            }

            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias.length() > 0) continue;

                const script_module::local_var* imported = nullptr;
                for (u16 s = 0;s < imports[i]->symbols.size() && !imported;s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_local) {
                        imported = &imports[i]->mod->local(name);
                    } else if (sym.alias == name && sym.is_local) {
                        imported = &imports[i]->mod->local(sym.name);
                    }
                }

                if (imported) {
                    var v = empty_var(imported->type, name);
                    v.m_instantiation = imported->ref;

                    if (imported->type->is_primitive) {
                        var ptr = empty_var(type("data"));
                        add(operation::module_data).operand(ptr).operand(imm((u64)imports[i]->mod->id())).operand(imm(imported->offset));
                        v.set_mem_ptr(ptr);
                        add(operation::load).operand(v).operand(ptr);
                    } else {
                        add(operation::module_data).operand(v).operand(imm((u64)imports[i]->mod->id())).operand(imm(imported->offset));
                    }

                    auto& vec = block()->named_vars;
                    vec.push_back(v);
                    return vec.back();
                }
            }


            log()->err(ec::c_undefined_identifier, node()->ref, name.c_str());
            return error_var();
        }

        script_function* context::function(const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            std::vector<script_function*> matches;
            std::vector<script_function*> all;
            
            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias.length() > 0) continue;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    } else if (sym.alias == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(sym.name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    }
                }
            }

            for (u16 f = 0;f < new_functions.size();f++) {
                if (new_functions[f]->name != name) continue;
                all.push_back(new_functions[f]);
            }

            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

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

        script_function* context::function(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            std::vector<script_function*> matches;
            std::vector<script_function*> all;

            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias != from_aliased_import) continue;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    } else if (sym.alias == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(sym.name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    }
                }
            }

            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

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
            std::vector<script_function*> all;
            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias.length() > 0) continue;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    } else if (sym.alias == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(sym.name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    }
                }
            }

            for (u16 f = 0;f < new_functions.size();f++) {
                if (new_functions[f]->name != name) continue;
                all.push_back(new_functions[f]);
            }

            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

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


        script_function* context::find_func(const std::string& from_aliased_import, const std::string& name, script_type* ret, const std::vector<script_type*>& args) {
            std::vector<script_function*> all;

            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias != from_aliased_import) continue;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    } else if (sym.alias == name && sym.is_func) {
                        auto funcs = imports[i]->mod->function_overloads(sym.name);
                        all.insert(all.begin(), funcs.begin(), funcs.end());
                    }
                }
            }

            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

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

        bool context::identifier_in_use(const std::string& name) {
            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias == name) return true;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name) return true;
                    if (sym.alias == name) return true;
                }
            }

            script_type* t = new_types->get(name);
            if (t) return true;

            for (u16 i = 0;i < subtype_types.size();i++) {
                if (std::string(*subtype_types[i]->identifier) == name) return true;
            }

            for (u16 f = 0;f < new_functions.size();f++) {
                if (new_functions[f]->name == name) return true;
            }

            for (u8 i = (u8)block_stack.size() - 1;i > 0;i--) {
                for (u16 v = 0;v < block_stack[i]->named_vars.size();v++) {
                    if (block_stack[i]->named_vars[v].name() == name) return true;
                }

                // don't check beyond the most recent function block
                // (function compilation can be nested, if a subtype class has to be compiled and that subtype has methods)
                if (block_stack[i]->func) break;
            }

            return false;
        }

        script_type* context::type(const std::string& name) {
            if (name == "subtype") {
                if (subtype_replacement) return subtype_replacement;
                else {
                    log()->err(ec::c_invalid_subtype_use, node()->ref);
                    return env->global()->types()->get("error_type");
                }
            }

            for (u16 i = 0;i < imports.size();i++) {
                if (imports[i]->alias.length() > 0) continue;

                for (u16 s = 0;s < imports[i]->symbols.size();s++) {
                    auto& sym = imports[i]->symbols[s];
                    if (sym.alias.length() == 0 && sym.name == name && sym.is_type) return sym.type;
                    if (sym.alias == name && sym.is_type) return sym.type;
                }
            }

            script_type* t = new_types->get(name);

            if (!t) {
                log()->err(ec::c_no_such_type, node()->ref, name.c_str());
                return env->global()->types()->get("error_type");
            }

            return t;
        }

        script_type* context::type(parse::ast* type_identifier) {
            std::string name = *type_identifier;
            if (type_identifier->rvalue) {
                std::string tn = *type_identifier->rvalue;
                script_type* tp = nullptr;
                for (u16 i = 0;i < imports.size() && !tp;i++) {
                    if (imports[i]->alias != name) continue;

                    for (u16 s = 0;s < imports[i]->symbols.size() && !tp;s++) {
                        auto& sym = imports[i]->symbols[s];
                        if (sym.alias.length() == 0 && sym.name == tn && sym.is_type) tp = sym.type;
                        else if (sym.alias == tn && sym.is_type) tp = sym.type;
                    }
                }

                // todo: imported subtype classes

                return tp;
            }

            if (name == "subtype") {
                if (subtype_replacement) return subtype_replacement;
                else {
                    log()->err(ec::c_invalid_subtype_use, type_identifier->ref);
                    return env->global()->types()->get("error_type");
                }
            }

            for (u16 i = 0;i < subtype_types.size();i++) {
                if (name == std::string(*subtype_types[i]->identifier)) {
                    if (!type_identifier->data_type) {
                        log()->err(ec::c_instantiation_requires_subtype, type_identifier->ref, name.c_str());
                        return env->global()->types()->get("error_type");
                    }

                    std::string full_name = name + "<" + std::string(*type_identifier->data_type) + ">";
                    script_type* t = new_types->get(full_name);
                    if (t) return t;

                    subtype_replacement = type(type_identifier->data_type);
                    script_type* new_tp = class_declaration(*this, subtype_types[i]);
                    subtype_replacement = nullptr;

                    return new_tp;
                }
            }

            script_type* t = type(name);
            if (!t) return env->global()->types()->get("error_type");

            if (type_identifier->data_type) {
                if (!t->requires_subtype) {
                    // t is not a subtype class (error)
                    log()->err(ec::c_unexpected_instantiation_subtype, type_identifier->data_type->ref, name.c_str());
                    return env->global()->types()->get("error_type");
                } else {
                    script_type* st = type(type_identifier->data_type);
                    std::string ctn = t->name + "<" + st->name + ">";
                    script_type* ct = type(ctn);
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
                        out.types.push_back(ct);
                    }

                    return ct;
                }
            } else if (t->requires_subtype) {
                log()->err(ec::c_instantiation_requires_subtype, type_identifier->ref, name.c_str());
                return env->global()->types()->get("error_type");
            }

            return t;
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

        tac_instruction& context::add(operation op) {
            if (!compiling_function) {
                // global code
                global_code.push_back(tac_instruction(op, node()->ref));
                return global_code.back();
            }

            out.code.push_back(tac_instruction(op, node()->ref));
            return out.code.back();
        }

        void context::ensure_code_ref() {
            if (!compiling_function) {
                if (i64(global_code.size()) >= i64(global_code.capacity()) - 32) {
                    // prevent vector resizing before some instruction is fully defined
                    global_code.reserve(global_code.capacity() + 32);
                }
                return;
            }

            if (i64(out.code.size()) >= i64(out.code.capacity()) - 32) {
                // prevent vector resizing before some instruction is fully defined
                out.code.reserve(out.code.capacity() + 32);
            }
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
            block_stack[block_stack.size() - 1]->func = f;

            if (f) {
                out.funcs.push_back({ f, gjs::func_stack(), out.code.size(), 0, register_allocator(out) });
                compiling_function = true;
            }
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
                    compiling_function = false;
                }

                block_context* b = block_stack.back();

                if (b->stack_objs.size() > 0) {
                    script_type* void_tp = type("void");
                    for (u16 i = 0;i < b->stack_objs.size();i++) {
                        var& v = b->stack_objs[i];

                        // only destruct objects if this isn't the global scope
                        // global objects will be destructed with the module
                        if (v.type()->destructor && block_stack.size() > 1) {
                            call(*this, v.type()->destructor, { v });
                        }

                        add(operation::stack_free).operand(v);
                    }
                }

                delete b;
            }
            block_stack.pop_back();

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