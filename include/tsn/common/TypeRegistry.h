#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/interfaces/IDataTypeHolder.h>

namespace tsn {
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