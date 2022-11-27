#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <gs/compiler/Parser.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        Function::Function(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr) {
            m_fullyQualifiedName = signature ? signature->generateFullyQualifiedFunctionName(name) : "";
            m_displayName = signature ? signature->generateFunctionDisplayName(name) : "";
            m_id = -1;
            m_name = name;
            m_signature = signature;
            m_access = access;
            m_address = address;
            m_wrapperAddress = wrapperAddr;
            m_isMethod = false;
            m_isTemplate = false;
        }

        Function::~Function() {
        }

        function_id Function::getId() const {
            return m_id;
        }

        const utils::String& Function::getName() const {
            return m_name;
        }
        
        const utils::String& Function::getDisplayName() const {
            return m_displayName;
        }

        const utils::String& Function::getFullyQualifiedName() const {
            return m_fullyQualifiedName;
        }

        FunctionType* Function::getSignature() const {
            return m_signature;
        }
        
        const SourceLocation& Function::getSource() const {
            return m_src;
        }

        bool Function::isMethod() const {
            return m_isMethod;
        }

        bool Function::isTemplate() const {
            return m_isTemplate;
        }

        access_modifier Function::getAccessModifier() const {
            return m_access;
        }

        void Function::setAccessModifier(access_modifier access) {
            m_access = access;
        }

        bool Function::isThisCall() const {
            return m_signature->getArguments().some([](const function_argument& a) { 
                return a.argType == arg_type::this_ptr;
            });
        }

        void* Function::getAddress() const {
            return m_address;
        }

        void* Function::getWrapperAddress() const {
            return m_wrapperAddress;
        }

        void Function::setThisType(DataType* tp) {
            FunctionType* sig = getSignature();
            sig->setThisType(tp);
            m_fullyQualifiedName = sig->generateFullyQualifiedFunctionName(tp->getFullyQualifiedName() + "::" + m_name);
            m_displayName = sig->generateFunctionDisplayName(tp->getName() + "::" + m_name);
        }



        
        Method::Method(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset)
        : Function(name, signature, access, address, wrapperAddr)
        {
            m_baseOffset = baseOffset;
            m_isMethod = true;
        }

        u64 Method::getThisPtrOffset() const {
            return m_baseOffset;
        }
        
        Method* Method::clone(const utils::String& name, u64 baseOffset) const {
            return new Method(name, getSignature(), getAccessModifier(), getAddress(), getWrapperAddress(), baseOffset);
        }



        
        TemplateFunction::TemplateFunction(const utils::String& name, access_modifier access, compiler::ast_node* ast)
        : Function(name, nullptr, access, nullptr, nullptr) {
            m_ast = ast;
            m_isTemplate = true;
        }
        TemplateFunction::~TemplateFunction() {
            compiler::ast_node::destroyDetachedAST(m_ast);
        }

        TemplateMethod::TemplateMethod(const utils::String& name, access_modifier access, u64 baseOffset, compiler::ast_node* ast)
        : Method(name, nullptr, access, nullptr, nullptr, baseOffset) {
            m_ast = ast;
            m_isTemplate = true;
        }
        TemplateMethod::~TemplateMethod() {
            compiler::ast_node::destroyDetachedAST(m_ast);
        }
    };
};