#include <compiler/variable.h>
#include <compiler/context.h>
#include <compiler/function.h>
#include <compiler/compile.h>
#include <parser/ast.h>
#include <common/errors.h>
#include <common/warnings.h>
#include <common/context.h>
#include <common/script_type.h>
#include <common/script_function.h>
#include <util/robin_hood.h>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;

    namespace compile {
        bool has_valid_conversion(context& ctx, script_type* from, script_type* to) {
            if (from->id() == to->id()) return true;
            if (from->base_type && from->base_type->id() == to->id()) return true;
            if (!from->is_primitive && to->name == "data") return true;
            if (from->name == "data" && !to->is_primitive) return true;

            if (from->name == "void" || to->name == "void") return false;

            if (!from->is_primitive) {
                var fd = ctx.dummy_var(from);
                if (to->is_primitive) {
                    // last chance to find a conversion
                    return fd.has_unambiguous_method("<cast>", to, { from });
                } else if (fd.has_unambiguous_method("<cast>", to, { from })) return true;
            }

            if (!to->is_primitive) {
                var td = ctx.dummy_var(to);
                return td.has_unambiguous_method("constructor", to, { ctx.type("data"), from });
            }
            
            // conversion between non-void primitive types is always possible
            return true;
        }


        var::var(context* ctx, u64 u) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_reg_id = -1;
            m_imm.u = u;
            m_type = ctx->type("u64");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_flags = bind::pf_none;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, i64 i) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_flags = bind::pf_none;
            m_reg_id = -1;
            m_imm.i = i;
            m_type = ctx->type("i64");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, f32 f) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_flags = bind::pf_none;
            m_reg_id = -1;
            m_imm.f = f;
            m_type = ctx->type("f32");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, f64 d) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_flags = bind::pf_none;
            m_reg_id = -1;
            m_imm.d = d;
            m_type = ctx->type("f64");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, const std::string& s) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_flags = bind::pf_none;
            m_reg_id = -1;
            m_type = ctx->type("string");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_imm.u = 0;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, u32 reg_id, script_type* type) {
            m_ctx = ctx;
            m_is_imm = false;
            m_instantiation = ctx->node()->ref;
            m_flags = bind::pf_none;
            m_reg_id = reg_id;
            m_type = type;
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_imm.u = 0;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var() {
            m_ctx = nullptr;
            m_is_imm = false;
            m_flags = bind::pf_none;
            m_reg_id = -1;
            m_type = nullptr;
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_imm.u = 0;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = 0;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(const var& v) {
            m_ctx = v.m_ctx;
            m_is_imm = v.m_is_imm;
            m_instantiation = v.m_instantiation;
            m_flags = v.m_flags;
            m_reg_id = v.m_reg_id;
            m_imm = v.m_imm;
            m_type = v.m_type;
            m_stack_loc = v.m_stack_loc;
            m_arg_idx = v.m_arg_idx;
            m_mem_ptr = v.m_mem_ptr;
            m_name = v.m_name;
            m_stack_id = v.m_stack_id;
            m_setter.func = v.m_setter.func;
            m_setter.this_obj = v.m_setter.this_obj;
        }

        var::~var() {
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }




        u64 var::size() const {
            return m_type->size;
        }

        bool var::has_prop(const std::string& prop) const {
            for (u16 i = 0;i < m_type->properties.size();i++) {
                if (m_type->properties[i].name == prop) return true;
            }

            return false;
        }

        var var::prop(const std::string& prop) const {
            for (u16 i = 0;i < m_type->properties.size();i++) {
                auto& p = m_type->properties[i];
                if (p.name == prop) {
                    if (p.getter) {
                        var v = call(*m_ctx, p.getter, { *this });
                        if (p.setter) {
                            v.m_setter.this_obj.reset(new var(*this));
                            v.m_setter.func = p.setter;
                        }
                        v.m_flags = p.flags;
                        return v;
                    } else {
                        var ptr = m_ctx->empty_var(m_ctx->type("u64"));
                        var ret = m_ctx->empty_var(p.type);
                        if (p.flags & bind::pf_static) {
                            m_ctx->add(operation::eq).operand(ptr).operand(m_ctx->imm(p.offset));
                            m_ctx->add(operation::load).operand(ret).operand(m_ctx->imm(p.offset));
                        } else {
                            m_ctx->add(operation::uadd).operand(ptr).operand(*this).operand(m_ctx->imm(p.offset));
                            m_ctx->add(operation::load).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
                        }
                        ret.set_mem_ptr(ptr);

                        if (p.setter) {
                            ret.m_setter.this_obj.reset(new var(*this));
                            ret.m_setter.func = p.setter;
                        }
                        ret.m_flags = p.flags;
                        return ret;
                    }
                }
            }

            m_ctx->log()->err(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
            return m_ctx->error_var();
        }

        var var::prop_ptr(const std::string& prop) const {
            for (u16 i = 0;i < m_type->properties.size();i++) {
                auto& p = m_type->properties[i];
                if (p.name == prop) {
                    if (p.flags & bind::pf_static) {
                        return m_ctx->imm(p.offset);
                    } else {
                        var ret = m_ctx->empty_var(m_ctx->type("u64"));
                        m_ctx->add(operation::uadd).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
                        return ret;
                    }
                }
            }
            
            m_ctx->log()->err(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
            return var();
        }

        bool var::has_any_method(const std::string& _name) const {
            for (u16 m = 0;m < m_type->methods.size();m++) {
                // match name
                script_function* func = nullptr;
                if (_name.find_first_of(' ') != std::string::npos) {
                    // probably an operator
                    std::vector<std::string> mparts = split(split(m_type->methods[m]->name, ":")[1], " \t\n\r");
                    std::vector<std::string> sparts = split(_name, " \t\n\r");
                    if (mparts.size() != sparts.size()) continue;
                    bool matched = true;
                    for (u32 i = 0;matched && i < mparts.size();i++) {
                        matched = mparts[i] == sparts[i];
                    }
                    if (matched) return true;
                }
                std::string& bt_name = m_type->base_type ? m_type->base_type->name : m_type->name;
                if (m_type->methods[m]->name == bt_name + "::" + _name) return true;
            }

            return false;
        }

        bool var::has_unambiguous_method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args) const {
            std::vector<script_function*> matches;

            for (u16 m = 0;m < m_type->methods.size();m++) {
                // match name
                script_function* func = nullptr;
                if (_name == "<cast>") {
                    script_function* mt = m_type->methods[m];
                    auto mparts = split(split(mt->name,":")[1], " \t\n\r");
                    if (mparts.size() == 2 && mparts[0] == "operator" && mparts[1] == mt->signature.return_type->name && mt->signature.arg_types.size() == 1) {
                        if (mt->signature.return_type->id() == ret->id()) {
                            return mt;
                        } else if (has_valid_conversion(*m_ctx, mt->signature.return_type, ret)) {
                            matches.push_back(mt);
                            continue;
                        }
                    }
                } else if (_name.find_first_of(' ') != std::string::npos) {
                    // probably an operator
                    std::vector<std::string> mparts = split(split(m_type->methods[m]->name, ":")[1], " \t\n\r");
                    std::vector<std::string> sparts = split(_name, " \t\n\r");
                    if (mparts.size() != sparts.size()) continue;
                    bool matched = true;
                    for (u32 i = 0;matched && i < mparts.size();i++) {
                        matched = mparts[i] == sparts[i];
                    }
                    if (matched) func = m_type->methods[m];
                }
                std::string& bt_name = m_type->base_type ? m_type->base_type->name : m_type->name;
                if (m_type->methods[m]->name == bt_name + "::" + _name) func = m_type->methods[m];

                if (!func) continue;

                // match return type
                if (ret && !has_valid_conversion(*m_ctx, ret, func->signature.return_type)) continue;
                bool ret_tp_strict = ret ? func->signature.return_type->id() == ret->id() : false;

                // match argument types
                if (func->signature.arg_types.size() != args.size()) continue;

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->signature.arg_types[i]->name == "subtype") {
                        if (m_type->sub_type->id() != args[i]->id()) {
                            match = false;
                            break;
                        }
                    } else {
                        if (func->signature.arg_types[i]->id() != args[i]->id()) {
                            match = false;
                            break;
                        }
                    }
                }

                if (match && ret_tp_strict) return true;

                if (!match) {
                    // check if the arguments are at least convertible
                    match = true;
                    for (u8 i = 0;i < args.size();i++) {
                        script_type* at = func->signature.arg_types[i];
                        if (at->name == "subtype") at = m_type->sub_type;
                        if (!has_valid_conversion(*m_ctx, args[i], at)) {
                            match = false;
                            break;
                        }
                    }

                    if (!match) continue;
                }

                matches.push_back(func);
            }

            if (matches.size() > 1) return false;
            if (matches.size() == 1) return true;
            return false;
        }

        script_function* var::method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args) const {
            std::vector<script_function*> matches;

            for (u16 m = 0;m < m_type->methods.size();m++) {
                // match name
                script_function* func = nullptr;
                if (_name == "<cast>") {
                    script_function* mt = m_type->methods[m];
                    auto mparts = split(split(mt->name,":")[1], " \t\n\r");
                    if (mparts.size() == 2 && mparts[0] == "operator" && mparts[1] == mt->signature.return_type->name && mt->signature.arg_types.size() == 1) {
                        if (mt->signature.return_type->id() == ret->id()) {
                            return mt;
                        } else if (has_valid_conversion(*m_ctx, mt->signature.return_type, ret)) {
                            matches.push_back(mt);
                            continue;
                        }
                    }
                } else if (_name.find_first_of(' ') != std::string::npos) {
                    // probably an operator
                    std::vector<std::string> mparts = split(split(m_type->methods[m]->name, ":")[1], " \t\n\r");
                    std::vector<std::string> sparts = split(_name, " \t\n\r");
                    if (mparts.size() != sparts.size()) continue;
                    bool matched = true;
                    for (u32 i = 0;matched && i < mparts.size();i++) {
                        matched = mparts[i] == sparts[i];
                    }
                    if (matched) func = m_type->methods[m];
                }

                std::string& bt_name = m_type->base_type ? m_type->base_type->name : m_type->name;
                if (m_type->methods[m]->name == bt_name + "::" + _name) func = m_type->methods[m];

                if (!func) continue;

                // match return type
                if (ret && !has_valid_conversion(*m_ctx, func->signature.return_type, ret)) continue;
                bool ret_tp_strict = ret ? func->signature.return_type->id() == ret->id() : false;

                // match argument types
                if (func->signature.arg_types.size() != args.size()) continue;

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->signature.arg_types[i]->name == "subtype") {
                        if (m_type->sub_type->id() != args[i]->id()) {
                            match = false;
                            break;
                        }
                    } else {
                        if (func->signature.arg_types[i]->id() != args[i]->id()) {
                            match = false;
                            break;
                        }
                    }
                }

                if (match && ret_tp_strict) return func;

                if (!match) {
                    // check if the arguments are at least convertible
                    match = true;
                    for (u8 i = 0;i < args.size();i++) {
                        script_type* at = func->signature.arg_types[i];
                        if (at->name == "subtype") at = m_type->sub_type;
                        if (!has_valid_conversion(*m_ctx, args[i], at)) {
                            match = false;
                            break;
                        }
                    }

                    if (!match) continue;
                }

                matches.push_back(func);
            }

            if (matches.size() > 1) {
                m_ctx->log()->err(ec::c_ambiguous_method, m_ctx->node()->ref, _name.c_str(), m_type->name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
                return nullptr;
            }

            if (matches.size() == 1) {
                return matches[0];
            }

            m_ctx->log()->err(ec::c_no_such_method, m_ctx->node()->ref, m_type->name.c_str(), _name.c_str(), arg_tp_str(args).c_str(), !ret ? "<any>" : ret->name.c_str());
            return nullptr;
        }

        bool var::convertible_to(script_type* tp) const {
            return has_valid_conversion(*m_ctx, m_type, tp);
        }

        var var::convert(script_type* to) const {
            script_type* from = m_type;

            if (from->id() == to->id()) return *this;
            if (from->base_type && from->base_type->id() == to->id()) return *this;
            if (!from->is_primitive && to->name == "data") return *this;
            if (from->name == "data" && !to->is_primitive) return *this;

            if (from->name == "void" || to->name == "void") {
                m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                return m_ctx->error_var();
            }

            if (!from->is_primitive) {
                if (to->is_primitive) {
                    // last chance to find a conversion
                    script_function* cast = method("<cast>", to, { m_type });
                    if (cast) {
                        var ret = call(*m_ctx, cast, { *this });
                        if (ret.type()->id() != to->id()) {
                            return ret.convert(to);
                        }

                        return ret;
                    }

                    m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                    return m_ctx->error_var();
                } else if (has_unambiguous_method("<cast>", to, { m_type })) {
                    script_function* cast = method("<cast>", to, { m_type });
                    var ret = call(*m_ctx, cast, { *this });
                    if (ret.type()->id() != to->id()) {
                        return ret.convert(to);
                    }

                    return ret;
                }
            }

            if (!to->is_primitive) {
                var tv = m_ctx->dummy_var(to);
                script_function* ctor = tv.method("constructor", to, { m_ctx->type("data"), from });
                if (ctor) {
                    var ret = m_ctx->empty_var(to);
                    ret.raise_stack_flag();
                    construct_on_stack(*m_ctx, ret, { *this });

                    if (ret.type()->id() != to->id()) {
                        return ret.convert(to);
                    }
                    return ret;
                }
                
                m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                return m_ctx->error_var();
            }
            
            // conversion between non-void primitive types is always possible
            var v = m_ctx->empty_var(to);
            m_ctx->add(operation::eq).operand(v).operand(*this);
            m_ctx->add(operation::cvt).operand(v).operand(m_ctx->imm((u64)from->id())).operand(m_ctx->imm((u64)to->id()));
            return v;
        }

        std::string var::to_string() const {
            if (is_spilled()) {
                if (m_name.length() > 0) return format("[$sp + %u] (%s)", m_stack_loc, m_name.c_str());
                else return format("[$sp + %u]", m_stack_loc);
            }
            if (m_is_imm) {
                if (m_type->is_floating_point) {
                    if (m_type->size == sizeof(f64)) {
                        return format("%f", m_imm.d);
                    } else {
                        return format("%f", m_imm.f);
                    }
                } else {
                    if (m_type->is_unsigned) {
                        return format("%llu", m_imm.u);
                    } else {
                        return format("%lld", m_imm.i);
                    }
                }
            }

            if (m_name.length() > 0) {
                if (m_type->is_floating_point) return format("$FP%d (%s)", m_reg_id, m_name.c_str());
                return format("$GP%d (%s)", m_reg_id, m_name.c_str());
            }

            if (m_type->is_floating_point) return format("$FP%d", m_reg_id);
            return format("$GP%d", m_reg_id);
        }

        void var::set_mem_ptr(const var& v) {
            m_mem_ptr.valid = v.valid();
            m_mem_ptr.reg = v.m_reg_id;
        }

        void var::raise_stack_flag() {
            static u64 next_stack_id = 1;
            m_stack_id = next_stack_id++;
            m_ctx->block()->stack_objs.push_back(*this);
        }

        void var::adopt_stack_flag(var& from) {
            m_stack_id = from.m_stack_id;
            from.m_stack_id = 0;
            bool found = false;
            for (u16 b = 0;b < m_ctx->block_stack.size();b++) {
                auto* cblock = m_ctx->block_stack[b];
                for (u16 i = 0;i < cblock->stack_objs.size();i++) {
                    if (cblock->stack_objs[i].m_stack_id == m_stack_id) {
                        cblock->stack_objs.insert(cblock->stack_objs.begin() + i, *this);
                        cblock->stack_objs.erase(cblock->stack_objs.begin() + (u64(i) + u64(1)));
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }

        script_type* var::call_this_tp() const {
            return m_type->base_type && m_type->is_host ? m_type->base_type : m_type;
        }



        using ot = parse::ast::operation_type;

        static const char* ot_str[] = {
            "invlaid",
            "+",
            "-",
            "*",
            "/",
            "%",
            "<<",
            ">>",
            "&&",
            "||",
            "&",
            "|",
            "^",
            "+=",
            "-=",
            "*=",
            "/=",
            "%=",
            "<<=",
            ">>=",
            "&&=",
            "||=",
            "&=",
            "|=",
            "^=",
            "++",
            "--",
            "++",
            "--",
            "<",
            ">",
            "<=",
            ">=",
            "!=",
            "==",
            "=",
            "!",
            "-"
            "invlaid",
            "[]",
            "invalid",
            "invalid"
        };

        operation get_op(ot op, script_type* tp) {
            using o = operation;
            if (!tp->is_primitive) return o::null;

            static o possible_arr[][4] = {
                { o::iadd , o::uadd , o::dadd , o::fadd  },
                { o::isub , o::usub , o::dsub , o::fsub  },
                { o::imul , o::umul , o::dmul , o::fmul  },
                { o::idiv , o::udiv , o::ddiv , o::fdiv  },
                { o::imod , o::umod , o::dmod , o::fmod  },
                { o::ilt  , o::ult  , o::dlt  , o::flt   },
                { o::igt  , o::ugt  , o::dgt  , o::fgt   },
                { o::ilte , o::ulte , o::dlte , o::flte  },
                { o::igte , o::ugte , o::dgte , o::fgte  },
                { o::incmp, o::uncmp, o::dncmp, o::fncmp },
                { o::icmp , o::ucmp , o::dcmp , o::fcmp  },
                { o::sl   , o::sl   , o::null , o::null  },
                { o::sr   , o::sr   , o::null , o::null  },
                { o::land , o::land , o::null , o::null  },
                { o::lor  , o::lor  , o::null , o::null  },
                { o::band , o::band , o::null , o::null  },
                { o::bor  , o::bor  , o::null , o::null  },
                { o::bxor , o::bxor , o::null , o::null  }
            };
            static robin_hood::unordered_map<ot, u8> possible_map = {
                { ot::add       , 0  },
                { ot::sub       , 1  },
                { ot::mul       , 2  },
                { ot::div       , 3  },
                { ot::mod       , 4  },
                { ot::less      , 5  },
                { ot::greater   , 6  },
                { ot::lessEq    , 7  },
                { ot::greaterEq , 8  },
                { ot::notEq     , 9  },
                { ot::isEq      , 10 },
                { ot::shiftLeft , 11 },
                { ot::shiftRight, 12 },
                { ot::land      , 13 },
                { ot::lor       , 14 },
                { ot::band      , 15 },
                { ot::bor       , 16 },
                { ot::bxor      , 17 }
            };

            if (!tp->is_floating_point) {
                if (!tp->is_unsigned) {
                    return possible_arr[possible_map[op]][0];
                } else {
                    return possible_arr[possible_map[op]][1];
                }
            } else {
                if (tp->size == sizeof(f64)) {
                    return possible_arr[possible_map[op]][2];
                } else {
                    return possible_arr[possible_map[op]][3];
                }
            }

            return o::null;
        }

        var do_bin_op(context* ctx, const var& a, const var& b, ot _op) {
            if (a.flag(bind::pf_write_only) || b.flag(bind::pf_write_only)) {
                ctx->log()->err(ec::c_no_read_write_only, ctx->node()->ref);
                return ctx->error_var();
            }

            operation op = get_op(_op, a.type());
            if ((u8)op) {
                var ret = ctx->empty_var(a.type());
                var v = b.convert(a.type());
                ctx->add(op).operand(ret).operand(a).operand(v);
                return ret;
            }
            else {
                script_function* f = a.method(std::string("operator ") + ot_str[(u8)_op], a.type(), { a.call_this_tp(), b.type() });
                if (f) {
                    return call(*ctx, f, { a, b.convert(f->signature.arg_types[0]) });
                }
            }

            return ctx->dummy_var(a.type());
        }

        var var::operator + (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::add);
        }

        var var::operator - (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::sub);
        }

        var var::operator * (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::mul);
        }

        var var::operator / (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::div);
        }

        var var::operator % (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::mod);
        }

        var var::operator << (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::shiftLeft);
        }

        var var::operator >> (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::shiftRight);
        }

        var var::operator && (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::land);
        }

        var var::operator || (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::lor);
        }

        var var::operator & (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::band);
        }

        var var::operator | (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::bor);
        }

        var var::operator ^ (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::bxor);
        }

        var var::operator += (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::addEq);
            operator_eq(result);
            return result;
        }

        var var::operator -= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::subEq);
            operator_eq(result);
            return result;
        }

        var var::operator *= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::mulEq);
            operator_eq(result);
            return result;
        }

        var var::operator /= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::divEq);
            operator_eq(result);
            return result;
        }

        var var::operator %= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::modEq);
            operator_eq(result);
            return result;
        }

        var var::operator <<= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::shiftLeftEq);
            operator_eq(result);
            return result;
        }

        var var::operator >>= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::shiftRightEq);
            operator_eq(result);
            return result;
        }

        var var::operator_landEq (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::landEq);
            operator_eq(result);
            return result;
        }

        var var::operator_lorEq (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::lorEq);
            operator_eq(result);
            return result;
        }

        var var::operator &= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::bandEq);
            operator_eq(result);
            return result;
        }

        var var::operator |= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::borEq);
            operator_eq(result);
            return result;
        }

        var var::operator ^= (const var& rhs) const {
            var result = do_bin_op(m_ctx, *this, rhs, ot::bxorEq);
            operator_eq(result);
            return result;
        }

        var var::operator ++ () const {
            if (m_type->is_primitive) {
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::add);
                operator_eq(result);
                return result;
            }

            script_function* f = method("operator ++", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
            return m_ctx->error_var();
        }

        var var::operator -- () const {
            if (m_type->is_primitive) {
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
                operator_eq(result);
                return result;
            }

            script_function* f = method("operator --", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
            return m_ctx->error_var();
        }

        var var::operator ++ (int) const {
            if (m_type->is_primitive) {
                var clone = m_ctx->empty_var(m_type);
                clone.operator_eq(*this);
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::add);
                operator_eq(result);
                return result;
            }

            var clone = m_ctx->empty_var(m_type);
            // todo: specific error when type is not copy constructible 
            clone.raise_stack_flag();
            construct_on_stack(*m_ctx, clone, { *this });
            script_function* f = method("operator ++", m_type, { call_this_tp() });
            if (f) call(*m_ctx, f, { *this });
            return clone;
        }

        var var::operator -- (int) const {
            if (m_type->is_primitive) {
                var clone = m_ctx->empty_var(m_type);
                clone.operator_eq(*this);
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
                operator_eq(result);
                return result;
            }

            var clone = m_ctx->empty_var(m_type);
            // todo: specific error when type is not copy constructible
            clone.raise_stack_flag();
            construct_on_stack(*m_ctx, clone, { *this });
            script_function* f = method("operator --", m_type, { call_this_tp() });
            if (f) call(*m_ctx, f, { *this });
            return clone;
        }

        var var::operator < (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::less);
        }

        var var::operator > (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::greater);
        }

        var var::operator <= (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::lessEq);
        }

        var var::operator >= (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::greaterEq);
        }

        var var::operator != (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::notEq);
        }

        var var::operator == (const var& rhs) const {
            return do_bin_op(m_ctx, *this, rhs, ot::isEq);
        }

        var var::operator ! () const {
            if (m_type->is_primitive) {
                return do_bin_op(m_ctx, *this, m_ctx->imm(i64(0)), ot::isEq);
            }

            script_function* f = method("operator !", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
            return m_ctx->error_var();
        }

        var var::operator - () const {
            if (m_type->is_primitive) {
                if (m_flags & bind::pf_write_only) {
                    m_ctx->log()->err(ec::c_no_read_write_only, m_ctx->node()->ref);
                    return m_ctx->error_var();
                }

                var v = m_ctx->empty_var(m_type);
                m_ctx->add(operation::neg).operand(v).operand(*this);
                return v;
            }

            script_function* f = method("operator -", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
            return m_ctx->error_var();
        }

        var var::operator_eq (const var& rhs) const {
            if (m_flags & bind::pf_read_only) {
                m_ctx->log()->err(ec::c_no_assign_read_only, m_ctx->node()->ref);
                return *this;
            }
            if (rhs.m_flags & bind::pf_write_only) {
                m_ctx->log()->err(ec::c_no_read_write_only, m_ctx->node()->ref);
                return *this;
            }

            if (m_setter.func) {
                var real_value = call(*m_ctx, m_setter.func, { *m_setter.this_obj, rhs });
                m_ctx->add(operation::eq).operand(*this).operand(real_value);
            } else {
                // todo: operator =, remove 'adopt_stack_flag'
                if (m_type->is_primitive) {
                    var v = rhs.convert(m_type);
                    m_ctx->add(operation::eq).operand(*this).operand(v);
                    if (m_mem_ptr.valid) {
                        m_ctx->add(operation::store).operand(var(m_ctx, m_mem_ptr.reg, m_ctx->type("data"))).operand(*this);
                    }
                } else {
                    script_function* assign = method("operator =", m_type, { rhs.m_type, rhs.m_type });
                    if (assign) call(*m_ctx, assign, { *this, rhs });
                }
            }

            return *this;
        }

        var var::operator [] (const var& rhs) const {
            script_function* f = method("operator []", nullptr, { call_this_tp(), rhs.type() });
            if (f) {
                return call(*m_ctx, f, { *this, rhs });
            }

            return m_ctx->error_var();
        }
    };
};