#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace gs {
    namespace ffi {
        class DataType;
        class FunctionSignatureType;
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
            const utils::Array<ffi::DataType*>& allTypes() const;
            u32 typeCount() const;

            void mergeTypes(IDataTypeHolder& types);

            void addHostType(size_t hash, ffi::DataType* tp);
            void addForeignType(ffi::DataType* tp);
            void addFuncType(ffi::FunctionSignatureType* tp);
            void removeType(ffi::DataType* tp);
        
        private:
            utils::Array<ffi::DataType*> m_types;
            robin_hood::unordered_map<type_id, u32> m_typeIdMap;
            robin_hood::unordered_map<size_t, u32> m_typeHashMap;
    };
};