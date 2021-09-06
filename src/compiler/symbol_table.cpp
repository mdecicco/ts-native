#include <gjs/compiler/symbol_table.h>
#include <gjs/compiler/context.h>
#include <gjs/compiler/function.h>
#include <gjs/parser/ast.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/script_enum.h>
#include <gjs/common/errors.h>

namespace gjs {
    using ec = error::ecode;

    namespace compile {
        symbol::~symbol() {
            if (m_stype == symbol_type::st_var && m_var->is_imm()) {
                // imm vars aren't stored anywhere, so
                // they must be dynamically allocated
                // specifically for storage in the symbol
                delete m_var;
            } else if (m_stype == symbol_type::st_modulevar) {
                // it's unsafe to point to modulevar
                // records on the modules directly, they
                // have to be allocated specifically for
                // storage here
                delete m_modulevar;
            }
        }

        symbol::symbol(script_function* func) {
            m_stype = symbol_type::st_function;
            m_func = func;
            m_type = nullptr;
            m_enum = nullptr;
            m_modulevar = nullptr;
            m_var = nullptr;
            m_capture = nullptr;
            m_scope_idx = 0;
        }

        symbol::symbol(script_type* type) {
            m_stype = symbol_type::st_type;
            m_func = nullptr;
            m_type = type;
            m_enum = nullptr;
            m_modulevar = nullptr;
            m_var = nullptr;
            m_capture = nullptr;
            m_scope_idx = 0;
        }

        symbol::symbol(script_enum* enum_) {
            m_stype = symbol_type::st_enum;
            m_func = nullptr;
            m_type = nullptr;
            m_enum = enum_;
            m_modulevar = nullptr;
            m_var = nullptr;
            m_capture = nullptr;
            m_scope_idx = 0;
        }

        symbol::symbol(const script_module::local_var* modulevar) {
            m_stype = symbol_type::st_modulevar;
            m_func = nullptr;
            m_type = nullptr;
            m_enum = nullptr;
            m_modulevar = modulevar;
            m_var = nullptr;
            m_capture = nullptr;
            m_scope_idx = 0;
        }

        symbol::symbol(var* var_) {
            m_stype = symbol_type::st_var;
            m_func = nullptr;
            m_type = nullptr;
            m_enum = nullptr;
            m_modulevar = nullptr;
            m_var = var_;
            m_capture = nullptr;
            m_scope_idx = u32(var_->ctx()->block_stack.size() - 1);
        }

        symbol::symbol(capture* cap) {
            m_stype = symbol_type::st_capture;
            m_func = nullptr;
            m_type = nullptr;
            m_enum = nullptr;
            m_modulevar = nullptr;
            m_var = nullptr;
            m_capture = cap;
            m_scope_idx = 0;
        }


        symbol_list::~symbol_list() {
            for (symbol_table* t : tables) {
                delete t;
            }
            tables.clear();
        }

        symbol* symbol_list::add(script_function* func) {
            symbols.emplace_back(func);
            return &symbols.back();
        }

        symbol* symbol_list::add(script_type* type) {
            symbols.emplace_back(type);
            return &symbols.back();
        }

        symbol* symbol_list::add(script_enum* enum_) {
            symbols.emplace_back(enum_);
            return &symbols.back();
        }

        symbol* symbol_list::add(const script_module::local_var* modulevar) {
            symbols.emplace_back(new script_module::local_var(*modulevar));
            return &symbols.back();
        }

        symbol* symbol_list::add(var* var_) {
            symbols.emplace_back(var_);
            return &symbols.back();
        }

        symbol* symbol_list::add(capture* cap) {
            symbols.emplace_back(cap);
            return &symbols.back();
        }

        void symbol_list::remove(script_function* func) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_function && it->get_func() == func) {
                    symbols.erase(it);
                    return;
                }
            }
        }

        void symbol_list::remove(script_type* type) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_type && it->get_type() == type) {
                    symbols.erase(it);
                    return;
                }
            }
        }

        void symbol_list::remove(script_enum* enum_) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_enum && it->get_enum() == enum_) {
                    symbols.erase(it);
                    return;
                }
            }
        }

        void symbol_list::remove(const script_module::local_var* modulevar) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_modulevar && it->get_modulevar() == modulevar) {
                    symbols.erase(it);
                    return;
                }
            }
        }

        void symbol_list::remove(var* var_) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_var && it->get_var() == var_) {
                    symbols.erase(it);
                    return;
                }
            }
        }

        void symbol_list::remove(capture* cap) {
            for (auto it = symbols.begin();it != symbols.end();it++) {
                if (it->sym_type() == symbol::symbol_type::st_capture && it->get_capture() == cap) {
                    symbols.erase(it);
                    return;
                }
            }
        }


        symbol_table::symbol_table(context* ctx) {
            module = nullptr;
            type = nullptr;
            enum_ = nullptr;
            m_ctx = ctx;
        }

        symbol_table::~symbol_table() {
            m_symbols.clear();
        }

        void symbol_table::clear() {
            m_symbols.clear();
        }

        symbol* symbol_table::set(const std::string& name, script_function* func) {
            return m_symbols[name].add(func);
        }

        symbol* symbol_table::set(const std::string& name, script_type* type) {
            symbol_table* t = get_type(name);
            bool exists = t != nullptr;
            if (!t) {
                t = nest(name);
                t->type = type;
            }

            for (u32 f = 0;f < type->methods.size();f++) {
                script_function* fn = type->methods[f];
                std::vector<script_type*> argTps;
                for (u8 a = 0;a < fn->type->signature->args.size();a++) {
                    argTps.push_back(fn->type->signature->args[a].tp);
                }
                if (!t->get_func_strict(fn->name, fn->type->signature->return_type, argTps, false)) t->set(fn->name, fn);
            }

            // todo
            // for (u32 p = 0;p < type->properties.size();p++) { }

            if (exists) {
                symbol_list& l = m_symbols[name];
                for (auto& s : l.symbols) {
                    if (s.sym_type() == symbol::symbol_type::st_type && s.get_type() == type) return &s;
                }
            }

            return m_symbols[name].add(type);
        }

        symbol* symbol_table::set(const std::string& name, script_enum* enum_) {
            symbol_table* t = nest(name);
            t->enum_ = enum_;
            auto values = enum_->values();
            for (auto it = values.begin();it != values.end();++it) {
                t->set(it->first, new var(m_ctx->imm(it->second)));
            }

            return m_symbols[name].add(enum_);
        }

        symbol* symbol_table::set(const std::string& name, const script_module::local_var* modulevar) {
            return m_symbols[name].add(modulevar);
        }

        symbol* symbol_table::set(const std::string& name, var* var_) {
            return m_symbols[name].add(var_);
        }

        symbol* symbol_table::set(const std::string& name, capture* cap) {
            return m_symbols[name].add(cap);
        }

        symbol_table* symbol_table::nest(const std::string& name) {
            m_symbols[name].tables.push_back(new symbol_table(m_ctx));
            return m_symbols[name].tables.back();
        }

        symbol_list* symbol_table::get(const std::string& name) {
            if (!m_symbols.count(name)) return nullptr;
            return &m_symbols[name];
        }

        symbol_table* symbol_table::get_module(const std::string& name) {
            symbol_list* l = get(name);
            if (!l) return nullptr;

            for (auto& t : l->tables) {
                if (t->module) return t;
            }
            return nullptr;
        }

        symbol_table* symbol_table::get_type(const std::string& name) {
            symbol_list* l = get(name);
            if (!l) return nullptr;

            for (auto& t : l->tables) {
                if (t->type) return t;
            }
            return nullptr;
        }

        symbol_table* symbol_table::get_enum(const std::string& name) {
            symbol_list* l = get(name);
            if (!l) return nullptr;

            for (auto& t : l->tables) {
                if (t->enum_) return t;
            }
            return nullptr;
        }
        var* symbol_table::get_var(const std::string& name) {
            symbol_list* l = get(name);
            if (!l) return nullptr;

            for (auto& t : l->symbols) {
                if (t.m_var) return t.m_var;
            }

            return nullptr;
        }
        capture* symbol_table::get_capture(const std::string& name) {
            symbol_list* l = get(name);
            if (!l) return nullptr;

            for (auto& t : l->symbols) {
                if (t.m_capture) return t.m_capture;
            }

            return nullptr;
        }

        script_function* symbol_table::get_func(const std::string& name, script_type* ret, const std::vector<script_type*>& args, bool log_errors, bool* was_ambiguous) {
            symbol_list* symbols = get(name);
            if (!symbols || !symbols->symbols.size()) {
                if (log_errors) m_ctx->log()->err(ec::c_no_such_function, m_ctx->node()->ref, name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
                return nullptr;
            }

            std::vector<script_function*> all;
            for (auto& s : symbols->symbols) {
                if (s.sym_type() == symbol::symbol_type::st_function) all.push_back(s.get_func());
            }

            std::vector<script_function*> matches;
            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

                // match return type
                if (ret && !has_valid_conversion(*m_ctx, func->type->signature->return_type, ret)) continue;
                bool ret_tp_strict = ret ? func->type->signature->return_type->id() == ret->id() : false;

                // match argument types
                if (func->type->signature->explicit_argc != args.size()) continue;

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->type->signature->explicit_arg(i).tp->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }

                if (match && ret_tp_strict) return func;

                if (!match) {
                    // check if the arguments are at least convertible
                    match = true;
                    for (u8 i = 0;i < args.size();i++) {
                        if (!has_valid_conversion(*m_ctx, args[i], func->type->signature->explicit_arg(i).tp)) {
                            match = false;
                            break;
                        }
                    }

                    if (!match) continue;
                }

                matches.push_back(func);
            }

            if (matches.size() > 1) {
                if (was_ambiguous) *was_ambiguous = true;
                if (log_errors) m_ctx->log()->err(ec::c_ambiguous_function, m_ctx->node()->ref, name.c_str(), name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
                return nullptr;
            }

            if (matches.size() == 1) {
                return matches[0];
            }

            if (log_errors) m_ctx->log()->err(ec::c_no_such_function, m_ctx->node()->ref, name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
            return nullptr;
        }

        script_function* symbol_table::get_func_strict(const std::string& name, script_type* ret, const std::vector<script_type*>& args, bool log_errors) {
            symbol_list* symbols = get(name);
            if (!symbols || !symbols->symbols.size()) {
                if (log_errors) m_ctx->log()->err(ec::c_no_such_function, m_ctx->node()->ref, name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
                return nullptr;
            }

            std::vector<script_function*> all;
            for (auto& s : symbols->symbols) {
                if (s.sym_type() == symbol::symbol_type::st_function) all.push_back(s.get_func());
            }

            std::vector<script_function*> matches;
            for (u16 f = 0;f < all.size();f++) {
                script_function* func = all[f];

                // match return type
                if (ret && func->type->signature->return_type->id() != ret->id()) continue;

                // match argument types
                if (func->type->signature->explicit_argc != args.size()) continue;

                // only strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->type->signature->explicit_arg(i).tp->id() != args[i]->id()) {
                        match = false;
                        break;
                    }
                }

                if (match) return func;
            }

            if (log_errors) m_ctx->log()->err(ec::c_no_such_function, m_ctx->node()->ref, name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
            return nullptr;
        }
    };
};