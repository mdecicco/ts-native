#include <gjs/compiler/variable.h>
#include <gjs/compiler/context.h>
#include <gjs/compiler/function.h>
#include <gjs/compiler/compile.h>
#include <gjs/parser/ast.h>
#include <gjs/common/errors.h>
#include <gjs/common/warnings.h>
#include <gjs/common/script_context.h>
#include <gjs/common/script_type.h>
#include <gjs/common/script_function.h>
#include <gjs/common/function_signature.h>
#include <gjs/common/script_module.h>
#include <gjs/builtin/script_array.h>
#include <gjs/util/util.h>
#include <gjs/util/robin_hood.h>
#include <cmath>

namespace gjs {
    using exc = error::exception;
    using ec = error::ecode;
    using wc = warning::wcode;

    namespace compile {
        var::var(context* ctx, u64 u) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_reg_id = -1;
            m_parent_reg_id = -1;
            m_imm.u = u;
            m_type = ctx->type("u64");
            m_stack_loc = -1;
            m_arg_idx = -1;
            m_stack_id = 0;
            m_mem_ptr.valid = false;
            m_mem_ptr.reg = -1;
            m_flags = 0;
            m_setter.this_obj = nullptr;
            m_setter.func = nullptr;
        }

        var::var(context* ctx, i64 i) {
            m_ctx = ctx;
            m_is_imm = true;
            m_instantiation = ctx->node()->ref;
            m_flags = 0;
            m_reg_id = -1;
            m_parent_reg_id = -1;
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
            m_flags = 0;
            m_reg_id = -1;
            m_parent_reg_id = -1;
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
            m_flags = 0;
            m_reg_id = -1;
            m_parent_reg_id = -1;
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
            m_flags = 0;
            m_reg_id = -1;
            m_parent_reg_id = -1;
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
            m_flags = 0;
            m_reg_id = reg_id;
            m_parent_reg_id = -1;
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
            m_flags = 0;
            m_reg_id = -1;
            m_parent_reg_id = -1;
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
            m_parent_reg_id = v.m_parent_reg_id;
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
            m_ctx = nullptr;
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

            return has_any_method(prop);
        }

        bool var::has_parent() const {
            return m_parent_reg_id != -1;
        }

        u32 var::parent_reg_id() const {
            return m_parent_reg_id;
        }

        var var::prop(const std::string& prop, bool log_errors) const {
            if (m_type->is_host && m_type->base_type && m_type->base_type->name == "array" && prop == "length") {
                // optimization specifically for array length access
                var v = m_ctx->empty_var(m_ctx->type("u32"));
                var ptr = m_ctx->empty_var(m_ctx->type("data"));
                m_ctx->add(operation::uadd).operand(ptr).operand(*this).operand(m_ctx->imm((u64)offsetof(script_array, m_count)));
                m_ctx->add(operation::load).operand(v).operand(ptr);
                v.m_flags = (u8)property_flags::pf_read_only;
                return v;
            }

            for (u16 i = 0;i < m_type->properties.size();i++) {
                auto& p = m_type->properties[i];
                if (p.name == prop) {
                    if (p.getter) {
                        var v = call(*m_ctx, p.getter, {}, this);
                        if (p.setter) {
                            v.m_setter.this_obj.reset(new var(*this));
                            v.m_setter.func = p.setter;
                        }
                        v.m_flags = p.flags;
                        return v;
                    } else {
                        var ptr = m_ctx->empty_var(m_ctx->type("u64"));
                        var ret = m_ctx->empty_var(p.type);
                        if (p.flags & u8(property_flags::pf_static)) {
                            if (p.type->is_host) {
                                // offset points to absolute memory location
                                m_ctx->add(operation::eq).operand(ptr).operand(m_ctx->imm(p.offset));
                                
                                if (p.flags & u8(property_flags::pf_pointer)) {
                                    // ptr points to a pointer to the value. Get pointer to value in ptr
                                    m_ctx->add(operation::load).operand(ptr).operand(ptr);

                                    if (p.type->is_primitive) {
                                        // load value of primitive to ret
                                        m_ctx->add(operation::load).operand(ret).operand(ptr);
                                    } else {
                                        // get pointer to value in ret
                                        m_ctx->add(operation::eq).operand(ret).operand(ptr);
                                    }
                                } else {
                                    if (p.type->is_primitive) {
                                        // load value of primitive to ret
                                        m_ctx->add(operation::load).operand(ret).operand(m_ctx->imm(p.offset));
                                    } else {
                                        // get pointer to value in ret
                                        m_ctx->add(operation::eq).operand(ret).operand(m_ctx->imm(p.offset));
                                    }
                                }
                            } else {
                                // offset points to module memory location
                                m_ctx->add(operation::module_data).operand(ptr).operand(m_ctx->imm(u64(p.type->owner->id()))).operand(m_ctx->imm(p.offset));
                                if (p.type->is_primitive) {
                                    // load value of primitive to ret
                                    m_ctx->add(operation::load).operand(ret).operand(ptr);
                                } else {
                                    // get pointer to value in ret
                                    m_ctx->add(operation::eq).operand(ret).operand(ptr);
                                }
                            }
                        } else {
                            m_ctx->add(operation::uadd).operand(ptr).operand(*this).operand(m_ctx->imm(p.offset));
                            if (p.flags & u8(property_flags::pf_pointer)) {
                                // ptr points to a pointer to the value. Get pointer to value in ptr
                                m_ctx->add(operation::load).operand(ptr).operand(ptr);

                                if (p.type->is_primitive) {
                                    // load value of primitive to ret
                                    m_ctx->add(operation::load).operand(ret).operand(ptr);
                                } else {
                                    // get pointer to value in ret
                                    m_ctx->add(operation::eq).operand(ret).operand(ptr);
                                }
                            } else {
                                if (p.type->is_primitive) {
                                    // load value of primitive to ret
                                    m_ctx->add(operation::load).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
                                } else {
                                    // get pointer to value in ret
                                    m_ctx->add(operation::eq).operand(ret).operand(ptr);
                                }
                            }
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

            script_function* method = nullptr;
            for (u16 i = 0;i < m_type->methods.size();i++) {
                if (m_type->methods[i]->name == m_type->name + "::" + prop) {
                    if (method) {
                        if (log_errors) m_ctx->log()->err(ec::c_ambiguous_method_name, m_ctx->node()->ref, prop.c_str());
                        return m_ctx->error_var();
                    }
                    method = m_type->methods[i];
                }
            }

            if (method) return m_ctx->func_var(method);

            if (log_errors) m_ctx->log()->err(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
            return m_ctx->error_var();
        }

        var var::prop_ptr(const std::string& prop, bool log_errors) const {
            for (u16 i = 0;i < m_type->properties.size();i++) {
                auto& p = m_type->properties[i];
                if (p.name == prop) {
                    if (p.flags & u8(property_flags::pf_pointer)) {
                        var ptr = m_ctx->empty_var(p.type);
                        if (p.flags & u8(property_flags::pf_static)) {
                            m_ctx->add(operation::load).operand(ptr).operand(m_ctx->imm(p.offset));
                        } else {
                            m_ctx->add(operation::load).operand(ptr).operand(*this).operand(m_ctx->imm(p.offset));
                        }
                        return ptr;
                    }

                    if (p.flags & u8(property_flags::pf_static)) {
                        return m_ctx->imm(p.offset);
                    } else {
                        var ret = m_ctx->empty_var(p.type);
                        m_ctx->add(operation::uadd).operand(ret).operand(*this).operand(m_ctx->imm(p.offset));
                        return ret;
                    }
                }
            }
            
            if (log_errors) m_ctx->log()->err(ec::c_no_such_property, m_ctx->node()->ref, m_type->name.c_str(), prop.c_str());
            return m_ctx->error_var();
        }

        bool var::has_any_method(const std::string& _name) const {
            return m_type->has_any_method(_name);
        }

        bool var::has_unambiguous_method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args) const {
            std::vector<script_function*> matches;

            for (u16 m = 0;m < m_type->methods.size();m++) {
                // match name
                script_function* func = nullptr;
                if (_name == "<cast>") {
                    script_function* mt = m_type->methods[m];
                    auto mparts = split(split(mt->name,":")[1], " \t\n\r");
                    if (mparts.size() == 2 && mparts[0] == "operator" && mparts[1] == mt->type->signature->return_type->name && mt->type->signature->explicit_argc == 1) {
                        if (mt->type->signature->return_type->id() == ret->id()) {
                            return mt;
                        } else if (mt->type->signature->return_type->is_convertible_to(ret)) {
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

                if (_name == "constructor" && func->type->signature->explicit_argc == 1 && func->type->signature->explicit_arg(0).tp->id() == m_type->id()) {
                    // don't match the copy constructor unless the requested arguments strictly match
                    if (args.size() == 1 && args[0]->id() == m_type->id()) matches.push_back(func);
                    continue;
                }

                // match return type
                if (ret && !ret->is_convertible_to(func->type->signature->return_type)) continue;
                bool ret_tp_strict = ret ? func->type->signature->return_type->id() == ret->id() : func->type->signature->return_type->size == 0;

                // match argument types
                if (func->type->signature->explicit_argc != args.size()) continue;

                // prefer strict type matches
                bool match = true;
                for (u8 i = 0;i < args.size();i++) {
                    if (func->type->signature->explicit_arg(i).tp->name == "subtype") {
                        if (m_type->sub_type->id() != args[i]->id()) {
                            match = false;
                            break;
                        }
                    } else {
                        if (func->type->signature->explicit_arg(i).tp->id() != args[i]->id()) {
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
                        script_type* at = func->type->signature->explicit_arg(i).tp;
                        if (at->name == "subtype") at = m_type->sub_type;
                        if (!args[i]->is_convertible_to(at)) {
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

        script_function* var::method(const std::string& _name, script_type* ret, const std::vector<script_type*>& args, bool log_errors) const {
            bool wasAmbiguous = false;
            script_function* f = m_type->method(_name, ret, args, &wasAmbiguous);

            if (log_errors) {
                if (wasAmbiguous) {
                    m_ctx->log()->err(
                        ec::c_ambiguous_method,
                        m_ctx->node()->ref,
                        _name.c_str(),
                        m_type->name.c_str(),
                        arg_tp_str(args).c_str(),
                        !ret ? "<any>" : ret->name.c_str()
                    );
                }
                else if (!f) {
                    m_ctx->log()->err(
                        ec::c_no_such_method,
                        m_ctx->node()->ref,
                        m_type->name.c_str(),
                        _name.c_str(),
                        arg_tp_str(args).c_str(),
                        !ret ? "<any>" : ret->name.c_str()
                    );
                }
            }

            return f;
        }

        bool var::convertible_to(script_type* tp) const {
            return m_type->is_convertible_to(tp);
        }

        var var::convert(script_type* to, bool store_imms_in_reg) const {
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
                    script_function* cast = method("<cast>", to, {});
                    if (cast) {
                        var ret = call(*m_ctx, cast, {}, this);
                        if (ret.type()->id() != to->id()) {
                            return ret.convert(to);
                        }

                        return ret;
                    }

                    m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                    return m_ctx->error_var();
                } else if (has_unambiguous_method("<cast>", to, {})) {
                    script_function* cast = method("<cast>", to, {});
                    return call(*m_ctx, cast, {}, this);
                }
            }

            if (!to->is_primitive) {
                var tv = m_ctx->dummy_var(to);
                script_function* ctor = tv.method("constructor", to, { from });
                if (ctor) {
                    var ret = m_ctx->empty_var(to);
                    ret.add_to_stack();
                    construct_on_stack(*m_ctx, ret, { *this });
                    return ret;
                }
                
                m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                return m_ctx->error_var();
            }

            if (to->signature) {
                // the from->id() == to->id() check above handles the only valid case
                // for function signature types
                m_ctx->log()->err(ec::c_no_valid_conversion, m_ctx->node()->ref, from->name.c_str(), to->name.c_str());
                return m_ctx->error_var();
            }
            
            // conversion between non-void primitive types is always possible
            if (m_is_imm) {
                var r;
                if (from->is_floating_point) {
                    if (to->is_floating_point) {
                        if (from->size == sizeof(f64)) r = m_ctx->imm((f32)m_imm.f);
                        else r = m_ctx->imm((f64)m_imm.d);
                    } else {
                        if (to->is_unsigned) {
                            if (from->size == sizeof(f64)) r = m_ctx->imm((u64)m_imm.d, to);
                            else r = m_ctx->imm((u64)m_imm.f, to);
                        }
                        else {
                            if (from->size == sizeof(f64)) r = m_ctx->imm((i64)m_imm.d, to);
                            else r = m_ctx->imm((i64)m_imm.i, to);
                        }
                    }
                } else {
                    if (to->is_floating_point) {
                        if (to->size == sizeof(f64)) {
                            if (from->is_unsigned) r = m_ctx->imm((f64)m_imm.u);
                            else r = m_ctx->imm((f64)m_imm.i);
                        } else {
                            if (from->is_unsigned) r = m_ctx->imm((f32)m_imm.u);
                            else r = m_ctx->imm((f32)m_imm.i);
                        }
                    } else {
                        if (to->is_unsigned) {
                            if (from->is_unsigned) r = m_ctx->imm(m_imm.u, to);
                            else r = m_ctx->imm((u64)m_imm.i, to);
                        }
                        else {
                            if (from->is_unsigned) r = m_ctx->imm((i64)m_imm.u, to);
                            else r = m_ctx->imm(m_imm.i, to);
                        }
                    }
                }

                if (store_imms_in_reg) {
                    var v = m_ctx->empty_var(to);
                    m_ctx->add(operation::eq).operand(v).operand(r);
                    return v;
                }

                return r;
            }

            var v = m_ctx->empty_var(to);
            m_ctx->add(operation::eq).operand(v).operand(*this);

            u64 from_id = join_u32(from->owner->id(), from->id());
            u64 to_id = join_u32(to->owner->id(), to->id());
            m_ctx->add(operation::cvt).operand(v).operand(m_ctx->imm(from_id)).operand(m_ctx->imm(to_id));
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

            static bool showStackInfo = false;
            if (is_arg()) {
                if (m_name.length() > 0) {
                    if (m_type->is_floating_point) {
                        if (m_stack_id > 0 && showStackInfo) return format("$FPA%d (%s) <$sp[%d]>", m_arg_idx, m_name.c_str(), m_stack_id);
                        else return format("$FPA%d (%s)", m_arg_idx, m_name.c_str());
                    }
                    if (m_stack_id > 0 && showStackInfo) return format("$GPA%d (%s) <$sp[%d]>", m_arg_idx, m_name.c_str(), m_stack_id);
                    else return format("$GPA%d (%s)", m_arg_idx, m_name.c_str());
                }

                if (m_type->is_floating_point) {
                    if (m_stack_id > 0 && showStackInfo) return format("$FPA%d <$sp[%d]>", m_arg_idx, m_stack_id);
                    else return format("$FPA%d", m_arg_idx);
                }
                if (m_stack_id > 0 && showStackInfo) return format("$GPA%d <$sp[%d]>", m_arg_idx, m_stack_id);
                else return format("$GPA%d", m_arg_idx);
            } else {
                if (m_name.length() > 0) {
                    if (m_type->is_floating_point) {
                        if (m_stack_id > 0 && showStackInfo) return format("$FP%d (%s) <$sp[%d]>", m_reg_id, m_name.c_str(), m_stack_id);
                        else return format("$FP%d (%s)", m_reg_id, m_name.c_str());
                    }
                    if (m_stack_id > 0 && showStackInfo) return format("$GP%d (%s) <$sp[%d]>", m_reg_id, m_name.c_str(), m_stack_id);
                    else return format("$GP%d (%s)", m_reg_id, m_name.c_str());
                }

                if (m_type->is_floating_point) {
                    if (m_stack_id > 0 && showStackInfo) return format("$FP%d <$sp[%d]>", m_reg_id, m_stack_id);
                    else return format("$FP%d", m_reg_id);
                }
                if (m_stack_id > 0 && showStackInfo) return format("$GP%d <$sp[%d]>", m_reg_id, m_stack_id);
                else return format("$GP%d", m_reg_id);
            }
        }

        void var::set_mem_ptr(const var& v) {
            m_mem_ptr.valid = v.valid();
            m_mem_ptr.reg = v.m_reg_id;
        }

        void var::reserve_stack_id() {
            static u64 next_stack_id = 1;
            m_stack_id = next_stack_id++;
        }

        void var::add_to_stack() {
            if (!m_stack_id) reserve_stack_id();
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
        
        void var::set_register(u32 reg_id){
            m_reg_id = reg_id;
        }
            
        void var::set_stack_loc(u32 stack_loc) {
            m_stack_loc = stack_loc;
        }

        void var::set_parent_reg_id(u32 reg_id) {
            m_parent_reg_id = reg_id;
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
                { ot::addEq     , 0  },
                { ot::subEq     , 1  },
                { ot::mulEq     , 2  },
                { ot::divEq     , 3  },
                { ot::modEq     , 4  },
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

        inline double modf(double x, double y) { return ::modf(x, &y); }
        var solve_imm_bin_op(context* ctx, const var& a, const var& b, ot op) {
            union { f64 f; i64 i; } av;
            union { f64 f; i64 i; } bv;
            bool i[2] = { false, false };
            if (a.type()->is_floating_point) {
                if (a.type()->size == (u32)sizeof(f64)) av.f = a.imm_d();
                else av.f = a.imm_f();
                i[0] = true;
            } else {
                if (a.type()->is_unsigned) av.i = a.imm_u();
                else av.i = a.imm_i();
            }
            if (b.type()->is_floating_point) {
                if (b.type()->size == (u32)sizeof(f64)) bv.f = b.imm_d();
                else bv.f = b.imm_f();
                i[1] = true;
            } else {
                if (b.type()->is_unsigned) bv.i = b.imm_u();
                else bv.i = b.imm_i();
            }

            switch (op) {
                case ot::add: {
                    if (i[0] && i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f + bv.f);
                        else return ctx->imm((f32)(av.f + bv.f));
                    }
                    if (i[0] && !i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f + bv.i);
                        else return ctx->imm((f32)(av.f + bv.i));
                    }
                    if (!i[0] &&  i[1]) return ctx->imm(av.i + bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i + bv.i);
                    break;
                }
                case ot::sub: {
                    if (i[0] && i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f - bv.f);
                        else return ctx->imm((f32)(av.f - bv.f));
                    }
                    if (i[0] && !i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f - bv.i);
                        else return ctx->imm((f32)(av.f - bv.i));
                    }
                    if (!i[0] &&  i[1]) return ctx->imm(av.i - bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i - bv.i);
                    break;
                }
                case ot::mul: {
                    if (i[0] && i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f * bv.f);
                        else return ctx->imm((f32)(av.f * bv.f));
                    }
                    if (i[0] && !i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f * bv.i);
                        else return ctx->imm((f32)(av.f * bv.i));
                    }
                    if (!i[0] &&  i[1]) return ctx->imm(av.i * bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i * bv.i);
                    break;
                }
                case ot::div: {
                    if (i[0] && i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f / bv.f);
                        else return ctx->imm((f32)(av.f / bv.f));
                    }
                    if (i[0] && !i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(av.f / bv.i);
                        else return ctx->imm((f32)(av.f / bv.i));
                    }
                    if (!i[0] &&  i[1]) return ctx->imm(av.i / bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i / bv.i);
                    break;
                }
                case ot::mod: {
                    if (i[0] && i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(modf(av.f, bv.f));
                        else return ctx->imm((f32)modf(av.f, bv.f));
                    }
                    if (i[0] && !i[1]) {
                        if (a.type()->size == (u32)sizeof(f64)) return ctx->imm(modf(av.f, (f64)bv.i));
                        else return ctx->imm((f32)modf(av.f, (f64)bv.i));
                    }
                    if ( i[0] &&  i[1]) return ctx->imm(modf(av.f, bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(modf(av.f, bv.f));
                    if (!i[0] &&  i[1]) return ctx->imm(modf(f64(av.i), bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(f64(av.i % bv.i));
                    break;
                }
                case ot::shiftLeft: {
                    if ( i[0] &&  i[1]) return ctx->imm((i64)av.f << (i64)bv.f);
                    if ( i[0] && !i[1]) return ctx->imm((i64)av.f << bv.i);
                    if (!i[0] &&  i[1]) return ctx->imm(av.i << (i64)bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i << bv.i);
                    break;
                }
                case ot::shiftRight: {
                    if ( i[0] &&  i[1]) return ctx->imm((i64)av.f >> (i64)bv.f);
                    if ( i[0] && !i[1]) return ctx->imm((i64)av.f >> bv.i);
                    if (!i[0] &&  i[1]) return ctx->imm(av.i >> (i64)bv.f);
                    if (!i[0] && !i[1]) return ctx->imm(av.i >> bv.i);
                    break;
                }
                case ot::land: {
                    // 0 == 0.0f, !0 == !0.0f
                    return ctx->imm(i64(av.i && bv.i));
                }
                case ot::lor: {
                    // 0 == 0.0f, !0 == !0.0f
                    return ctx->imm(i64(av.i || bv.i));
                }
                case ot::band: {
                    if (!i[0] && !i[1]) return ctx->imm(av.i & bv.i);
                    else {
                        i64 x = av.i & bv.i;
                        return ctx->imm(*(f64*)&x);
                    }
                }
                case ot::bor: {
                    if (!i[0] && !i[1]) return ctx->imm(av.i | bv.i);
                    else {
                        i64 x = av.i | bv.i;
                        return ctx->imm(*(f64*)&x);
                    }
                }
                case ot::bxor: {
                    if (!i[0] && !i[1]) return ctx->imm(av.i ^ bv.i);
                    else {
                        i64 x = av.i ^ bv.i;
                        return ctx->imm(*(f64*)&x);
                    }
                }
                case ot::less: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f < bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f < f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) < bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i < bv.i));
                    break;
                }
                case ot::greater: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f > bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f > f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) > bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i > bv.i));
                    break;
                }
                case ot::lessEq: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f <= bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f <= f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) <= bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i <= bv.i));
                    break;
                }
                case ot::greaterEq: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f >= bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f >= f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) >= bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i >= bv.i));
                    break;
                }
                case ot::notEq: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f != bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f != f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) != bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i != bv.i));
                    break;
                }
                case ot::isEq: {
                    if ( i[0] &&  i[1]) return ctx->imm(i64(av.f == bv.f));
                    if ( i[0] && !i[1]) return ctx->imm(i64(av.f == f64(bv.i)));
                    if (!i[0] &&  i[1]) return ctx->imm(i64(f64(av.i) == bv.f));
                    if (!i[0] && !i[1]) return ctx->imm(i64(av.i == bv.i));
                    break;
                }
                case ot::addEq: [[fallthrough]];
                case ot::subEq: [[fallthrough]];
                case ot::mulEq: [[fallthrough]];
                case ot::divEq: [[fallthrough]];
                case ot::modEq: [[fallthrough]];
                case ot::shiftLeftEq: [[fallthrough]];
                case ot::shiftRightEq: [[fallthrough]];
                case ot::landEq: [[fallthrough]];
                case ot::lorEq: [[fallthrough]];
                case ot::bandEq: [[fallthrough]];
                case ot::borEq: [[fallthrough]];
                case ot::bxorEq: {
                    ctx->log()->err(ec::c_no_assign_literal, ctx->node()->ref);
                    return ctx->error_var();
                }
            }

            ctx->log()->err(ec::c_invalid_binary_operator, ctx->node()->ref);
            return ctx->error_var();
        }

        var do_bin_op(context* ctx, const var& a, const var& b, ot _op, bool* usedCustomOperator = nullptr) {
            if (a.flag(property_flags::pf_write_only) || b.flag(property_flags::pf_write_only)) {
                ctx->log()->err(ec::c_no_read_write_only, ctx->node()->ref);
                return ctx->error_var();
            }

            if (a.is_imm() && b.is_imm()) return solve_imm_bin_op(ctx, a, b, _op);

            static bool resultIsBoolean[] = {
                false, // null
                false, // load
                false, // store
                false, // stack_alloc
                false, // stack_free
                false, // module_data
                false, // reserve
                false, // resolve
                false, // iadd
                false, // isub
                false, // imul
                false, // idiv
                false, // imod
                false, // uadd
                false, // usub
                false, // umul
                false, // udiv
                false, // umod
                false, // fadd
                false, // fsub
                false, // fmul
                false, // fdiv
                false, // fmod
                false, // dadd
                false, // dsub
                false, // dmul
                false, // ddiv
                false, // dmod
                false, // sl
                false, // sr
                true , // land
                true , // lor
                false, // band
                false, // bor
                false, // bxor
                true , // ilt
                true , // igt
                true , // ilte
                true , // igte
                true , // incmp
                true , // icmp
                true , // ult
                true , // ugt
                true , // ulte
                true , // ugte
                true , // uncmp
                true , // ucmp
                true , // flt
                true , // fgt
                true , // flte
                true , // fgte
                true , // fncmp
                true , // fcmp
                true , // dlt
                true , // dgt
                true , // dlte
                true , // dgte
                true , // dncmp
                true , // dcmp
                false, // eq
                false, // neg
                false, // cvt
                false, // call
                false, // param
                false, // ret
                false, // label
                false, // branch
                false, // jump
                false  // term
            };

            operation op = get_op(_op, a.type());
            if ((u8)op) {
                if (resultIsBoolean[(u8)op]) {
                    var ret = ctx->empty_var(ctx->type("bool"));
                    var v = b.convert(a.type());
                    ctx->add(op).operand(ret).operand(a).operand(v);
                    return ret;
                }

                var ret = ctx->empty_var(a.type());
                var v = b.convert(a.type());
                ctx->add(op).operand(ret).operand(a).operand(v);
                return ret;
            }
            else {
                script_function* f = a.method(std::string("operator ") + ot_str[(u8)_op], a.type(), { b.type() });
                if (f) {
                    if (usedCustomOperator) *usedCustomOperator = true;
                    return call(*ctx, f, { b.convert(f->type->signature->explicit_arg(0).tp) }, &a);
                }
            }

            return ctx->error_var();
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
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::addEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator -= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::subEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator *= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::mulEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator /= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::divEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator %= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::modEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator <<= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::shiftLeftEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator >>= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::shiftRightEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator_landEq (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::landEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator_lorEq (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::lorEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator &= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::bandEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator |= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::borEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator ^= (const var& rhs) const {
            bool usedCustom = false;
            var result = do_bin_op(m_ctx, *this, rhs, ot::bxorEq, &usedCustom);
            if (!usedCustom) operator_eq(result);
            return result;
        }

        var var::operator ++ () const {
            if (m_type->is_primitive) {
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::add);
                operator_eq(result);
                return result;
            }

            script_function* f = method("operator ++", m_type, { call_this_tp() });
            if (f) return call(*m_ctx, f, {}, this);
            return m_ctx->error_var();
        }

        var var::operator -- () const {
            if (m_type->is_primitive) {
                var result = do_bin_op(m_ctx, *this, m_ctx->imm(i64(1)), ot::sub);
                operator_eq(result);
                return result;
            }

            script_function* f = method("operator --", m_type, {});
            if (f) return call(*m_ctx, f, {}, this);
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
            clone.add_to_stack();
            construct_on_stack(*m_ctx, clone, { *this });
            script_function* f = method("operator ++", m_type, {});
            if (f) call(*m_ctx, f, {}, this);
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
            clone.add_to_stack();
            construct_on_stack(*m_ctx, clone, { *this });
            script_function* f = method("operator --", m_type, {});
            if (f) call(*m_ctx, f, {}, this);
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
            if (is_imm()) return m_ctx->imm(i64(m_imm.u == 0));

            if (m_type->is_primitive) {
                return do_bin_op(m_ctx, *this, m_ctx->imm(i64(0)), ot::isEq);
            }

            script_function* f = method("operator !", m_type, {});
            if (f) return call(*m_ctx, f, {}, this);
            return m_ctx->error_var();
        }

        var var::operator - () const {
            if (is_imm()) {
                if (m_type->is_floating_point) {
                    if (m_type->size == sizeof(f64)) return m_ctx->imm(-m_imm.d);
                    else return m_ctx->imm(-m_imm.f);
                } else {
                    if (m_type->is_unsigned) return m_ctx->imm(-i64(m_imm.u));
                    else return m_ctx->imm(-m_imm.i);
                }
            }

            if (m_type->is_primitive) {
                if (flag(property_flags::pf_write_only)) {
                    m_ctx->log()->err(ec::c_no_read_write_only, m_ctx->node()->ref);
                    return m_ctx->error_var();
                }

                var v = m_ctx->empty_var(m_type);
                m_ctx->add(operation::neg).operand(v).operand(*this);
                return v;
            }

            script_function* f = method("operator -", m_type, {});
            if (f) return call(*m_ctx, f, {}, this);
            return m_ctx->error_var();
        }

        var var::operator_eq (const var& rhs) const {
            if (is_imm()) {
                m_ctx->log()->err(ec::c_no_assign_literal, m_ctx->node()->ref);
                return m_ctx->error_var();
            }

            if (flag(property_flags::pf_read_only)) {
                m_ctx->log()->err(ec::c_no_assign_read_only, m_ctx->node()->ref);
                return *this;
            }
            if (rhs.flag(property_flags::pf_write_only)) {
                m_ctx->log()->err(ec::c_no_read_write_only, m_ctx->node()->ref);
                return *this;
            }

            if (m_setter.func) {
                var real_value = call(*m_ctx, m_setter.func, { rhs }, m_setter.this_obj.get());
                m_ctx->add(operation::eq).operand(*this).operand(real_value);
            } else {
                if (m_type->is_primitive) {
                    var v = rhs.convert(m_type);
                    m_ctx->add(operation::eq).operand(*this).operand(v);
                    if (m_mem_ptr.valid) {
                        /*
                        if (v.m_is_imm) {
                            var sv = m_ctx->empty_var(v.m_type);
                            m_ctx->add(operation::eq).operand(sv).operand(v);
                            m_ctx->add(operation::store).operand(var(m_ctx, m_mem_ptr.reg, m_type)).operand(sv);
                        }
                        else m_ctx->add(operation::store).operand(var(m_ctx, m_mem_ptr.reg, m_type)).operand(*this);
                        */
                        m_ctx->add(operation::store).operand(var(m_ctx, m_mem_ptr.reg, m_ctx->type("data"))).operand(*this);
                    }
                } else {
                    script_function* assign = method("operator =", m_type, { rhs.m_type });
                    if (assign) call(*m_ctx, assign, { rhs }, this);
                }
            }

            return *this;
        }

        var var::operator [] (const var& rhs) const {
            if (m_type->is_host && m_type->base_type && m_type->base_type->name == "array") {
                script_type* u64t = m_ctx->type("u64");
                // optimization specifically for array element access
                var idx = rhs.convert(u64t);
                var v = m_ctx->empty_var(m_type->sub_type);
                v.set_parent_reg_id(m_reg_id);

                var ptr = m_ctx->empty_var(m_ctx->type("data"));
                // ptr = &this.m_data
                m_ctx->add(operation::uadd).operand(ptr).operand(*this).operand(m_ctx->imm((u64)offsetof(script_array, m_data)));
                
                // ptr = *ptr
                m_ctx->add(operation::load).operand(ptr).operand(ptr);

                // ptr += (idx * elem_size)
                var off = m_ctx->empty_var(u64t);
                m_ctx->add(operation::umul).operand(off).operand(idx).operand(m_ctx->imm((u64)v.size()));
                m_ctx->add(operation::uadd).operand(ptr).operand(ptr).operand(off);

                if (v.type()->is_primitive) {
                    m_ctx->add(operation::load).operand(v).operand(ptr);
                    v.set_mem_ptr(ptr);
                    return v;
                } else {
                    m_ctx->add(operation::eq).operand(v).operand(ptr);
                    return v;
                }
            }

            script_function* f = method("operator []", nullptr, { rhs.type() });
            if (f) {
                return call(*m_ctx, f, { rhs }, this);
            }

            return m_ctx->error_var();
        }
    };
};
