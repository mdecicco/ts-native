#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IDataTypeHolder.h>

namespace gs {
    namespace ffi {
        class DataTypeRegistry : public IContextual, public IDataTypeHolder {
            public:
                DataTypeRegistry(Context* ctx);
                ~DataTypeRegistry();
        };
    };
};