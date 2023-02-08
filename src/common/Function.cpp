#include <tsn/common/Function.h>
#include <tsn/common/DataType.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/compiler/Parser.h>
#include <utils/Buffer.hpp>
#include <utils/Array.hpp>

namespace tsn {
    namespace ffi {
        //
        // Function
        //
        Function::Function() {
            m_id = -1;
            m_registryIndex = -1;
            m_access = public_access;
            m_signature = nullptr;
            m_address = nullptr;
            m_wrapperAddress = nullptr;
            m_isMethod = false;
            m_isTemplate = false;
        }

        Function::Function(const utils::String& name, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr) {
            m_fullyQualifiedName = signature ? signature->generateFullyQualifiedFunctionName(name) : "";
            m_displayName = signature ? signature->generateFunctionDisplayName(name) : "";
            m_id = -1;
            m_registryIndex = -1;
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
            if (!m_signature) return false;
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

        bool Function::serialize(utils::Buffer* out, Context* ctx) const {
            if (!out->write(m_id)) return false;
            if (!out->write(m_name)) return false;
            if (!out->write(m_displayName)) return false;
            if (!out->write(m_fullyQualifiedName)) return false;
            if (!out->write(m_access)) return false;
            if (!out->write(m_signature ? m_signature->getId() : type_id(0))) return false;
            if (!m_src.serialize(out, ctx)) return false;

            return true;
        }

        bool Function::deserialize(utils::Buffer* in, Context* ctx) {
            auto readStr = [in](utils::String& str) {
                str = in->readStr();
                if (str.size() == 0) return false;
                return true;
            };
            
            auto readType = [in, ctx](FunctionType** out) {
                type_id id;
                if (!in->read(id)) return false;

                if (id == 0) *out = nullptr;
                else {
                    *out = (FunctionType*)ctx->getTypes()->getType(id);
                    if (!*out) return false;
                }
                
                return true;
            };

            if (!in->read(m_id)) return false;
            if (!readStr(m_name)) return false;
            if (!readStr(m_displayName)) return false;
            if (!readStr(m_fullyQualifiedName)) return false;
            if (!in->read(m_access)) return false;
            if (!readType(&m_signature)) return false;

            return true;
        }

        void Function::setThisType(DataType* tp) {
            FunctionType* sig = getSignature();
            sig->setThisType(tp);
            m_fullyQualifiedName = sig->generateFullyQualifiedFunctionName(tp->getFullyQualifiedName() + "::" + m_name);
            m_displayName = sig->generateFunctionDisplayName(tp->getName() + "::" + m_name);
        }



        //
        // Method
        //

        Method::Method() {
            m_baseOffset = 0;
            m_isMethod = true;
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

        bool Method::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Function::serialize(out, ctx)) return false;
            if (!out->write(m_baseOffset)) return false;
            return true;
        }

        bool Method::deserialize(utils::Buffer* in, Context* ctx) {
            if (!Function::deserialize(in, ctx)) return false;
            if (!in->read(m_baseOffset)) return false;
            return true;
        }



        //
        // TemplateFunction
        //
        TemplateFunction::TemplateFunction() {
            m_ast = nullptr;
            m_isTemplate = true;
        }

        TemplateFunction::TemplateFunction(const utils::String& name, access_modifier access, compiler::ParseNode* ast)
        : Function(name, nullptr, access, nullptr, nullptr) {
            m_ast = ast;
            m_isTemplate = true;
        }

        TemplateFunction::~TemplateFunction() {
            if (m_ast) compiler::ParseNode::destroyDetachedAST(m_ast);
            m_ast = nullptr;
        }

        compiler::ParseNode* TemplateFunction::getAST() {
            return m_ast;
        }

        bool TemplateFunction::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Function::serialize(out, ctx)) return false;
            if (!m_ast->serialize(out, ctx)) return false;
            return true;
        }

        bool TemplateFunction::deserialize(utils::Buffer* in, Context* ctx) {
            if (!Function::deserialize(in, ctx)) return false;
            
            m_ast = new compiler::ParseNode();
            if (!m_ast->deserialize(in, ctx)) {
                delete m_ast;
                m_ast = nullptr;
                return false;
            }

            return true;
        }



        //
        // TemplateMethod
        //
        TemplateMethod::TemplateMethod() {
            m_ast = nullptr;
            m_isTemplate = true;
        }

        TemplateMethod::TemplateMethod(const utils::String& name, access_modifier access, u64 baseOffset, compiler::ParseNode* ast)
        : Method(name, nullptr, access, nullptr, nullptr, baseOffset) {
            m_ast = ast;
            m_isTemplate = true;
        }

        TemplateMethod::~TemplateMethod() {
            if (m_ast) compiler::ParseNode::destroyDetachedAST(m_ast);
            m_ast = nullptr;
        }

        compiler::ParseNode* TemplateMethod::getAST() {
            return m_ast;
        }

        bool TemplateMethod::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Method::serialize(out, ctx)) return false;
            if (!m_ast->serialize(out, ctx)) return false;
            return true;
        }

        bool TemplateMethod::deserialize(utils::Buffer* in, Context* ctx) {
            if (!Method::deserialize(in, ctx)) return false;

            m_ast = new compiler::ParseNode();
            if (!m_ast->deserialize(in, ctx)) {
                delete m_ast;
                m_ast = nullptr;
                return false;
            }

            return true;
        }
    };
};