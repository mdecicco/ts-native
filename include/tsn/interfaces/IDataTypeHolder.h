#pragma once
#include <tsn/common/types.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace tsn {
    namespace ffi {
        class DataType;
        class FunctionType;
    };
        
    class IDataTypeHolder {
        public:
            IDataTypeHolder();
            ~IDataTypeHolder();

            template <typename T>
            ffi::DataType* getType() const;
            
            template <typename T>
            ffi::DataType* getType(T&& arg) const;

            ffi::DataType* getType(type_id id) const;
            ffi::DataType* getTypeByHostHash(size_t hash) const;
            const utils::Array<ffi::DataType*>& allTypes() const;
            u32 typeCount() const;

            void addHostType(size_t hash, ffi::DataType* tp);
            void addForeignType(ffi::DataType* tp);
            void addFuncType(ffi::FunctionType* tp);
            void removeType(ffi::DataType* tp);
        
        private:
            utils::Array<ffi::DataType*> m_types;
            robin_hood::unordered_map<type_id, u32> m_typeIdMap;
            robin_hood::unordered_map<size_t, u32> m_typeHashMap;
    };
};