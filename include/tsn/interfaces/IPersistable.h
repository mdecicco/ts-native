#pragma once
#include <tsn/common/types.h>

namespace utils {
    class Buffer;
};

namespace tsn {
    class Context;

    class IPersistable {
        public:
            virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const = 0;
            virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra) = 0;
    };
};