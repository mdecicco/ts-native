#include <gs/common/Function.h>
#include <gs/common/DataType.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        Function::Function(const utils::String& name, FunctionSignatureType* signature, access_modifier access, void* address, void* wrapperAddr) {
            m_fullyQualifiedName = signature->generateFullyQualifiedFunctionName(name);
            m_id = -1;
            m_name = name;
            m_signature = signature;
            m_access = access;
            m_address = address;
            m_wrapperAddress = wrapperAddr;
            m_isMethod = false;
        }

        Function::~Function() {
        }

        function_id Function::getId() const {
            return m_id;
        }

        const utils::String& Function::getName() const {
            return m_name;
        }

        const utils::String& Function::getFullyQualifiedNameName() const {
            return m_fullyQualifiedName;
        }

        FunctionSignatureType* Function::getSignature() const {
            return m_signature;
        }

        bool Function::isMethod() const {
            return m_isMethod;
        }

        access_modifier Function::getAccessModifier() const {
            return m_access;
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

        
        Method::Method(const utils::String& name, FunctionSignatureType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset)
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
    };
};