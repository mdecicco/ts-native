#include <gs/common/TypeRegistry.h>
#include <gs/common/DataType.h>
#include <utils/Array.hpp>

namespace gs {
    namespace ffi {
        DataTypeRegistry::DataTypeRegistry(Context* ctx) : IContextual(ctx) { }
        DataTypeRegistry::~DataTypeRegistry() { }
    };
};