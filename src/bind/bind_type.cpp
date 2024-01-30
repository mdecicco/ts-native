#include <tsn/bind/bind_type.h>
#include <tsn/bind/bind.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/common/Module.h>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        DataTypeBinder::DataTypeBinder(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, const utils::String& name, const utils::String& fullyQualifiedName, type_meta&& meta) {
            funcRegistry = freg;
            typeRegistry = treg;
            m_mod = mod;

            DataType* existing = treg->getTypeByHostHash(meta.host_hash);
            if (existing) {
                throw BindException(utils::String::Format(
                    "Attempted to bind new type '%s' which has already been bound (as '%s')",
                    name.c_str(), existing->getFullyQualifiedName().c_str()
                ));
            }

            m_type = new DataType(name, fullyQualifiedName, meta);
            m_type->m_id = (type_id)std::hash<utils::String>()(m_type->getFullyQualifiedName());
            typeRegistry->addHostType(meta.host_hash, m_type);

            if (mod) mod->addHostType(m_type->m_info.host_hash, m_type);
            else typeRegistry->getContext()->getGlobal()->addHostType(m_type->m_info.host_hash, m_type);
            m_isFinal = false;
        }

        DataTypeBinder::~DataTypeBinder() {
            if (!m_isFinal && m_type) {
                typeRegistry->removeType(m_type);
                delete m_type;
            }
            m_type = nullptr;
        }
        
        type_meta& DataTypeBinder::info() {
            return m_type->m_info;
        }
        
        void DataTypeBinder::setAccessModifier(access_modifier access) {
            m_type->m_access = access;
        }

        void DataTypeBinder::setDestructor(Function* dtor) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_destructor = dtor;
        }

        void DataTypeBinder::addMethod(Function* method) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_methods.push(method);
        }

        void DataTypeBinder::addProperty(type_property&& prop) {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Attempted to modify finalized type definition '%s'",
                    m_type->m_name.c_str()
                ));
            }

            m_type->m_properties.push(prop);
        }
        
        bool DataTypeBinder::propNameExists(const utils::String& name) const {
            return m_type->m_properties.some([name](const type_property& p) {
                return p.name == name;
            });
        }

        DataType* DataTypeBinder::finalize() {
            if (m_isFinal) {
                throw BindException(utils::String::Format(
                    "Type definition '%s' is already finalized",
                    m_type->m_name.c_str()
                ));
            }

            if (!m_type->m_info.is_primitive && !m_type->m_info.is_trivially_destructible && !m_type->m_destructor) {
                throw BindException(utils::String::Format(
                    "Type definition '%s' cannot be finalized: It is not trivially destructible and has no destructor bound",
                    m_type->m_name.c_str()
                ));
            }

            m_isFinal = true;

            if (m_type->m_destructor) {
                if (m_mod) m_mod->addFunction(m_type->m_destructor);
            }

            for (u32 i = 0;i < m_type->m_methods.size();i++) {
                if (m_mod) m_mod->addFunction(m_type->m_methods[i]);
            }

            return m_type;
        }


        
        DataTypeExtender::DataTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, DataType* type) {
            funcRegistry = freg;
            typeRegistry = treg;

            m_type = type;
            m_mod = mod;
            if (!m_type) {
                throw BindException("Attempted to extend type which has not been bound");
            }
        }

        DataTypeExtender::DataTypeExtender(Module* mod, FunctionRegistry* freg, DataTypeRegistry* treg, type_meta&& meta) {
            funcRegistry = freg;
            typeRegistry = treg;

            m_type = treg->getTypeByHostHash(meta.host_hash);
            m_mod = mod;
            if (!m_type) {
                throw BindException("Attempted to extend type which has not been bound");
            }
        }

        DataTypeExtender::~DataTypeExtender() {
            m_type = nullptr;
        }
        
        type_meta& DataTypeExtender::info() {
            return m_type->m_info;
        }

        void DataTypeExtender::addMethod(Function* method) {
            m_type->m_methods.push(method);
        }

        DataTypeExtender& DataTypeExtender::addMethod(
            const utils::String& name,
            DataType* retTp,
            bool returnsPointer,
            const utils::Array<function_argument>& _args,
            compiler::InlineCodeGenFunc genFn,
            access_modifier access
        ) {
            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, typeRegistry->getVoidPtr() });
            args.append(_args);

            FunctionType tmp(m_type, retTp, args, returnsPointer);
            FunctionType* sig = (FunctionType*)typeRegistry->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                typeRegistry->addFuncType(sig);
            }

            Method* m = new Method(
                name,
                utils::String(m_mod ? m_mod->getName() + "::" : ""),
                sig,
                access,
                nullptr,
                nullptr,
                0,
                m_mod
            );

            m->makeInline(genFn);
            addMethod(m);
            
            funcRegistry->registerFunction(m);

            return *this;
        }

        DataTypeExtender& DataTypeExtender::addStaticMethod(
            const utils::String& name,
            DataType* retTp,
            bool returnsPointer,
            const utils::Array<function_argument>& _args,
            compiler::InlineCodeGenFunc genFn,
            access_modifier access
        ) {
            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, typeRegistry->getVoidPtr() });
            args.append(_args);

            FunctionType tmp(nullptr, retTp, args, returnsPointer);
            FunctionType* sig = (FunctionType*)typeRegistry->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                typeRegistry->addFuncType(sig);
            }

            Function* fn = new Function(
                name,
                utils::String(m_mod ? m_mod->getName() + "::" : "") + m_type->getName() + "::",
                sig,
                access,
                nullptr,
                nullptr,
                m_mod
            );

            fn->makeInline(genFn);
            
            funcRegistry->registerFunction(fn);
            addMethod(fn);

            return *this;
        }

        void DataTypeExtender::setDestructor(compiler::InlineCodeGenFunc genFn, access_modifier access) {
            utils::Array<function_argument> args;
            args.push({ arg_type::context_ptr, typeRegistry->getVoidPtr() });

            FunctionType tmp(m_type, typeRegistry->getVoid(), args, false);
            FunctionType* sig = (FunctionType*)typeRegistry->getType(tmp.getId());
            if (!sig) {
                sig = new FunctionType(tmp);
                typeRegistry->addFuncType(sig);
            }

            Function* fn = new Function(
                "destructor",
                utils::String(m_mod ? m_mod->getName() + "::" : "") + m_type->getName() + "::",
                sig,
                access,
                nullptr,
                nullptr,
                m_mod
            );

            fn->makeInline(genFn);
            
            funcRegistry->registerFunction(fn);
        }

        void DataTypeExtender::setDestructor(Function* dtor) {
            if (m_type->m_destructor) throw BindException("Attempted to set destructor for type which already has one");
            m_type->m_destructor = dtor;
        }

        void DataTypeExtender::addProperty(type_property&& prop) {
            if (propNameExists(prop.name)) {
                throw BindException(utils::String::Format("Property '%s' already exists on type '%s'", prop.name.c_str(), m_type->getName().c_str()));
            }

            m_type->m_properties.push(prop);
        }

        type_property& DataTypeExtender::addProperty(const utils::String& name, DataType* type, u64 offset, value_flag_mask flags, access_modifier access) {
            addProperty({
                name,
                access,
                offset,
                type,
                convertPropertyMask(flags),
                nullptr,
                nullptr
            });

            return m_type->m_properties.last();
        }

        type_property& DataTypeExtender::addProperty(const utils::String& name, DataType* type, Function* getter, Function* setter, access_modifier access) {
            value_flag_mask flags = 0;
            if (getter) flags |= vf_read;
            if (setter) flags |= vf_write;

            addProperty({
                name,
                access,
                0,
                type,
                convertPropertyMask(flags),
                getter,
                setter
            });

            return m_type->m_properties.last();
        }
        
        type_property& DataTypeExtender::addProperty(const utils::String& name, DataType* type, compiler::InlineCodeGenFunc getterGenFn, compiler::InlineCodeGenFunc setterGenFn, access_modifier access) {
            value_flag_mask flags = 0;
            if (getterGenFn) flags |= vf_read;
            if (setterGenFn) flags |= vf_write;

            Method* getter = nullptr;
            Method* setter = nullptr;

            if (getterGenFn) {
                utils::Array<function_argument> args = {
                    { arg_type::context_ptr, typeRegistry->getVoidPtr() }
                };

                FunctionType tmp(m_type, type, args, false);
                FunctionType* sig = (FunctionType*)typeRegistry->getType(tmp.getId());
                if (!sig) {
                    sig = new FunctionType(tmp);
                    typeRegistry->addFuncType(sig);
                }

                getter = new Method(
                    "$get_" + name,
                    utils::String(m_mod ? m_mod->getName() + "::" : ""),
                    sig,
                    private_access,
                    nullptr,
                    nullptr,
                    0,
                    m_mod
                );

                getter->makeInline(getterGenFn);
                
                funcRegistry->registerFunction(getter);
            }

            if (setterGenFn) {
                utils::Array<function_argument> args = {
                    { arg_type::context_ptr, typeRegistry->getVoidPtr() },
                    { type->getInfo().is_primitive ? arg_type::value : arg_type::pointer, type }
                };

                FunctionType tmp(m_type, type, args, false);
                FunctionType* sig = (FunctionType*)typeRegistry->getType(tmp.getId());
                if (!sig) {
                    sig = new FunctionType(tmp);
                    typeRegistry->addFuncType(sig);
                }

                setter = new Method(
                    "$set_" + name,
                    utils::String(m_mod ? m_mod->getName() + "::" : ""),
                    sig,
                    private_access,
                    nullptr,
                    nullptr,
                    0,
                    m_mod
                );

                setter->makeInline(getterGenFn);
                
                funcRegistry->registerFunction(setter);
            }

            addProperty({
                name,
                access,
                0,
                type,
                convertPropertyMask(flags),
                getter,
                setter
            });

            return m_type->m_properties.last();
        }

        bool DataTypeExtender::propNameExists(const utils::String& name) const {
            return m_type->m_properties.some([name](const type_property& p) {
                return p.name == name;
            });
        }
    };
};