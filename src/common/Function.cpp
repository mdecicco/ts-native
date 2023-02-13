#include <tsn/common/Function.h>
#include <tsn/common/DataType.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/Context.h>
#include <tsn/compiler/TemplateContext.h>
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

        Function::Function(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr) {
            m_extraQualifiers = extraQualifiers;
            m_fullyQualifiedName = signature ? signature->generateFullyQualifiedFunctionName(extraQualifiers + name) : "";
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

        bool Function::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!out->write(m_id)) return false;
            if (!out->write(m_name)) return false;
            if (!out->write(m_displayName)) return false;
            if (!out->write(m_fullyQualifiedName)) return false;
            if (!out->write(m_access)) return false;
            if (!out->write(m_signature ? m_signature->getId() : type_id(0))) return false;
            if (!out->write(m_isTemplate)) return false;
            if (!out->write(m_isMethod)) return false;
            if (!m_src.serialize(out, ctx, nullptr)) return false;

            return true;
        }

        bool Function::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }

        void Function::setThisType(DataType* tp) {
            FunctionType* sig = getSignature();
            sig->setThisType(tp);
            m_fullyQualifiedName = sig->generateFullyQualifiedFunctionName(m_extraQualifiers + tp->getName() + "::" + m_name);
            m_displayName = sig->generateFunctionDisplayName(m_extraQualifiers + tp->getName() + "::" + m_name);
        }



        //
        // Method
        //

        Method::Method() {
            m_baseOffset = 0;
            m_isMethod = true;
        }

        Method::Method(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset)
        : Function(name, extraQualifiers, signature, access, address, wrapperAddr)
        {
            m_baseOffset = baseOffset;
            m_isMethod = true;
        }

        u64 Method::getThisPtrOffset() const {
            return m_baseOffset;
        }
        
        Method* Method::clone(const utils::String& name, u64 baseOffset) const {
            return new Method(name, m_extraQualifiers, getSignature(), getAccessModifier(), getAddress(), getWrapperAddress(), baseOffset);
        }

        bool Method::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!Function::serialize(out, ctx, nullptr)) return false;
            if (!out->write(m_baseOffset)) return false;
            return true;
        }

        bool Method::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // TemplateFunction
        //
        TemplateFunction::TemplateFunction() {
            m_data = nullptr;
            m_isTemplate = true;
        }

        TemplateFunction::TemplateFunction(const utils::String& name, const utils::String& extraQualifiers, access_modifier access, compiler::TemplateContext* templateData)
        : Function(name, extraQualifiers, nullptr, access, nullptr, nullptr) {
            m_data = templateData;
            m_isTemplate = true;
        }

        TemplateFunction::~TemplateFunction() {
            if (m_data) delete m_data;
            m_data = nullptr;
        }

        compiler::TemplateContext* TemplateFunction::getTemplateData() {
            return m_data;
        }

        bool TemplateFunction::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!Function::serialize(out, ctx, nullptr)) return false;
            if (!m_data->serialize(out, ctx, nullptr)) return false;
            return true;
        }

        bool TemplateFunction::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }



        //
        // TemplateMethod
        //
        TemplateMethod::TemplateMethod() {
            m_data = nullptr;
            m_isTemplate = true;
        }

        TemplateMethod::TemplateMethod(const utils::String& name, const utils::String& extraQualifiers, access_modifier access, u64 baseOffset, compiler::TemplateContext* templateData)
        : Method(name, extraQualifiers, nullptr, access, nullptr, nullptr, baseOffset) {
            m_data = templateData;
            m_isTemplate = true;
        }

        TemplateMethod::~TemplateMethod() {
            if (m_data) delete m_data;
            m_data = nullptr;
        }

        compiler::TemplateContext* TemplateMethod::getTemplateData() {
            return m_data;
        }

        bool TemplateMethod::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!Method::serialize(out, ctx, nullptr)) return false;
            if (!m_data->serialize(out, ctx, nullptr)) return false;
            return true;
        }

        bool TemplateMethod::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }
    };
};