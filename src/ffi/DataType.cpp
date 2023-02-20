#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/compiler/TemplateContext.h>
#include <tsn/compiler/Parser.h>
#include <tsn/utils/function_match.h>
#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace ffi {
        //
        // DataType
        //

        DataType::DataType() {
            m_id = 0;
            m_name = "";
            m_fullyQualifiedName = "";
            m_info = { 0 };
            m_destructor = nullptr;
            m_access = public_access;
            m_itype = dti_plain;
            m_templateBase = nullptr;
        }

        DataType::DataType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            const type_meta& info
        ) {
            m_id = (type_id)std::hash<utils::String>()(fullyQualifiedName);
            m_name = name;
            m_fullyQualifiedName = fullyQualifiedName;
            m_info = info;
            m_destructor = nullptr;
            m_access = public_access;
            m_itype = dti_plain;
            m_templateBase = nullptr;
        }

        DataType::DataType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            const type_meta& info,
            const utils::Array<type_property>& properties,
            const utils::Array<type_base>& bases,
            Function* dtor,
            const utils::Array<Function*>& methods
        ) {
            m_id = (type_id)std::hash<utils::String>()(fullyQualifiedName);
            m_name = name;
            m_fullyQualifiedName = fullyQualifiedName;
            m_info = info;
            m_properties = properties;
            m_bases = bases;
            m_destructor = dtor;
            m_methods = methods;
            m_access = public_access;
            m_itype = dti_plain;
            m_templateBase = nullptr;
        }

        DataType::~DataType() {
        }

        
        utils::Array<ffi::Function*> DataType::findMethods(
            const utils::String& name,
            const DataType* retTp,
            const DataType** argTps,
            u8 argCount,
            function_match_flags flags
        ) const {
            return function_match(name, retTp, argTps, argCount, m_methods, flags);
        }

        const type_property* DataType::getProp(const utils::String& name, bool excludeInherited, bool excludePrivate) const {
            const type_property* p = m_properties.find([name, excludePrivate](type_property p) {
                if (excludePrivate && p.access == private_access) return false;
                return p.name == name;
            });

            if (p) return p;
            
            if (!excludeInherited) {
                m_bases.some([&name, &p, excludePrivate](type_base b) {
                    p = b.type->getProp(name, false, excludePrivate);
                    return p != nullptr;
                });
            }

            return p;
        }

        type_id DataType::getId() const {
            return m_id;
        }

        const utils::String& DataType::getName() const {
            return m_name;
        }

        const utils::String& DataType::getFullyQualifiedName() const {
            return m_fullyQualifiedName;
        }

        const type_meta& DataType::getInfo() const {
            return m_info;
        }

        const utils::Array<type_property>& DataType::getProperties() const {
            return m_properties;
        }

        const utils::Array<Function*>& DataType::getMethods() const {
            return m_methods;
        }

        const utils::Array<type_base>& DataType::getBases() const {
            return m_bases;
        }

        Function* DataType::getDestructor() const {
            return m_destructor;
        }
        
        access_modifier DataType::getAccessModifier() const {
            return m_access;
        }

        void DataType::setAccessModifier(access_modifier access) {
            m_access = access;
        }

        bool DataType::isConvertibleTo(const DataType* to) const {
            if (!to) return false;
            if (m_info.is_primitive && to->m_info.is_primitive) return true;

            if (m_info.is_host) {
                size_t data_hash = type_hash<void*>();
                size_t null_hash = type_hash<null_t>();
                if (m_info.host_hash == data_hash || m_info.host_hash == null_hash) {
                    if (to->m_info.host_hash == data_hash || to->m_info.host_hash == null_hash) {
                        return true;
                    }
                }
            }

            if (isEqualTo(to)) return true;

            const DataType* toEffective = to->getEffectiveType();
            auto castMethods = findMethods("operator " + toEffective->getFullyQualifiedName(), toEffective, nullptr, 0, fm_skip_implicit_args | fm_strict_return);
            if (castMethods.size() == 1) return true;

            const DataType* self = this->getEffectiveType();
            auto copyCtors = to->findMethods("constructor", nullptr, &self, 1, fm_skip_implicit_args | fm_strict_args);
            return copyCtors.size() == 1;
        }
        
        bool DataType::isImplicitlyAssignableTo(const DataType* to) const {
            if (!to) return false;
            if (m_info.is_primitive && to->m_info.is_primitive) return true;

            return isEquivalentTo(to) && m_info.is_trivially_copyable && to->m_info.is_trivially_copyable;
        }

        bool DataType::isEquivalentTo(const DataType* _to) const {
            if (!_to) return false;
            const DataType* to = _to->getEffectiveType();
            if (isEqualTo(to)) return true;
            if (
                m_info.size                         != to->m_info.size                          ||
                m_info.is_pod                       != to->m_info.is_pod                        ||
                m_info.is_trivially_constructible   != to->m_info.is_trivially_constructible    ||
                m_info.is_trivially_copyable        != to->m_info.is_trivially_copyable         ||
                m_info.is_trivially_destructible    != to->m_info.is_trivially_destructible     ||
                m_info.is_primitive                 != to->m_info.is_primitive                  ||
                m_info.is_floating_point            != to->m_info.is_floating_point             ||
                m_info.is_integral                  != to->m_info.is_integral                   ||
                m_info.is_unsigned                  != to->m_info.is_unsigned                   ||
                m_info.is_function                  != to->m_info.is_function                   ||
                m_info.is_template                  != to->m_info.is_template                   ||
                m_methods.size()                    != to->m_methods.size()                     ||
                m_properties.size()                 != to->m_properties.size()                  ||
                m_bases.size()                      != to->m_bases.size()                       ||
                (m_destructor == nullptr)           != (to->m_destructor == nullptr)
            ) return false;

            const DataType* effectiveSelf = getEffectiveType();
            // check method signatures
            for (u32 i = 0;i < effectiveSelf->m_methods.size();i++) {
                Function* m1 = effectiveSelf->m_methods[i];
                Function* m2 = to->m_methods[i];
                if (m1->isMethod() != m2->isMethod()) return false;
                if (m1->getAccessModifier() != m2->getAccessModifier()) return false;
                if (!m1->getSignature()->isEqualTo(m2->getSignature())) return false;
            }

            // check properties
            for (u32 i = 0;i < effectiveSelf->m_properties.size();i++) {
                const type_property& p1 = effectiveSelf->m_properties[i];
                const type_property& p2 = to->m_properties[i];

                if (p1.offset != p2.offset) return false;
                if (p1.access != p2.access) return false;
                if (p1.flags.can_read != p2.flags.can_read) return false;
                if (p1.flags.can_write != p2.flags.can_write) return false;
                if (p1.flags.is_pointer != p2.flags.is_pointer) return false;
                if (p1.flags.is_static != p2.flags.is_static) return false;
                if (p1.name != p2.name) return false;
                if (!p1.type->isEqualTo(p2.type)) return false;
            }

            return true;
        }
        
        bool DataType::isEqualTo(const DataType* to) const {
            if (!to) return false;
            return getEffectiveType()->m_id == to->getEffectiveType()->m_id;
        }
        
        bool DataType::isConstructableWith(const utils::Array<DataType*>& args) const {
            const DataType* self = this->getEffectiveType();
            auto ctors = findMethods(
                "constructor",
                nullptr,
                const_cast<const DataType**>(args.data()), args.size(),
                fm_skip_implicit_args
            );
            return ctors.size() == 1;
        }

        DataType* DataType::clone(const utils::String& name, const utils::String& fullyQualifiedName) const {
            return new DataType(name, fullyQualifiedName, m_info, m_properties, m_bases, m_destructor, m_methods);
        }
        
        const DataType* DataType::getEffectiveType() const {
            if (m_info.is_alias) return ((AliasType*)this)->getRefType()->getEffectiveType();
            return this;
        }
        
        DataType* DataType::getEffectiveType() {
            if (m_info.is_alias) return ((AliasType*)this)->getRefType()->getEffectiveType();
            return this;
        }
        
        data_type_instance DataType::getInstanceType() const {
            return m_itype;
        }

        ffi::TemplateType* DataType::getTemplateBase() const {
            return getEffectiveType()->m_templateBase;
        }

        const utils::Array<DataType*>& DataType::getTemplateArguments() const {
            return getEffectiveType()->m_templateArgs;
        }

        bool DataType::isInstantiationOf(ffi::TemplateType* templ) const {
            TemplateType* templBase = getTemplateBase();

            if (!templBase || !templ) return false;
            return templBase->isEqualTo(templ);
        }

        bool DataType::isInstantiationOf(ffi::TemplateType* templ, const utils::Array<ffi::DataType*>& withArgs) const {
            TemplateType* templBase = getTemplateBase();
            const utils::Array<DataType*>& templArgs = getTemplateArguments();

            if (!templBase || !templ) return false;
            if (templArgs.size() != withArgs.size()) return false;
            if (!templBase->isEqualTo(templ)) return false;

            return !templArgs.some([&withArgs](const DataType* tp, u32 idx) {
                return !tp->isEqualTo(withArgs[idx]);
            });
        }
            
        bool DataType::serialize(utils::Buffer* out, Context* ctx) const {
            auto writeFunc = [out](const Function* f) {
                if (f) return !out->write(f->getId());
                return !out->write(function_id(0));
            };

            auto writeProperty = [out, writeFunc](const type_property& p) {
                if (!out->write(p.name)) return true;
                if (!out->write(p.access)) return true;
                if (!out->write(p.offset)) return true;
                if (!out->write(p.type->getId())) return true;
                if (!out->write(p.flags)) return true;
                if (writeFunc(p.getter)) return true;
                if (writeFunc(p.setter)) return true;
                return false;
            };

            auto writeBase = [out](const type_base& b) {
                if (!out->write(b.type->getId())) return true;
                if (!out->write(b.offset)) return true;
                if (!out->write(b.access)) return true;
                return false;
            };

            if (!out->write(m_id)) return false;
            if (!out->write(m_itype)) return false;
            if (!out->write(m_name)) return false;
            if (!out->write(m_fullyQualifiedName)) return false;
            if (!out->write(m_info)) return false;
            if (!out->write(m_access)) return false;
            if (writeFunc(m_destructor)) return false;

            if (!out->write(m_properties.size())) return false;
            if (m_properties.some(writeProperty)) return false;

            if (!out->write(m_bases.size())) return false;
            if (m_bases.some(writeBase)) return false;

            if (!out->write(m_methods.size())) return false;
            if (m_methods.some(writeFunc)) return false;

            if (!out->write(m_templateBase ? m_templateBase->getId() : 0)) return false;
            if (!out->write(m_templateArgs.size())) return false;
            for (u32 i = 0;i < m_templateArgs.size();i++) {
                if (!out->write(m_templateArgs[i]->getId())) return false;
            }

            return true;
        }

        bool DataType::deserialize(utils::Buffer* in, Context* ctx) {
            // Types must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // function_argument
        //

        bool function_argument::isImplicit() const {
            return (u32)argType <= (u32)arg_type::this_ptr;
        }


        //
        // FunctionSignatureType
        //
        FunctionType::FunctionType() {
            m_returnType = nullptr;
            m_itype = dti_function;
        }

        FunctionType::FunctionType(DataType* returnType, const utils::Array<function_argument>& args) {
            m_itype = dti_function;
            m_name = returnType->m_name + "(";
            m_fullyQualifiedName = returnType->m_fullyQualifiedName + "(";
            args.each([this](const function_argument& arg, u32 idx) {
                if (idx > 0) {
                    m_name += ",";
                    m_fullyQualifiedName += ",";
                }

                bool is_implicit = arg.argType == arg_type::func_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::ret_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::context_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::this_ptr;
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) {
                    m_name += "$";
                    m_fullyQualifiedName += "$";
                }

                m_name += arg.dataType->m_name;
                m_fullyQualifiedName += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) {
                    m_name += "*";
                    m_fullyQualifiedName += "*";
                }
            });

            m_name += ")";
            m_fullyQualifiedName += ")";

            m_id = (type_id)std::hash<utils::String>()(m_fullyQualifiedName);
            m_info = {
                1            , // is pod
                0            , // is_trivially_constructible
                0            , // is_trivially_copyable
                0            , // is_trivially_destructible
                0            , // is_primitive
                0            , // is_floating_point
                0            , // is_integral
                0            , // is_unsigned
                1            , // is_function
                0            , // is_template
                0            , // is_alias
                1            , // is_host
                0            , // is_anonymous
                sizeof(void*), // size
                0              // host_hash
            };

            m_returnType = returnType;
            m_args = args;
        }

        FunctionType::~FunctionType() {
        }

        utils::String FunctionType::generateFullyQualifiedFunctionName(const utils::String& funcName) {
            utils::String name = m_returnType->m_fullyQualifiedName + " " + funcName + "(";
            m_args.each([&name](const function_argument& arg, u32 idx) {
                if (idx > 0) name += ",";

                bool is_implicit = arg.isImplicit();
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) name += "$";

                name += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) name += "*";
            });

            name += ")";

            return name;
        }

        utils::String FunctionType::generateFunctionDisplayName(const utils::String& funcName) {
            utils::String name = m_returnType->m_name + " " + funcName + "(";
            u32 aidx = 0;
            m_args.each([&name, &aidx](const function_argument& arg, u32 idx) {
                if (arg.isImplicit()) return;
                if (aidx > 0) name += ",";

                name += arg.dataType->m_name;

                if (arg.argType == arg_type::pointer) name += "*";
                aidx++;
            });

            name += ")";

            return name;
        }

        DataType* FunctionType::getReturnType() const {
            return m_returnType;
        }

        const utils::Array<function_argument>& FunctionType::getArguments() const {
            return m_args;
        }

        bool FunctionType::isEquivalentTo(DataType* to) const {
            if (!to->getInfo().is_function) return false;
            FunctionType* s = (FunctionType*)to;
            
            if (m_args.size() != s->m_args.size()) return false;
            if (!m_returnType->isEquivalentTo(s->m_returnType)) return false;
            for (u32 i = 0;i < m_args.size();i++) {
                if (m_args[i].argType != s->m_args[i].argType) return false;
                if (!m_args[i].dataType->isEquivalentTo(s->m_args[i].dataType)) return false;
            }

            return true;
        }

        void FunctionType::setThisType(DataType* tp) {
            if (m_args.size() < 4 || m_args[3].argType != arg_type::this_ptr) {
                throw std::exception("Attempted to set 'this' type for function that is not a non-static class method");
            }

            m_args[3].dataType = tp;

            m_name = m_returnType->m_name + "(";
            m_fullyQualifiedName = m_returnType->m_fullyQualifiedName + "(";
            m_args.each([this](const function_argument& arg, u32 idx) {
                if (idx > 0) {
                    m_name += ",";
                    m_fullyQualifiedName += ",";
                }

                bool is_implicit = arg.argType == arg_type::func_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::ret_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::context_ptr;
                is_implicit = is_implicit || arg.argType == arg_type::this_ptr;
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) {
                    m_name += "$";
                    m_fullyQualifiedName += "$";
                }

                m_name += arg.dataType->m_name;
                m_fullyQualifiedName += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) {
                    m_name += "*";
                    m_fullyQualifiedName += "*";
                }
            });

            m_name += ")";
            m_fullyQualifiedName += ")";
        }
            
        bool FunctionType::serialize(utils::Buffer* out, Context* ctx) const {
            if (!DataType::serialize(out, ctx)) return false;

            auto writeArg = [out](const function_argument& a) {
                if (!out->write(a.argType)) return true;
                if (!out->write(a.dataType->getId())) return true;
                return false;
            };

            if (!out->write(m_returnType ? m_returnType->getId() : type_id(0))) return false;
            if (!out->write(m_args.size())) return false;
            if (m_args.some(writeArg)) return false;

            return true;
        }

        bool FunctionType::deserialize(utils::Buffer* in, Context* ctx) {
            // Types must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // TemplateType
        //
        type_meta templateTypeMeta (const compiler::ParseNode* n) {
            return {
                0, // is_pod
                0, // is_trivially_constructible
                0, // is_trivially_copyable
                0, // is_trivially_destructible
                0, // is_primitive
                0, // is_floating_point
                0, // is_integral
                0, // is_unsigned
                   // is_function
                (n->tp == compiler::nt_function) ? (unsigned)1 : (unsigned)0,
                1, // is_template
                0, // is_alias
                0, // is_host
                0, // is_anonymous
                0, // size
                0, // host_hash
            };
        }

        TemplateType::TemplateType() {
            m_data = nullptr;
            m_itype = dti_template;
        }

        TemplateType::TemplateType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            compiler::TemplateContext* templateData
        ) : DataType(name, fullyQualifiedName, templateTypeMeta(templateData->getAST())) {
            m_data = templateData;
            m_itype = dti_template;
        }

        TemplateType::~TemplateType() {
            if (m_data) delete m_data;
        }

        compiler::TemplateContext* TemplateType::getTemplateData() const {
            return m_data;
        }
            
        bool TemplateType::serialize(utils::Buffer* out, Context* ctx) const {
            if (!DataType::serialize(out, ctx)) return false;
            if (!m_data->serialize(out, ctx)) return false;
            return true;
        }

        bool TemplateType::deserialize(utils::Buffer* in, Context* ctx) {
            // Types must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // AliasType
        //
        
        type_meta aliasTypeMeta (DataType* refTp) {
            type_meta i = refTp->getInfo();
            i.is_alias = 1;
            i.is_anonymous = 0;
            return i;
        }

        AliasType::AliasType() {
            m_ref = nullptr;
            m_itype = dti_alias;
        }

        AliasType::AliasType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            DataType* refTp
        ) : DataType(name, fullyQualifiedName, aliasTypeMeta(refTp)) {
            m_ref = refTp;
            m_itype = dti_alias;
        }

        AliasType::~AliasType() {
            
        }

        DataType* AliasType::getRefType() const {
            return m_ref;
        }
            
        bool AliasType::serialize(utils::Buffer* out, Context* ctx) const {
            if (!DataType::serialize(out, ctx)) return false;
            if (!out->write(m_ref->getId())) return false;
            return true;
        }

        bool AliasType::deserialize(utils::Buffer* in, Context* ctx) {
            // Types must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // ClassType
        //
        ClassType::ClassType() {
            m_itype = dti_class;
        }

        ClassType::ClassType(const utils::String& name, const utils::String& fullyQualifiedName) : DataType(name, fullyQualifiedName, { 0 }, {}, {}, nullptr, {}) {
            m_info.is_pod = 1;
            m_info.is_trivially_constructible = 1;
            m_info.is_trivially_copyable = 1;
            m_info.is_trivially_destructible = 1;
            m_itype = dti_class;
        }

        ClassType::~ClassType() {
        }

        void ClassType::addBase(DataType* tp, access_modifier access) {
            type_base b;
            b.access = access;
            b.offset = m_info.size;
            b.type = tp;

            m_info.size += tp->m_info.size;

            if (!tp->m_info.is_pod) m_info.is_pod = 0;
            if (!tp->m_info.is_trivially_constructible) m_info.is_trivially_constructible = 0;
            if (!tp->m_info.is_trivially_copyable) m_info.is_trivially_copyable = 0;
            if (!tp->m_info.is_trivially_destructible) m_info.is_trivially_destructible = 0;

            m_bases.push(b);
        }

        void ClassType::addProperty(const utils::String& name, DataType* tp, value_flags flags, access_modifier access, Function* getter, Function* setter) {
            type_property p;
            p.name = name;
            p.type = tp;
            p.flags = flags;
            p.access = access;
            p.offset = m_info.size;
            p.getter = getter;
            p.setter = setter;
            
            m_info.size += tp->m_info.size;

            if (!tp->m_info.is_pod) m_info.is_pod = 0;
            if (!tp->m_info.is_trivially_constructible) m_info.is_trivially_constructible = 0;
            if (!tp->m_info.is_trivially_copyable) m_info.is_trivially_copyable = 0;
            if (!tp->m_info.is_trivially_destructible) m_info.is_trivially_destructible = 0;
            m_properties.push(p);
        }

        void ClassType::addMethod(ffi::Method* method) {
            m_methods.push(method);
        }

        void ClassType::setDestructor(ffi::Method* dtor) {
            m_destructor = dtor;
        }
    };
};