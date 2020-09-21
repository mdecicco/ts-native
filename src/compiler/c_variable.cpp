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
        bool has_valid_conversion(script_type* from, script_type* to) {
            if (from->id() == to->id()) return true;
            if (from->base_type && from->base_type->id() == to->id()) return true;
            if (!from->is_primitive && to->name == "data") return true;
            if (from->name == "data" && !to->is_primitive) return true;

            if (!from->is_primitive) {
                for (u16 m = 0;m < from->methods.size();m++) {
                    std::vector<std::string> mparts = split(split(from->methods[m]->name, ":")[1], " \t\n\r");
                    if (mparts.size() != 2) continue;
                    bool matched = true;
                    if (mparts[0] == "operator" && mparts[1] == to->name) {
                        // cast function exists
                        return true;
                    }
                }

                return false;
            }

            if (!to->is_primitive) {
                for (u16 m = 0;m < to->methods.size();m++) {
                    std::vector<std::string> mparts = split(split(to->methods[m]->name, ":")[1], " \t\n\r");
                    if (mparts.size() != 1) continue;
                    if (mparts[0] != "constructor") continue;
                    script_function* ctor = to->methods[m];
                    if (ctor->signature.arg_types.size() != 2) continue;
                    script_type* copyFromTp = ctor->signature.arg_types[1];
                    if (copyFromTp->id() == from->id()) return true;
                    if (copyFromTp->name == "subtype" && has_valid_conversion(from, copyFromTp->sub_type)) return true;
                }

                return false;
            }

            if (from->name == "void" || to->name == "void") return false;

            // conversion between primitive types is always possible
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
            m_is_stack_obj = false;
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
            m_is_stack_obj = false;
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
            m_is_stack_obj = false;
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
            m_is_stack_obj = false;
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
            m_imm.u = 0;
            m_is_stack_obj = false;
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
            m_imm.u = 0;
            m_is_stack_obj = false;
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
            m_imm.u = 0;
            m_is_stack_obj = false;
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
            m_mem_ptr = v.m_mem_ptr;
            m_name = v.m_name;
            m_is_stack_obj = v.m_is_stack_obj;
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
                        if (p.flags & bind::pf_static) {
                            m_ctx->add(operation::eq).operand(ptr).operand(m_ctx->imm(p.offset));
                        } else {
                            m_ctx->add(operation::uadd).operand(ptr).operand(*this).operand(m_ctx->imm(p.offset));
                        }
                        var ret = m_ctx->empty_var(p.type);
                        ret.set_mem_ptr(ptr);
                        m_ctx->add(operation::load).operand(ret).operand(ptr);

                        if (p.setter) {
                            ret.m_setter.this_obj.reset(new var(*this));
                            ret.m_setter.func = p.setter;
                        }
                        ret.m_flags = p.flags;
                        return ret;
                    }
                }
            }

            throw exc(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
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

            throw exc(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
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
                if (_name.find_first_of(' ') != std::string::npos) {
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
                if (ret && !has_valid_conversion(ret, func->signature.return_type)) continue;
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
                        if (!has_valid_conversion(args[i], at)) {
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
                if (_name.find_first_of(' ') != std::string::npos) {
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
                if (ret && !has_valid_conversion(func->signature.return_type, ret)) continue;
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
                        if (!has_valid_conversion(args[i], at)) {
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
            return has_valid_conversion(m_type, tp);
        }

        var var::convert(script_type* tp) const {
            return *this;
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
            operation op = get_op(_op, a.type());
            if ((u8)op) {
                var ret = ctx->empty_var(a.type());
                ctx->add(op).operand(ret).operand(a).operand(b.convert(a.type()));
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
        }

        var var::operator -- () const {
            if (m_type->is_primitive) {
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
                operator_eq(result);
                return result;
            }

            script_function* f = method("operator --", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
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
            construct_on_stack(*m_ctx, clone, { *this });
            clone.raise_stack_flag();
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
            construct_on_stack(*m_ctx, clone, { *this });
            clone.raise_stack_flag();
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
        }

        var var::operator - () const {
            if (m_type->is_primitive) {
                var v = m_ctx->empty_var(m_type);
                m_ctx->add(operation::neg).operand(v).operand(*this);
                return v;
            }

            script_function* f = method("operator -", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, { *this });
        }

        var var::operator_eq (const var& rhs) const {
            if (m_setter.func) {
                var real_value = call(*m_ctx, m_setter.func, { *m_setter.this_obj, rhs });
                m_ctx->add(operation::eq).operand(*this).operand(real_value);
            } else {
                m_ctx->add(operation::eq).operand(*this).operand(rhs.convert(m_type));
                if (m_mem_ptr.valid) {
                    m_ctx->add(operation::store).operand(var(m_ctx, m_mem_ptr.reg, m_ctx->type("data"))).operand(*this);
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