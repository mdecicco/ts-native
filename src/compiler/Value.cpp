#include <tsn/compiler/Value.hpp>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/common/Module.h>

#include <utils/Buffer.hpp>

using namespace tsn::ffi;
using namespace utils;

namespace tsn {
    namespace compiler {
        Value::Value() : ITypedObject(nullptr) {
            m_func = nullptr;
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 0;
            m_slotId = 0;
            m_imm.u = 0;
        }

        Value::Value(const Value& o) : ITypedObject(o.m_type) {
            reset(o);
        }

        Value::Value(FunctionDef* o, DataType* tp) : ITypedObject(tp) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 0;
            m_slotId = 0;
            m_imm.u = 0;
        }

        Value::Value(FunctionDef* o, u64 imm) : ITypedObject(o->getContext()->getTypes()->getUInt64()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
            m_imm.u = imm;
        }

        Value::Value(FunctionDef* o, i64 imm) : ITypedObject(o->getContext()->getTypes()->getType<i64>()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
            m_imm.i = imm;
        }

        Value::Value(FunctionDef* o, f64 imm) : ITypedObject(o->getContext()->getTypes()->getType<f64>()) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
            m_imm.f = imm;
        }

        Value::Value(FunctionDef* o, FunctionDef* imm) : ITypedObject(imm->getOutput() ? imm->getOutput()->getSignature() : nullptr) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 1;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
            m_imm.fn = imm;
        }

        Value::Value(FunctionDef* o, DataType* imm, bool unused) : ITypedObject(imm) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 1;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
        }
        
        Value::Value(FunctionDef* o, Module* imm) : ITypedObject(nullptr) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 1;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 0;
            m_flags.is_immediate = 1;
            m_slotId = 0;
            m_imm.mod = imm;
        }
        
        Value::Value(FunctionDef* o, Module* imm, u32 slotId) : ITypedObject(imm->getData()[slotId].type) {
            m_func = o;
            m_src = o->getCompiler()->getCurrentSrc();
            m_regId = 0;
            m_allocId = 0;
            m_srcPtr = nullptr;
            m_srcSetter = nullptr;
            m_srcSelf = nullptr;
            m_flags.is_argument = 0;
            m_flags.is_read_only = 0;
            m_flags.is_pointer = 0;
            m_flags.is_type = 0;
            m_flags.is_module = 0;
            m_flags.is_function = 0;
            m_flags.is_function_id = 0;
            m_flags.is_module_data = 1;
            m_flags.is_immediate = 1;
            m_slotId = slotId;
            m_imm.mod = imm;
        }

        Value::~Value() {
            if (m_srcPtr) delete m_srcPtr;
            m_srcPtr = nullptr;

            if (m_srcSelf) delete m_srcSelf;
            m_srcSelf = nullptr;
        }
        
        // Discards any info about the current value and adopts the info from another
        void Value::reset(const Value& o) {
            m_func = o.m_func;
            m_name = o.m_name;
            m_src = o.m_src;
            m_type = o.m_type;
            m_regId = o.m_regId;
            m_allocId = o.m_allocId;
            m_srcPtr = o.m_srcPtr ? new Value(*o.m_srcPtr) : nullptr;
            m_srcSetter = o.m_srcSetter;
            m_srcSelf = o.m_srcSelf ? new Value(*o.m_srcSelf) : nullptr;
            m_flags = o.m_flags;
            m_slotId = o.m_slotId;
            m_imm = o.m_imm;
        }

        Value Value::convertedTo(DataType* tp) const {
            if (m_flags.is_module) {
                m_func->getCompiler()->valueError(*this, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_type) {
                m_func->getCompiler()->valueError(*this, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_module_data) {
                m_func->getCompiler()->valueError(*this, cm_err_module_data_used_as_value, "Module data refs cannot be used as a value");
                return m_func->getPoison();
            }

            if (tp->isEqualTo(m_type)) {
                return *this;
            }

            DataType* voidp = m_func->getContext()->getTypes()->getVoidPtr();
            if (m_type->isEqualTo(voidp) || tp->isEqualTo(voidp)) {
                // This is necessary to enable trusted scripts to work with
                // _real_ pointers. Writers of trusted scripts should know
                // what they are doing when working with pointers / casting
                Value ret = Value(*this);
                ret.m_type = tp;
                ret.m_flags.is_pointer = 1;
                return ret;
            }

            if (m_type->getInfo().is_primitive && tp->getInfo().is_primitive) {
                if (m_flags.is_pointer) {
                    Value ret = m_func->val(tp);
                    Value v = **this;
                    m_func->add(ir_cvt).op(ret).op(v).op(m_func->imm(tp->getId()));
                    return ret;
                } else if (m_flags.is_immediate) {
                    const auto& ai = m_type->getInfo();
                    const auto& bi = tp->getInfo();

                    if (ai.is_floating_point) {
                        if (bi.is_floating_point) return *this;
                        if (bi.is_unsigned) {
                            Value ret;

                            switch (bi.size) {
                                case sizeof(u64): ret.reset(Value(m_func, u64(m_imm.f)));
                                case sizeof(u32): ret.reset(Value(m_func, u64(u32(m_imm.f))));
                                case sizeof(u16): ret.reset(Value(m_func, u64(u16(m_imm.f))));
                                case sizeof(u8) : ret.reset(Value(m_func, u64(u8(m_imm.f))));
                                default: ret.reset(Value(m_func, u64(m_imm.f)));
                            }

                            ret.setType(tp);
                            return ret;
                        }

                        Value ret;

                        switch (bi.size) {
                            case sizeof(i64): ret.reset(Value(m_func, i64(m_imm.f)));
                            case sizeof(i32): ret.reset(Value(m_func, i64(i32(m_imm.f))));
                            case sizeof(i16): ret.reset(Value(m_func, i64(i16(m_imm.f))));
                            case sizeof(i8) : ret.reset(Value(m_func, i64(i8(m_imm.f))));
                            default: ret.reset(Value(m_func, i64(m_imm.f)));
                        }

                        ret.setType(tp);
                        return ret;
                    } else {
                        if (bi.is_floating_point) {
                            if (ai.is_unsigned) {
                                Value ret;

                                switch (bi.size) {
                                    case sizeof(f64): ret.reset(Value(m_func, f64(m_imm.u)));
                                    case sizeof(f32): ret.reset(Value(m_func, f64(f32(m_imm.u))));
                                    default: ret.reset(Value(m_func, f64(m_imm.u)));
                                }

                                ret.setType(tp);
                                return ret;
                            }

                            Value ret;

                            switch (bi.size) {
                                case sizeof(f64): ret.reset(Value(m_func, f64(m_imm.i)));
                                case sizeof(f32): ret.reset(Value(m_func, f64(f32(m_imm.i))));
                                default: ret.reset(Value(m_func, f64(m_imm.i)));
                            }

                            ret.setType(tp);
                            return ret;
                        } else {
                            if (ai.is_unsigned) {
                                if (bi.is_unsigned) {
                                    Value ret;

                                    switch (bi.size) {
                                        case sizeof(u64): ret.reset(Value(m_func, u64(m_imm.u)));
                                        case sizeof(u32): ret.reset(Value(m_func, u64(u32(m_imm.u))));
                                        case sizeof(u16): ret.reset(Value(m_func, u64(u16(m_imm.u))));
                                        case sizeof(u8) : ret.reset(Value(m_func, u64(u8(m_imm.u))));
                                        default: ret.reset(Value(m_func, u64(m_imm.u)));
                                    }

                                    ret.setType(tp);
                                    return ret;
                                }

                                Value ret;

                                switch (bi.size) {
                                    case sizeof(i64): ret.reset(Value(m_func, i64(m_imm.u)));
                                    case sizeof(i32): ret.reset(Value(m_func, i64(i32(m_imm.u))));
                                    case sizeof(i16): ret.reset(Value(m_func, i64(i16(m_imm.u))));
                                    case sizeof(i8) : ret.reset(Value(m_func, i64(i8(m_imm.u))));
                                    default: ret.reset(Value(m_func, i64(m_imm.u)));
                                }

                                ret.setType(tp);
                                return ret;
                            }

                            if (bi.is_unsigned) {
                                Value ret;

                                switch (bi.size) {
                                    case sizeof(u64): ret.reset(Value(m_func, u64(m_imm.i)));
                                    case sizeof(u32): ret.reset(Value(m_func, u64(u32(m_imm.i))));
                                    case sizeof(u16): ret.reset(Value(m_func, u64(u16(m_imm.i))));
                                    case sizeof(u8) : ret.reset(Value(m_func, u64(u8(m_imm.i))));
                                    default: ret.reset(Value(m_func, u64(m_imm.i)));
                                }

                                ret.setType(tp);
                                return ret;
                            }

                            Value ret;

                            switch (bi.size) {
                                case sizeof(i64): ret.reset(Value(m_func, i64(m_imm.i)));
                                case sizeof(i32): ret.reset(Value(m_func, i64(i32(m_imm.i))));
                                case sizeof(i16): ret.reset(Value(m_func, i64(i16(m_imm.i))));
                                case sizeof(i8) : ret.reset(Value(m_func, i64(i8(m_imm.i))));
                                default: ret.reset(Value(m_func, i64(m_imm.i)));
                            }

                            ret.setType(tp);
                            return ret;
                        }
                    }
                }

                Value ret = m_func->val(tp);
                m_func->add(ir_cvt).op(ret).op(*this).op(m_func->imm(tp->getId()));
                return ret;
            } else {
                // search for cast operators
                {
                    String name = String::Format("operator %s", tp->getFullyQualifiedName().c_str());
                    DataType* curClass = m_func->getCompiler()->currentClass();
                    const auto& methods = m_type->findMethods(
                        name,
                        tp,
                        nullptr,
                        0,
                        fm_ignore_args | fm_strict_return | ((!curClass || !curClass->isEqualTo(m_type)) ? fm_exclude_private : 0)
                    );

                    if (methods.size() == 1) {
                        if (!m_flags.is_type && !methods[0]->isThisCall()) {
                            m_func->getCompiler()->valueError(
                                *this,
                                cm_err_method_is_static,
                                "Method '%s' of type '%s' is static",
                                methods[0]->getDisplayName().c_str(),
                                m_type->getName().c_str()
                            );
                            return m_func->getPoison();
                        }

                        return m_func->getCompiler()->generateCall(methods[0], {}, this);
                    } else if (methods.size() > 1) {
                        m_func->getCompiler()->valueError(
                            *this,
                            cm_err_property_or_method_ambiguous,
                            "Cast operator '%s' of type '%s' is ambiguous",
                            name.c_str(),
                            m_type->getName().c_str()
                        );

                        for (u32 i = 0;i < methods.size();i++) {
                            m_func->getCompiler()->info(
                                cm_info_could_be,
                                "^ Could be '%s'",
                                methods[i]->getDisplayName().c_str()
                            ).src = methods[i]->getSource();
                        }

                        return m_func->getPoison();
                    }
                }
            
                // search for copy constructor
                {
                    String name = "constructor";
                    DataType* curClass = m_func->getCompiler()->currentClass();
                    const auto& methods = tp->findMethods(
                        "constructor",
                        nullptr,
                        const_cast<const DataType**>(&m_type),
                        1,
                        fm_skip_implicit_args | fm_strict_args | ((!curClass || !curClass->isEqualTo(m_type)) ? fm_exclude_private : 0)
                    );

                    if (methods.size() == 1) {
                        return m_func->getCompiler()->constructObject(tp, { *this });
                    } else if (methods.size() > 1) {
                        m_func->getCompiler()->valueError(
                            *this,
                            cm_err_ambiguous_constructor,
                            "Construction of type '%s' with arguments '%s' is ambiguous",
                            tp->getName().c_str(),
                            argListStr({ m_type }).c_str()
                        );

                        for (u32 i = 0;i < methods.size();i++) {
                            m_func->getCompiler()->info(
                                cm_info_could_be,
                                "^ Could be '%s'",
                                methods[i]->getDisplayName().c_str()
                            ).src = methods[i]->getSource();
                        }

                        return m_func->getPoison();
                    }
                }
            }

            m_func->getCompiler()->valueError(
                *this,
                cm_err_type_not_convertible,
                "No conversion from type '%s' to '%s' is available",
                m_type->getName().c_str(),
                tp->getName().c_str()
            );

            return m_func->getPoison();
        }

        Value Value::getProp(
            const String& name,
            bool excludeInherited,
            bool excludePrivate,
            bool excludeMethods,
            bool doError,
            member_expr_hints* hints
        ) {
            if (!excludeMethods) {
                function_match_flags flags = 0;
                Array<DataType*> argTps;

                if (excludePrivate) flags |= fm_exclude_private;
                if (hints && hints->for_func_call) {
                    flags |= fm_skip_implicit_args;
                    argTps = hints->args.map([](const Value& v) { return v.getType(); });
                } else flags |= fm_ignore_args;

                if (m_flags.is_module) {
                    auto funcs = m_imm.mod->findFunctions(
                        name,
                        nullptr,
                        const_cast<const DataType **>(argTps.data()),
                        argTps.size(),
                        flags
                    );

                    if (funcs.size() == 1) {
                        return m_func->getCompiler()->functionValue(funcs[0]);
                    } else if (funcs.size() > 1) {
                        if (doError) {
                            m_func->getCompiler()->valueError(
                                *this,
                                cm_err_export_ambiguous,
                                "Reference to export '%s' of module '%s' is ambiguous",
                                name.c_str(),
                                m_name.c_str()
                            );

                            for (u32 i = 0;i < funcs.size();i++) {
                                m_func->getCompiler()->info(
                                    cm_info_could_be,
                                    "^ Could be '%s'",
                                    funcs[i]->getDisplayName().c_str()
                                ).src = funcs[i]->getSource();
                            }
                        }

                        return m_func->getPoison();
                    }
                } else {
                    auto methods = m_type->findMethods(
                        name,
                        nullptr,
                        const_cast<const DataType**>(argTps.data()),
                        argTps.size(),
                        flags
                    );

                    if (methods.size() == 1) {
                        if (m_flags.is_type && methods[0]->isThisCall()) {
                            m_func->getCompiler()->valueError(
                                *this,
                                cm_err_method_not_static,
                                "Method '%s' of type '%s' is not static",
                                methods[0]->getDisplayName().c_str(),
                                m_type->getName().c_str()
                            );
                            return m_func->getPoison();
                        } else if (!m_flags.is_type && !methods[0]->isThisCall()) {
                            m_func->getCompiler()->valueError(
                                *this,
                                cm_err_method_is_static,
                                "Method '%s' of type '%s' is static",
                                methods[0]->getDisplayName().c_str(),
                                m_type->getName().c_str()
                            );
                            return m_func->getPoison();
                        }

                        Value ret = m_func->getCompiler()->functionValue(methods[0]);
                        if (!m_flags.is_type) ret.m_srcSelf = new Value(*this);
                        return ret;
                    } else if (methods.size() > 1) {
                        if (doError) {
                            m_func->getCompiler()->valueError(
                                *this,
                                cm_err_property_or_method_ambiguous,
                                "Reference to property or method '%s' of type '%s' is ambiguous",
                                name.c_str(),
                                m_type->getName().c_str()
                            );

                            for (u32 i = 0;i < methods.size();i++) {
                                m_func->getCompiler()->info(
                                    cm_info_could_be,
                                    "^ Could be '%s'",
                                    methods[i]->getDisplayName().c_str()
                                ).src = methods[i]->getSource();
                            }
                        }

                        return m_func->getPoison();
                    }
                }
            }

            if (m_flags.is_module) {
                i64 slotId = m_imm.mod->getData().findIndex([&name](const module_data& d) { return d.name == name; });
                if (slotId >= 0) {
                    return m_func->getCompiler()->moduleData(m_imm.mod, (u32)slotId);
                }

                m_func->getCompiler()->error(
                    cm_err_export_not_found,
                    "Module '%s' has no export named '%s'",
                    m_name.c_str(),
                    name.c_str()
                );
                
                return m_func->getPoison();
            }

            const type_property* prop = m_type->getProp(name, excludeInherited, excludePrivate);
            if (!prop) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_not_found,
                        "Type '%s' has no property or method named '%s'",
                        m_type->getName().c_str(),
                        name.c_str()
                    );
                }

                return m_func->getPoison();
            }

            DataType* curClass = m_func->getCompiler()->currentClass();
            if (prop->access == private_access && (!curClass || !curClass->isEqualTo(m_type)) && excludePrivate) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_is_private,
                        "Property '%s' of type '%s' is private",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            } else if (prop->access == trusted_access && !m_func->getCompiler()->isTrusted()) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_not_trusted,
                        "Property '%s' of type '%s' is only accessible to trusted scripts",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            }

            if (m_flags.is_type && !prop->flags.is_static) {
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_property_not_static,
                    "Property '%s' of type '%s' is not static",
                    name.c_str(),
                    m_type->getName().c_str()
                );
                return m_func->getPoison();
            } else if (!m_flags.is_type && prop->flags.is_static) {
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_property_is_static,
                    "Property '%s' of type '%s' is static",
                    name.c_str(),
                    m_type->getName().c_str()
                );
                return m_func->getPoison();
            }

            if (!prop->flags.can_read) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_no_read_access,
                        "Property '%s' of '%s' does not give read access",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            }

            if (prop->getter) {
                Value out = m_func->getCompiler()->generateCall(prop->getter, {}, this);
                out.m_flags.is_read_only = (prop->flags.can_write && prop->setter) ? 0 : 1;
                if (prop->setter) {
                    out.m_srcSetter = m_func->getCompiler()->getOutput()->getFunctionDef(prop->setter);
                }
                out.m_srcSelf = new Value(*this);
                return out;
            }

            Value ptr = m_func->val(prop->type);
            ptr.m_flags.is_pointer = 1;
            m_func->add(ir_uadd).op(ptr).op(*this).op(m_func->imm(prop->offset));

            if (!prop->flags.is_pointer && !prop->type->getInfo().is_primitive) {
                // object instantiation at ptr
                return ptr;
            }
            
            // load primitive or pointer from ptr
            Value out = m_func->val(prop->type);
            out.m_flags.is_read_only = prop->flags.can_write ? 0 : 1;
            out.m_flags.is_pointer = (prop->flags.is_pointer || prop->type->isEqualTo(m_func->getContext()->getTypes()->getVoidPtr())) ? 1 : 0;

            out.m_srcPtr = new Value(ptr);
            out.m_srcSelf = new Value(*this);

            m_func->add(ir_load).op(out).op(ptr);
            return out;
        }

        Value Value::getPropPtr(
            const String& name,
            bool excludeInherited,
            bool excludePrivate,
            bool doError
        ) {
            const type_property* prop = m_type->getProp(name, excludeInherited, excludePrivate);
            if (!prop) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_not_found,
                        "Type '%s' has no property or method named '%s'",
                        m_type->getName().c_str(),
                        name.c_str()
                    );
                }

                return m_func->getPoison();
            }

            DataType* curClass = m_func->getCompiler()->currentClass();
            if (prop->access == private_access && (!curClass || !curClass->isEqualTo(m_type)) && excludePrivate) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_is_private,
                        "Property '%s' of type '%s' is private",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            } else if (prop->access == trusted_access && !m_func->getCompiler()->isTrusted()) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_not_trusted,
                        "Property '%s' of type '%s' is only accessible to trusted scripts",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            }

            if (m_flags.is_type && !prop->flags.is_static) {
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_property_not_static,
                    "Property '%s' of type '%s' is not static",
                    name.c_str(),
                    m_type->getName().c_str()
                );
                return m_func->getPoison();
            } else if (!m_flags.is_type && prop->flags.is_static) {
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_property_is_static,
                    "Property '%s' of type '%s' is static",
                    name.c_str(),
                    m_type->getName().c_str()
                );
                return m_func->getPoison();
            }

            if (!prop->flags.can_read) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_property_no_read_access,
                        "Property '%s' of '%s' does not give read access",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            }

            if (prop->getter) {
                if (doError) {
                    m_func->getCompiler()->valueError(
                        *this,
                        cm_err_no_pointer_to_getter,
                        "Cannot get pointer to property accessor '%s' of '%s'",
                        name.c_str(),
                        m_type->getName().c_str()
                    );
                }
                return m_func->getPoison();
            }

            Value ptr = m_func->val(prop->type);
            ptr.m_flags.is_pointer = 1;
            m_func->add(ir_uadd).op(ptr).op(*this).op(m_func->imm(prop->offset));

            if (prop->flags.is_pointer) {
                // Load pointer to object or primitive
                m_func->add(ir_load).op(ptr).op(ptr);
            }

            return ptr;
        }

        vreg_id Value::getRegId() const {
            return m_regId;
        }

        alloc_id Value::getStackAllocId() const {
            return m_allocId;
        }

        u32 Value::getModuleDataSlotId() const {
            return m_slotId;
        }

        const String& Value::getName() const {
            return m_name;
        }

        const SourceLocation& Value::getSource() const {
            return m_src;
        }

        const Value::flags& Value::getFlags() const {
            return m_flags;
        }

        Value::flags& Value::getFlags() {
            return m_flags;
        }

        Value* Value::getSrcPtr() const {
            return m_srcPtr;
        }
        
        Value* Value::getSrcSelf() const {
            return m_srcSelf;
        }

        void Value::setType(ffi::DataType* to) {
            m_type = to;
        }

        void Value::setRegId(vreg_id reg) {
            m_regId = reg;
            m_allocId = 0;
        }

        void Value::setStackAllocId(alloc_id id) {
            m_allocId = id;
            m_regId = 0;
        }
        
        void Value::setStackSrc(const Value& src) {
            if (m_srcSelf) m_srcSelf->reset(src);
            else m_srcSelf = new Value(src);
        }

        bool Value::isValid() const {
            return m_regId > 0 || m_allocId > 0 || m_flags.is_module_data ||
                   m_flags.is_module || m_flags.is_function || m_flags.is_type ||
                   m_flags.is_immediate || m_flags.is_argument;
        }
        
        bool Value::isArg() const {
            return m_flags.is_argument == 1;
        }

        bool Value::isFloatingPoint() const {
            if (!m_type) return false;
            if (!m_type->getInfo().is_floating_point) return false;

            if (m_flags.is_pointer) return false;
            if (m_flags.is_module_data) return false;
            if (m_allocId > 0) return false;

            return true;
        }

        bool Value::isReg() const {
            return m_regId > 0;
        }

        bool Value::isImm() const {
            return m_flags.is_immediate;
        }

        bool Value::isStack() const {
            return m_allocId > 0;
        }

        bool Value::isModuleData() const {
            return m_flags.is_module_data;
        }

        bool Value::isType() const {
            return m_flags.is_type;
        }

        bool Value::isModule() const {
            return m_flags.is_module;
        }

        bool Value::isFunction() const {
            return m_flags.is_function;
        }

        bool Value::isFunctionID() const {
            return m_flags.is_function && m_flags.is_function_id;
        }

        Value Value::genBinaryOp(
            FunctionDef* fn,
            const Value* self,
            const Value& rhs,
            ir_instruction _i,
            ir_instruction _u,
            ir_instruction _f,
            ir_instruction _d,
            const char* overrideName,
            bool assignmentOp
        ) const {
            if (self->m_flags.is_module) {
                m_func->getCompiler()->valueError(*self, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (self->m_flags.is_type) {
                m_func->getCompiler()->valueError(*self, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            } else if (self->m_flags.is_module_data) {
                m_func->getCompiler()->valueError(*self, cm_err_module_data_used_as_value, "Module data refs cannot be used as a value");
                return m_func->getPoison();
            }

            if (self->m_flags.is_read_only && assignmentOp) {
                m_func->getCompiler()->valueError(*self, cm_err_value_not_writable, "Cannot write to read-only value");
                return m_func->getPoison();
            }

            DataType* selfTp = self->getType();
            bool isVoidPtr = selfTp->isEqualTo(m_func->getContext()->getTypes()->getVoidPtr());
            const auto& i = selfTp->getInfo();

            if (i.is_primitive) {
                Value out;

                ir_instruction inst = ir_noop;
                if (i.is_integral) {
                    if (i.is_unsigned) inst = _u;
                    else inst = _i;
                } else if (i.is_floating_point) {
                    if (i.size == sizeof(f64)) inst = _d;
                    else inst = _f;
                }

                Value _rhs = rhs.convertedTo(selfTp);
                if (self->m_flags.is_pointer) {
                    if (inst == ir_assign) {
                        // Assignment will happen via ir_store instruction a few blocks below here
                        out.reset(_rhs);
                    } else {
                        out.reset(fn->val(selfTp));
                        Value val;
                        if (isVoidPtr) {
                            // void* is treated differently, essentially like a u64
                            val.reset(*self);
                        } else {
                            // all other primitives should be loaded
                            val.reset(**self);
                        }
                        fn->add(inst).op(out).op(val).op(_rhs);
                    }
                } else {
                    if (inst == ir_assign) {
                        fn->add(inst).op(*self).op(_rhs);
                        out.reset(*self);
                    } else {
                        out.reset(fn->val(selfTp));
                        fn->add(inst).op(out).op(*self).op(_rhs);
                    }
                }

                if (assignmentOp) {
                    if (self->m_flags.is_pointer && !isVoidPtr) fn->add(ir_store).op(out).op(*self);
                    else if (self->m_srcPtr) fn->add(ir_store).op(out).op(*self->m_srcPtr);
                    else if (self->m_srcSetter) m_func->getCompiler()->generateCall(self->m_srcSetter, { out }, self->m_srcSelf);
                }

                return out;
            } else {
                const DataType* rtp = rhs.getType();
                Array<Function*> matches = selfTp->findMethods(overrideName, nullptr, &rtp, 1, fm_skip_implicit_args);
                if (matches.size() == 1) {
                    return fn->getCompiler()->generateCall(matches[0], { rhs }, self);
                } else if (matches.size() > 1) {
                    u8 strictC = 0;
                    Function* func = nullptr;
                    for (u32 i = 0;i < matches.size();i++) {
                        const auto& args = matches[i]->getSignature()->getArguments();
                        for (u32 a = 0;a < args.size();a++) {
                            if (args[a].isImplicit()) continue;
                            if (args[a].dataType->isEqualTo(rtp)) {
                                if (args.size() > a + 1) {
                                    // Function has more arguments which are not explicitly required
                                    break;
                                }

                                strictC++;
                                func = matches[i];
                            }
                        }
                    }
                    
                    if (strictC == 1) {
                        return fn->getCompiler()->generateCall(func, { rhs }, self);
                    }

                    fn->getCompiler()->functionError(
                        selfTp,
                        (DataType*)nullptr,
                        { rtp },
                        cm_err_method_ambiguous,
                        "Reference to method '%s' of type '%s' with arguments '%s' is ambiguous",
                        overrideName,
                        selfTp->getName().c_str(),
                        argListStr({ rtp }).c_str()
                    );

                    for (u32 i = 0;i < matches.size();i++) {
                        fn->getCompiler()->info(
                            cm_info_could_be,
                            "^ Could be: '%s'",
                            matches[i]->getDisplayName().c_str()
                        ).src = matches[i]->getSource();
                    }
                } else {
                    fn->getCompiler()->functionError(
                        selfTp,
                        (DataType*)nullptr,
                        { rtp },
                        cm_err_method_not_found,
                        "Type '%s' has no method named '%s' with arguments matching '%s'",
                        selfTp->getName().c_str(),
                        overrideName,
                        argListStr({ rtp }).c_str()
                    );
                }
            }

            return fn->getPoison();
        }

        Value Value::genUnaryOp(
            FunctionDef* fn,
            const Value* self,
            ir_instruction _i,
            ir_instruction _u,
            ir_instruction _f,
            ir_instruction _d,
            const char* overrideName,
            bool resultIsPreOp,
            bool assignmentOp,
            bool noResultReg
        ) const {
            if (self->m_flags.is_module) {
                m_func->getCompiler()->valueError(*self, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (self->m_flags.is_type) {
                m_func->getCompiler()->valueError(*self, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            }

            if (self->m_flags.is_read_only && assignmentOp) {
                m_func->getCompiler()->valueError(*self, cm_err_value_not_writable, "Cannot write to read-only value");
                return m_func->getPoison();
            }

            DataType* selfTp = self->getType();
            const auto& i = selfTp->getInfo();
            if (i.is_primitive) {
                Value out = fn->val(selfTp);

                ir_instruction inst = ir_noop;
                if (i.is_integral) {
                    if (i.is_unsigned) inst = _u;
                    else inst = _i;
                } else if (i.is_floating_point) {
                    if (i.size == sizeof(f64)) inst = _d;
                    else inst = _f;
                }

                Value v = self->m_flags.is_pointer ? **self : *self;

                // todo: Revise this... Something isn't right. out is assigned 2 times
                if (resultIsPreOp) fn->add(ir_assign).op(out).op(v);

                if (noResultReg) fn->add(inst).op(v);
                else fn->add(inst).op(out).op(v);

                if (!resultIsPreOp) fn->add(ir_assign).op(out).op(v);

                if (assignmentOp) {
                    if (self->m_flags.is_pointer) fn->add(ir_store).op(v).op(*self);
                    else if (self->m_srcPtr) fn->add(ir_store).op(v).op(*self->m_srcPtr);
                    else if (self->m_srcSetter) m_func->getCompiler()->generateCall(self->m_srcSetter, { v }, self->m_srcSelf);
                }

                return out;
            } else {
                Array<Function*> matches = selfTp->findMethods(overrideName, nullptr, nullptr, 0, fm_skip_implicit_args);
                if (matches.size() == 1) {
                    return fn->getCompiler()->generateCall(matches[0], { }, self);
                } else if (matches.size() > 1) {u8 strictC = 0;
                    fn->getCompiler()->functionError(
                        selfTp,
                        (DataType*)nullptr,
                        Array<Value>(),
                        cm_err_method_ambiguous,
                        "Reference to method '%s' of type '%s' with no arguments is ambiguous",
                        overrideName,
                        selfTp->getName().c_str()
                    );

                    for (u32 i = 0;i < matches.size();i++) {
                        fn->getCompiler()->info(
                            cm_info_could_be,
                            "^ Could be: '%s'",
                            matches[i]->getDisplayName().c_str()
                        ).src = matches[i]->getSource();
                    }
                } else {
                    fn->getCompiler()->functionError(
                        selfTp,
                        (DataType*)nullptr,
                        Array<Value>(),
                        cm_err_method_not_found,
                        "Type '%s' has no method named '%s' with no arguments",
                        selfTp->getName().c_str(),
                        overrideName
                    );
                }
            }

            return fn->getPoison();
        }

        Value Value::operator +  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_iadd, ir_uadd, ir_fadd, ir_dadd, "operator +");
        }

        Value Value::operator += (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_iadd, ir_uadd, ir_fadd, ir_dadd, "operator +=", true);
        }

        Value Value::operator -  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_isub, ir_usub, ir_fsub, ir_dsub, "operator -");
        }

        Value Value::operator -= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_isub, ir_usub, ir_fsub, ir_dsub, "operator -=", true);
        }

        Value Value::operator *  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_imul, ir_umul, ir_fmul, ir_dmul, "operator *");
        }

        Value Value::operator *= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_imul, ir_umul, ir_fmul, ir_dmul, "operator *=", true);
        }

        Value Value::operator /  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_idiv, ir_udiv, ir_fdiv, ir_ddiv, "operator /");
        }

        Value Value::operator /= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_idiv, ir_udiv, ir_fdiv, ir_ddiv, "operator /=", true);
        }

        Value Value::operator %  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_imod, ir_umod, ir_fmod, ir_dmod, "operator %");
        }

        Value Value::operator %= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_imod, ir_umod, ir_fmod, ir_dmod, "operator %=", true);
        }

        Value Value::operator ^  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_xor, ir_xor, ir_noop, ir_noop, "operator ^");
        }

        Value Value::operator ^= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_xor, ir_xor, ir_noop, ir_noop, "operator ^=", true);
        }

        Value Value::operator &  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_band, ir_band, ir_noop, ir_noop, "operator &");
        }

        Value Value::operator &= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_band, ir_band, ir_noop, ir_noop, "operator &=", true);
        }

        Value Value::operator |  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_bor, ir_bor, ir_noop, ir_noop, "operator |");
        }

        Value Value::operator |= (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_bor, ir_bor, ir_noop, ir_noop, "operator |=", true);
        }

        Value Value::operator << (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_shl, ir_shl, ir_noop, ir_noop, "operator <<");
        }

        Value Value::operator <<=(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_shl, ir_shl, ir_noop, ir_noop, "operator <<=", true);
        }

        Value Value::operator >> (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_shr, ir_shr, ir_noop, ir_noop, "operator >>");
        }

        Value Value::operator >>=(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_shr, ir_shr, ir_noop, ir_noop, "operator >>=", true);
        }

        Value Value::operator != (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ineq, ir_uneq, ir_fneq, ir_dneq, "operator !=");
        }

        Value Value::operator && (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_land, ir_land, ir_land, ir_land, "operator &&");
        }

        Value Value::operator || (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_lor, ir_lor, ir_lor, ir_lor, "operator ||");
        }

        Value Value::operator =  (const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_assign, ir_assign, ir_assign, ir_assign, "operator =", true);
        }

        Value Value::operator == (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ieq, ir_ueq, ir_feq, ir_deq, "operator ==");
        }

        Value Value::operator <  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ilt, ir_ult, ir_flt, ir_dlt, "operator <");
        }

        Value Value::operator <= (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_ilte, ir_ulte, ir_flte, ir_dlte, "operator <=");
        }

        Value Value::operator >  (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_igt, ir_ugt, ir_fgt, ir_dgt, "operator >");
        }

        Value Value::operator >= (const Value& rhs) const {
            return genBinaryOp(m_func, this, rhs, ir_igte, ir_ugte, ir_fgte, ir_dgte, "operator >=");
        }

        Value Value::operator [] (const Value& _rhs) const {
            if (m_flags.is_module) {
                m_func->getCompiler()->valueError(*this, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_type) {
                m_func->getCompiler()->valueError(*this, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_module_data) {
                m_func->getCompiler()->valueError(*this, cm_err_module_data_used_as_value, "Module data refs cannot be used as a value");
                return m_func->getPoison();
            }

            // todo: If script is trusted then allow offsetting from
            //       pointers. Otherwise error if that behavior is
            //       attempted.

            Value rhs = _rhs.m_flags.is_pointer && _rhs.m_type->getInfo().is_primitive ? *_rhs : _rhs;

            const DataType* rtp = rhs.getType();
            Array<Function*> matches = m_type->findMethods("operator []", nullptr, &rtp, 1, fm_skip_implicit_args);
            if (matches.size() == 1) {
                return m_func->getCompiler()->generateCall(matches[0], { rhs }, this);
            } else if (matches.size() > 1) {
                u8 strictC = 0;
                Function* func = nullptr;
                for (u32 i = 0;i < matches.size();i++) {
                    const auto& args = matches[i]->getSignature()->getArguments();
                    for (u32 a = 0;a < args.size();a++) {
                        if (args[a].isImplicit()) continue;
                        if (args[a].dataType == rtp) {
                            if (args.size() > a + 1) {
                                // Function has more arguments which are not explicitly required
                                break;
                            }

                            strictC++;
                            func = matches[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    return m_func->getCompiler()->generateCall(func, { rhs }, this);
                }

                m_func->getCompiler()->functionError(
                    m_type,
                    nullptr,
                    { _rhs },
                    cm_err_method_ambiguous,
                    "Reference to method 'operator []' of type '%s' with arguments '%s' is ambiguous",
                    m_type->getName().c_str(),
                    argListStr({ _rhs }).c_str()
                );

                for (u32 i = 0;i < matches.size();i++) {
                    m_func->getCompiler()->info(
                        cm_info_could_be,
                        "^ Could be: '%s'",
                        matches[i]->getDisplayName().c_str()
                    ).src = matches[i]->getSource();
                }
            } else {
                m_func->getCompiler()->functionError(
                    m_type,
                    nullptr,
                    { _rhs },
                    cm_err_method_not_found,
                    "Type '%s' has no method named 'operator []' with arguments matching '%s'",
                    m_type->getName().c_str(),
                    argListStr({ _rhs }).c_str()
                );
            }

            return m_func->getPoison();
        }

        Value Value::operator () (const Array<Value>& _args, Value* self) const {
            if (m_flags.is_module) {
                m_func->getCompiler()->valueError(*this, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_type) {
                m_func->getCompiler()->valueError(*this, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_module_data) {
                m_func->getCompiler()->valueError(*this, cm_err_module_data_used_as_value, "Module data refs cannot be used as a value");
                return m_func->getPoison();
            }

            Array<Value> args;
            Array<DataType*> argTps;

            _args.each([&args, &argTps](const Value& v) {
                args.push(v.m_flags.is_pointer && v.m_type->getInfo().is_primitive ? *v : v);
                argTps.push(v.m_type);
            });

            if (m_flags.is_function) {
                if (m_flags.is_immediate) {
                    return m_func->getCompiler()->generateCall(m_imm.fn, _args, self);
                }

                return m_func->getCompiler()->generateCall(*this, _args, self);
            }

            Array<Function*> matches = m_type->findMethods(
                "operator ()",
                nullptr,
                const_cast<const DataType**>(argTps.data()),
                argTps.size(),
                fm_skip_implicit_args
            );

            if (matches.size() == 1) {
                return m_func->getCompiler()->generateCall(matches[0], args, this);
            } else if (matches.size() > 1) {
                u8 strictC = 0;
                Function* func = nullptr;
                for (u32 i = 0;i < matches.size();i++) {
                    const auto& margs = matches[i]->getSignature()->getArguments();
                    u32 nonImplicitCount = 0;
                    for (u32 a = 0;a < margs.size();a++) {
                        if (margs[a].isImplicit() || !margs[a].dataType->isEqualTo(argTps[nonImplicitCount])) continue;
                        nonImplicitCount++;
                        if (nonImplicitCount == args.size() && margs.size() == a + 1) {
                            strictC++;
                            func = matches[i];
                        }
                    }
                }
                
                if (strictC == 1) {
                    return m_func->getCompiler()->generateCall(func, args, this);
                }

                m_func->getCompiler()->functionError(
                    m_type,
                    nullptr,
                    _args,
                    cm_err_method_ambiguous,
                    "Reference to method 'operator ()' of type '%s' with arguments '%s' is ambiguous",
                    m_type->getName().c_str(),
                    argListStr(_args).c_str()
                );

                for (u32 i = 0;i < matches.size();i++) {
                    m_func->getCompiler()->info(
                        cm_info_could_be,
                        "^ Could be: '%s'",
                        matches[i]->getDisplayName().c_str()
                    ).src = matches[i]->getSource();
                }
            } else {
                m_func->getCompiler()->functionError(
                    m_type,
                    nullptr,
                    _args,
                    cm_err_method_not_found,
                    "Type '%s' has no method named 'operator ()' with arguments matching '%s'",
                    m_type->getName().c_str(),
                    argListStr(_args).c_str()
                );
            }

            return m_func->getPoison();
        }

        Value Value::operator -  () {
            return genUnaryOp(m_func, this, ir_ineg, ir_noop, ir_fneg, ir_dneg, "operator -");
        }

        Value Value::operator -- () {
            return genUnaryOp(m_func, this, ir_idec, ir_udec, ir_fdec, ir_ddec, "operator --", false, true, true);
        }

        Value Value::operator -- (int) {
            return genUnaryOp(m_func, this, ir_idec, ir_udec, ir_fdec, ir_ddec, "operator --", true, true, true);
        }

        Value Value::operator ++ () {
            return genUnaryOp(m_func, this, ir_iinc, ir_uinc, ir_finc, ir_dinc, "operator ++", false, true, true);
        }

        Value Value::operator ++ (int) {
            return genUnaryOp(m_func, this, ir_iinc, ir_uinc, ir_finc, ir_dinc, "operator ++", true, true, true);
        }

        Value Value::operator !  () const {
            return genUnaryOp(m_func, this, ir_not, ir_not, ir_not, ir_not, "operator !");
        }

        Value Value::operator ~  () const {
            return genUnaryOp(m_func, this, ir_inv, ir_inv, ir_noop, ir_noop, "operator ~");
        }

        Value Value::operator *  () const {
            if (m_flags.is_module) {
                m_func->getCompiler()->valueError(*this, cm_err_module_used_as_value, "Modules cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_type) {
                m_func->getCompiler()->valueError(*this, cm_err_type_used_as_value, "Types cannot be used as a value");
                return m_func->getPoison();
            } else if (m_flags.is_module_data) {
                m_func->getCompiler()->valueError(*this, cm_err_module_data_used_as_value, "Module data refs cannot be used as a value");
                return m_func->getPoison();
            }
            
            if (!m_flags.is_pointer && !m_type->isEqualTo(m_func->getContext()->getTypes()->getVoidPtr())) {
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_internal,
                    "Attempt to dereference value of non-pointer type"
                );
                return m_func->getPoison();
            }

            if (!m_type->getInfo().is_primitive) {
                // Can't dereference pointer to object because objects can't be stored in registers
                m_func->getCompiler()->valueError(
                    *this,
                    cm_err_internal,
                    "Attempt to dereference value of non-primitive type"
                );
                return m_func->getPoison();
            }

            Value v = m_func->val(m_type);
            v.m_srcPtr = new Value(*this);
            v.m_flags.is_read_only = m_flags.is_read_only;
            m_func->add(ir_load).op(v).op(*this);
            return v;
        }

        Value Value::operator_logicalAndAssign(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_land, ir_land, ir_land, ir_land, "operator &&=", true);
        }

        Value Value::operator_logicalOrAssign(const Value& rhs) {
            return genBinaryOp(m_func, this, rhs, ir_lor, ir_lor, ir_lor, ir_lor, "operator ||=", true);
        }

        bool Value::isEquivalentTo(const Value& v) const {
            if (isReg()) {
                if (!v.isReg()) return false;
                return m_regId == v.m_regId;
            } else if (isStack()) {
                if (!v.isStack()) return false;
                return m_allocId == v.m_allocId;
            } else if (isArg()) {
                if (!v.isArg()) return false;
                return m_imm.u == v.m_imm.u;
            } else if (m_flags.is_module) {
                if (!v.m_flags.is_module) return false;
                return m_imm.mod == v.m_imm.mod;
            } else if (m_flags.is_module_data) {
                if (!v.m_flags.is_module_data) return false;
                return m_imm.mod == v.m_imm.mod && m_slotId == v.m_slotId;
            } else if (isType()) {
                if (!v.isType()) return false;
                return m_type->isEqualTo(v.m_type);
            } else if (m_flags.is_function) {
                if (!v.m_flags.is_function) return false;
                return m_imm.fn == v.m_imm.fn;
            } else if (m_flags.is_immediate) {
                if (!v.m_flags.is_immediate) return false;
                ffi::DataType* atp = m_type;
                const type_meta& ainfo = atp->getInfo();
                ffi::DataType* btp = v.m_type;
                const type_meta& binfo = btp->getInfo();
                ffi::DataType* nullTp = m_func->getContext()->getTypes()->getNull();
                bool aIsNull = atp->isEqualTo(nullTp);
                if (aIsNull != btp->isEqualTo(nullTp)) return false;
                else if (aIsNull) return true;

                if (ainfo.is_floating_point) {
                    if (!binfo.is_floating_point || ainfo.size != binfo.size) return false;

                    if (m_imm.f != v.m_imm.f) return false;
                } else {
                    if (binfo.is_floating_point || ainfo.is_unsigned != binfo.is_unsigned) return false;

                    if (ainfo.is_unsigned) {
                        if (m_imm.u != v.m_imm.u) return false;
                    } else {
                        if (m_imm.i != v.m_imm.i) return false;
                    }
                }
            }

            return true;
        }

        String Value::toString(Context* ctx) const {
            if (!isValid()) return "<Invalid Value>";

            String s;
            if (m_flags.is_argument) {
                if (m_type->getInfo().is_floating_point) s = String::Format("$FPA%d", m_imm.u);
                else s = String::Format("$GPA%d", m_imm.u);
            } else if (m_flags.is_function) {
                if (m_flags.is_immediate) {
                    ffi::Function* fn = nullptr;
                    
                    if (m_flags.is_function_id) fn = ctx->getFunctions()->getFunction(getImm<function_id>());
                    else fn = m_imm.fn->getOutput();

                    if (fn) s = String::Format("<Function %s>", fn->getDisplayName().c_str());
                    else s = String::Format("<Function %s>", m_imm.fn->getName().c_str());
                } else {
                    s = String::Format("<Closure %s>", m_type->getName().c_str());
                }
            } else if (m_flags.is_module) {
                s = String::Format("<Module %s>", m_imm.mod->getName().c_str());
            } else if (m_flags.is_type) {
                s = String::Format("<Type %s>", m_type->getName().c_str());
            } else if (m_flags.is_module_data) {
                s = String::Format("<Module %s : %s>", m_imm.mod->getName().c_str(), m_imm.mod->getDataInfo(m_slotId).name.c_str());
            } else if (m_type->getInfo().is_primitive) {
                if (isReg()) {
                    if (m_type->getInfo().is_floating_point) s = String::Format("$FP%d", m_regId);
                    else s = String::Format("$GP%d", m_regId);
                } else if (isStack()) {
                    s = String::Format("$ST%d", m_allocId);
                } else if (isImm()) {
                    const auto& i = m_type->getInfo();
                    if (i.is_floating_point) s = String::Format("%f", m_imm.f);
                    else {
                        if (i.is_unsigned) s = String::Format("%llu", m_imm.u);
                        else s = String::Format("%lli", m_imm.i);
                    }
                }
            } else if (m_type->isEqualTo(ctx->getTypes()->getNull())) {
                s = "null";
            } else {
                if (isReg()) {
                    s = String::Format("$GP%d", m_regId);
                } else if (isStack()) {
                    s = String::Format("$ST%d", m_allocId);
                } else {
                    s = "<Invalid Value>";
                }
            }

            if (m_name.size() > 0) {
                s += "(" + m_name + ")";
            }

            return s;
        }
    };
};