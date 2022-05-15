#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/ITypedObject.h>
#include <gs/interfaces/IContextual.h>
#include <gs/utils/remove_all.h>

#include <xtr1common>

namespace utils {
    class String;
};

namespace gs {
    namespace ffi {
        class DataType;
        class DataTypeRegistry;
    };

    class Object : public ITypedObject, public IContextual {
        public:
            // Constructs an object of the given type with the provided arguments
            template <typename... Args>
            Object(Context* ctx, ffi::DataType* tp, Args...);

            // Creates an empty object of the given type that is prepared for construction
            Object(Context* ctx, ffi::DataType* tp);

            // Creates a view of an object of the given type at the given address
            Object(Context* ctx, ffi::DataType* tp, void* ptr);

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
            Object call(const utils::String& funcName, Args... args);

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