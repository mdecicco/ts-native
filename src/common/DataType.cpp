#include <gs/common/DataType.h>
#include <gs/common/Function.h>
#include <gs/compiler/Parser.h>
#include <gs/utils/function_match.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        //
        // DataType
        //

        DataType::DataType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            const type_meta& info
        ) {
            m_id = -1;
            m_name = name;
            m_fullyQualifiedName = fullyQualifiedName;
            m_info = info;
            m_destructor = nullptr;
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
            m_id = -1;
            m_name = name;
            m_fullyQualifiedName = fullyQualifiedName;
            m_info = info;
            m_properties = properties;
            m_bases = bases;
            m_destructor = dtor;
            m_methods = methods;
        }

        DataType::DataType() {
            m_id = -1;
            m_destructor = nullptr;
            memset(&m_info, 0, sizeof(type_meta));
        }

        DataType::~DataType() {
        }

        
        utils::Array<ffi::Function*> DataType::findMethods(
            const utils::String& name,
            DataType* retTp,
            DataType** argTps,
            u8 argCount,
            function_match_flags flags
        ) {
            return function_match(name, retTp, argTps, argCount, m_methods, flags);
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

        bool DataType::isConvertibleTo(DataType* to) const {
            return false;
        }

        bool DataType::isImplicitlyAssignableTo(DataType* to) const {
            return false;
        }

        bool DataType::isEquivalentTo(DataType* to) const {
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
                m_info.is_alias                     != to->m_info.is_alias                      ||
                m_methods.size()                    != to->m_methods.size()                     ||
                m_properties.size()                 != to->m_properties.size()                  ||
                m_bases.size()                      != to->m_bases.size()                       ||
                (m_destructor == nullptr)           != (to->m_destructor == nullptr)
            ) return false;

            // check method signatures
            for (u32 i = 0;i < m_methods.size();i++) {
                Function* m1 = m_methods[i];
                Function* m2 = to->m_methods[i];
                if (m1->isMethod() != m2->isMethod()) return false;
                if (m1->getAccessModifier() != m2->getAccessModifier()) return false;
                if (!m1->getSignature()->isEquivalentTo(m2->getSignature())) return false;
            }

            // check properties
            for (u32 i = 0;i < m_properties.size();i++) {
                const type_property& p1 = m_properties[i];
                const type_property& p2 = to->m_properties[i];

                if (p1.offset != p2.offset) return false;
                if (p1.access != p2.access) return false;
                if (p1.flags.can_read != p2.flags.can_read) return false;
                if (p1.flags.can_write != p2.flags.can_write) return false;
                if (p1.flags.is_pointer != p2.flags.is_pointer) return false;
                if (p1.flags.is_static != p2.flags.is_static) return false;
                if (p1.name != p2.name) return false;
                if (p1.setter) {
                    if (!p2.setter) return false;
                    if (p1.setter->isMethod() != p2.setter->isMethod()) return false;
                    if (p1.setter->getAccessModifier() != p2.setter->getAccessModifier()) return false;
                    if (!p1.setter->getSignature()->isEquivalentTo(p2.setter->getSignature())) return false;
                }
                if (p1.getter) {
                    if (!p2.getter) return false;
                    if (p1.getter->isMethod() != p2.getter->isMethod()) return false;
                    if (p1.getter->getAccessModifier() != p2.getter->getAccessModifier()) return false;
                    if (!p1.getter->getSignature()->isEquivalentTo(p2.getter->getSignature())) return false;
                }
                if (!p1.type->isEquivalentTo(p2.type)) return false;
            }

            return true;
        }

        DataType* DataType::clone(const utils::String& name, const utils::String& fullyQualifiedName) const {
            return new DataType(name, fullyQualifiedName, m_info, m_properties, m_bases, m_destructor, m_methods);
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
        
        FunctionType::FunctionType(DataType* returnType, const utils::Array<function_argument>& args) {
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

            m_id = std::hash<utils::String>()(m_fullyQualifiedName);
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

                bool is_implicit = arg.argType == arg_type::this_ptr || arg.argType == arg_type::context_ptr;
                bool is_ptr = is_implicit || arg.argType == arg_type::pointer;

                if (is_implicit) name += "$";

                name += arg.dataType->m_fullyQualifiedName;

                if (is_ptr) name += "*";
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



        //
        // TemplateType
        //
        type_meta templateTypeMeta (compiler::ast_node* n) {
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

        TemplateType::TemplateType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            compiler::ast_node* baseAST
        ) : DataType(name, fullyQualifiedName, templateTypeMeta(baseAST)) {
            m_ast = baseAST;
        }

        TemplateType::~TemplateType() {
            compiler::ast_node::destroyDetachedAST(m_ast);
        }

        compiler::ast_node* TemplateType::getAST() const {
            return m_ast;
        }



        //
        // AliasType
        //
        
        type_meta aliasTypeMeta (DataType* refTp) {
            type_meta i = refTp->getInfo();
            i.is_alias = 1;
            return i;
        }

        AliasType::AliasType(
            const utils::String& name,
            const utils::String& fullyQualifiedName,
            DataType* refTp
        ) : DataType(name, fullyQualifiedName, aliasTypeMeta(refTp)) {
            m_ref = refTp;
        }

        AliasType::~AliasType() {
            
        }

        DataType* AliasType::getRefType() const {
            return m_ref;
        }
    };
};