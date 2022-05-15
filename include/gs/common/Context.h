#pragma once
#include <gs/common/types.h>

namespace gs {
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
        
        protected:
            ffi::DataTypeRegistry* m_types;
            ffi::FunctionRegistry* m_funcs;
    };
};