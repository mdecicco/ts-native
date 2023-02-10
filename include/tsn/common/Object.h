#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/ITypedObject.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/utils/remove_all.h>

#include <xtr1common>

namespace utils {
    class String;
};

namespace tsn {
    namespace ffi {
        class DataType;
        class DataTypeRegistry;
    };

    class Object : public ITypedObject, public IContextual {
        public:
            // Constructs an object of the given type with the provided arguments
            template <typename... Args>
            Object(Context* ctx, ffi::DataType* tp, Args&&... args);

            // Creates an empty object of the given type that is prepared for construction
            Object(Context* ctx, ffi::DataType* tp);

            // Creates a view of an object of the given type at the given address, optionally
            // taking ownership of the memory pointed to by the given address.
            //
            // IMPORTANT: If the Object should take ownership of 'ptr', then it _MUST_ be
            // allocated via utils::Mem::alloc. Ignore this rule at your own peril.
            Object(Context* ctx, bool takeOwnership, ffi::DataType* tp, void* ptr);

            // References the given object
            Object(const Object& o);

            // Only destroys the object if the memory holding it is owned and if
            // the reference count is zero
            ~Object();

            template <typename T>
            Object prop(const utils::String& propName, const T& value);

            const Object prop(const utils::String& propName) const;
            Object prop(const utils::String& propName);

            template <typename... Args>
            Object call(const utils::String& funcName, Args&&... args);

            void* getPtr() const;

            template <typename T>
            operator T&();
            
            template <typename T>
            operator const T&() const;
            
            template <typename T>
            operator T*();

            template <typename T>
            operator const T*() const;
        
        protected:
            void* m_data;
            u32* m_dataRefCount;
        
        private:
            void destruct();
    };
};