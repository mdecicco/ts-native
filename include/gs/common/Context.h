#pragma once
#include <gs/common/types.h>

namespace gs {
    class Module;
    namespace ffi {
        class DataTypeRegistry;
        class FunctionRegistry;
    };

    class Context {
        public:
            Context();
            ~Context();

            ffi::DataTypeRegistry* getTypes() const;
            ffi::FunctionRegistry* getFunctions() const;
            Module* getGlobal() const;
        
        protected:
            ffi::DataTypeRegistry* m_types;
            ffi::FunctionRegistry* m_funcs;
            Module* m_global;
    };
};