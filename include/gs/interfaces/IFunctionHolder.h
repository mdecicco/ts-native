#pragma once
#include <gs/common/types.h>

#include <utils/Array.h>
#include <utils/robin_hood.h>

namespace gs {
    namespace ffi {
         class Function;
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
        
        private:
            utils::Array<ffi::Function*> m_funcs;
            robin_hood::unordered_map<function_id, u32> m_funcIdMap;
    };
};