#include <gs/common/TypeRegistry.h>
#include <gs/common/DataType.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        DataTypeRegistry::DataTypeRegistry(Context* ctx) : IContextual(ctx) {
            m_anonTypeCount = 0;
        }
        DataTypeRegistry::~DataTypeRegistry() { }

        u32 DataTypeRegistry::getNextAnonTypeIndex() {
            return m_anonTypeCount++;
        }
    };
};