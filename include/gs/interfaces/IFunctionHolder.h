#pragma once
#include <gs/common/types.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace utils {
    class String;
};

namespace gs {
    namespace ffi {
         class Function;
         class DataType;
         class DataTypeRegistry;
    };

    class IFunctionHolder {
        public:
            IFunctionHolder();
            ~IFunctionHolder();

            ffi::Function* getFunction(function_id id) const;
            ffi::Function* getFunctionByIndex(u32 index) const;
            u32 getFunctionIndex(function_id id) const;
            const utils::Array<ffi::Function*>& allFunctions() const;
            u32 functionCount() const;

            void mergeFunctions(IFunctionHolder& funcs);

            void addFunction(ffi::Function* fn);

            /**
             * @brief Find functions that match the provided signature
             * 
             * @tparam Ret Signature return type
             * @tparam Args Signature argument types
             * 
             * @param types Context's type registry
             * @param name Function name
             * @param funcs Set of functions to search
             * @param flags Search options (See 'function_match_flags' documentation for more info)
             * 
             * @return Subset of 'funcs' that match the provided signature
             */
            template <typename Ret, typename...Args>
            utils::Array<ffi::Function*> findFunctions(
                ffi::DataTypeRegistry* types,
                const utils::String& name,
                const utils::Array<ffi::Function*>& funcs,
                function_match_flags flags
            );

            /**
             * @brief Find functions that match the provided signature
             * 
             * @param name Function name
             * @param retTp Signature return type
             * @param argTps Signature argument types
             * @param argCount Signature argument count
             * @param funcs Set of functions to search
             * @param flags Search options (See 'function_match_flags' documentation for more info)
             * @return Subset of 'funcs' that match the provided signature
             */
            utils::Array<ffi::Function*> findFunctions(
                const utils::String& name,
                ffi::DataType* retTp,
                ffi::DataType** argTps,
                u8 argCount,
                const utils::Array<ffi::Function*>& funcs,
                function_match_flags flags
            );
        
        private:
            utils::Array<ffi::Function*> m_funcs;
            robin_hood::unordered_map<function_id, u32> m_funcIdMap;
    };
};