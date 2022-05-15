#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/ITypedObject.h>
#include <gs/utils/remove_all.h>

#include <xtr1common>

namespace gs {
    namespace ffi {
        class DataType;
        class DataTypeRegistry;
    };

    class Object : public ITypedObject {
        public:
            template <typename... Args>
            Object(ffi::DataTypeRegistry* reg, ffi::DataType* tp, Args...);
            Object(ffi::DataType* tp);
            Object(ffi::DataType* tp, void* ptr);
            Object(const Object& o);
            ~Object();

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