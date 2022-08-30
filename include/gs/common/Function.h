#pragma once
#include <gs/common/types.h>
#include <utils/String.h>

namespace gs {
    namespace ffi {
        class FunctionSignatureType;
        class FunctionRegistry;
        class DataTypeRegistry;

        class Function {
            public:
                Function(const utils::String& name, FunctionSignatureType* signature, access_modifier access, void* address, void* wrapperAddr);
                virtual ~Function();

                function_id getId() const;
                const utils::String& getName() const;
                const utils::String& getFullyQualifiedNameName() const;
                FunctionSignatureType* getSignature() const;
                access_modifier getAccessModifier() const;
                bool isMethod() const;
                bool isThisCall() const;
                
                // Signature:
                // If host function:
                //     If method of class:
                //         RetTp (ThisTp::*)(ArgTypes...)
                //     If normal function:
                //         RetTp (*)(ArgTypes...)
                // If script function:
                //     If function was JIT compiled:
                //         Same as host function signature
                //     If function runs in a VM:
                //         Not a function pointer, but an index into the instruction buffer
                //         The VM handles translating arguments to this function
                void* getAddress() const;

                // Signature:
                // If method of class
                //     RetTp (RetTp (ThisTp::*)(ArgTypes...), RetTp*, ExecutionContext*, ThisTp*, ArgsTypes...)
                // If normal function
                //     RetTp (RetTp (*)(ArgTypes...), RetTp*, ExecutionContext*, ArgsTypes...)
                // If script function
                //     Will be null pointer
                void* getWrapperAddress() const;

            protected:
                bool m_isMethod;
            
            private:
                friend class FunctionRegistry;
                function_id m_id;
                utils::String m_name;
                utils::String m_fullyQualifiedName;
                FunctionSignatureType* m_signature;
                access_modifier m_access;
                void* m_address;
                void* m_wrapperAddress;
        };

        class Method : public Function {
            public:
                Method(const utils::String& name, FunctionSignatureType* signature, access_modifier access, void* address, void* wrapperAddr, u64 baseOffset);

                u64 getThisPtrOffset() const;
                Method* clone(const utils::String& name, u64 baseOffset) const;
            
            private:
                u64 m_baseOffset;
        };
    };
};