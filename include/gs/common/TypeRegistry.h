#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IDataTypeHolder.h>

namespace gs {
    namespace compiler {
        class Compiler;
    };

    namespace ffi {
        class DataType;
        class DataTypeRegistry : public IContextual, public IDataTypeHolder {
            public:
                DataTypeRegistry(Context* ctx);
                ~DataTypeRegistry();

                u32 getNextAnonTypeIndex();
            
            protected:
                u32 m_anonTypeCount;
        };
    };
};