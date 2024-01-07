#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
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
            m_id = 0;
            m_sourceModule = nullptr;
            m_registryIndex = u32(-1);
            m_access = public_access;
            m_signature = nullptr;
            m_address = nullptr;
            m_wrapperAddress = nullptr;
            m_isMethod = false;
            m_isTemplate = false;
            m_isInline = false;
        }

        Function::Function(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, Module* source) {
            m_extraQualifiers = extraQualifiers;
            m_fullyQualifiedName = signature ? signature->generateFullyQualifiedFunctionName(name, m_extraQualifiers) : "";
            m_displayName = signature ? signature->generateFunctionDisplayName(name, m_extraQualifiers) : "";
            m_id = (function_id)std::hash<utils::String>()(m_fullyQualifiedName);
            m_sourceModule = source;
            m_registryIndex = u32(-1);
            m_name = name;
            m_signature = signature;
            m_access = access;
            m_address = address;
            m_wrapperAddress = wrapperAddr;
            m_isMethod = false;
            m_isTemplate = false;
            m_isInline = false;
        }

        Function::~Function() {
        }

        function_id Function::getId() const {
            return m_id;
        }

        Module* Function::getSourceModule() const {
            return m_sourceModule;
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

        bool Function::isInline() const {
            return m_isInline;
        }

        void Function::makeInline(compiler::InlineCodeGenFunc generatorFn) {
            if (m_address) {
                throw std::exception("Functions which already have entrypoints cannot be made inline");
            }

            m_address = generatorFn;
            m_isInline = true;
        }

        access_modifier Function::getAccessModifier() const {
            return m_access;
        }

        void Function::setAccessModifier(access_modifier access) {
            m_access = access;
        }

        bool Function::isThisCall() const {
            if (!m_signature) return false;
            return m_signature->getThisType() != nullptr;
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
            if (!out->write(m_isTemplate)) return false;
            if (!out->write(m_isMethod)) return false;
            if (!m_src.serialize(out, ctx)) return false;

            return true;
        }

        bool Function::deserialize(utils::Buffer* in, Context* ctx) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }

        void Function::setThisType(DataType* tp, DataTypeRegistry* treg) {
            if (m_registryIndex != u32(-1)) {
                throw std::exception("Function already added to registry, it cannot be changed.");
            }

            DataType* current = m_signature->getThisType();
            if (!current) {
                throw std::exception("Attempted to set 'this' type for function that is not a non-static class method");
            }

            if (current->getId() == tp->getId()) return;

            FunctionType* newSig = new FunctionType(
                tp,
                m_signature->getReturnType(),
                m_signature->getArguments(),
                m_signature->returnsPointer()
            );

            const auto& types = treg->allTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (!info.is_function) continue;
                if (newSig->isEquivalentTo(t)) {
                    delete newSig;
                    newSig = (FunctionType*)t;
                    break;
                }
            }

            m_signature = newSig;

            m_fullyQualifiedName = m_signature->generateFullyQualifiedFunctionName(m_name, m_extraQualifiers);
            m_displayName = m_signature->generateFunctionDisplayName(m_name, m_extraQualifiers);
            m_id = (function_id)std::hash<utils::String>()(m_fullyQualifiedName);
        }
        
        void Function::setRetType(DataType* tp, bool returnsPointer, DataTypeRegistry* treg) {
            if (m_registryIndex != u32(-1)) {
                throw std::exception("Function already added to registry, it cannot be changed.");
            }

            if (m_signature->getReturnType()->getId() == tp->getId()) return;

            FunctionType* newSig = new FunctionType(
                m_signature->getThisType(),
                tp,
                m_signature->getArguments(),
                returnsPointer
            );

            const auto& types = treg->allTypes();
            for (auto* t : types) {
                const auto& info = t->getInfo();
                if (!info.is_function) continue;
                if (newSig->isEquivalentTo(t)) {
                    delete newSig;
                    newSig = (FunctionType*)t;
                    break;
                }
            }

            m_signature = newSig;

            m_fullyQualifiedName = m_signature->generateFullyQualifiedFunctionName(m_name, m_extraQualifiers);
            m_displayName = m_signature->generateFunctionDisplayName(m_name, m_extraQualifiers);
            m_id = (function_id)std::hash<utils::String>()(m_fullyQualifiedName);
        }



        //
        // Method
        //

        Method::Method() {
            m_baseOffset = 0;
            m_isMethod = true;
        }

        Method::Method(const utils::String& name, const utils::String& extraQualifiers, FunctionType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset, Module* source)
        : Function(name, extraQualifiers, signature, access, address, wrapperAddr, source)
        {
            m_baseOffset = baseOffset;
            m_isMethod = true;
        }

        u64 Method::getThisPtrOffset() const {
            return m_baseOffset;
        }
        
        Method* Method::clone(const utils::String& name, u64 baseOffset) const {
            return new Method(name, m_extraQualifiers, getSignature(), getAccessModifier(), getAddress(), getWrapperAddress(), baseOffset, m_sourceModule);
        }

        bool Method::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Function::serialize(out, ctx)) return false;
            if (!out->write(m_baseOffset)) return false;
            return true;
        }

        bool Method::deserialize(utils::Buffer* in, Context* ctx) {
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
        : Function(name, extraQualifiers, nullptr, access, nullptr, nullptr, templateData->getOrigin()) {
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

        bool TemplateFunction::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Function::serialize(out, ctx)) return false;
            if (!m_data->serialize(out, ctx)) return false;
            return true;
        }

        bool TemplateFunction::deserialize(utils::Buffer* in, Context* ctx) {
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
        : Method(name, extraQualifiers, nullptr, access, nullptr, nullptr, baseOffset, templateData->getOrigin()) {
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

        bool TemplateMethod::serialize(utils::Buffer* out, Context* ctx) const {
            if (!Method::serialize(out, ctx)) return false;
            if (!m_data->serialize(out, ctx)) return false;
            return true;
        }

        bool TemplateMethod::deserialize(utils::Buffer* in, Context* ctx) {
            // Functions must be deserialized specially, due to circular dependance between functions and types
            return false;
        }
    };
};