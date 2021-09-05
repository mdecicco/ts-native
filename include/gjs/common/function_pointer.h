#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_function;
    class script_buffer;
    class script_context;

    class function_pointer {
        public:
            function_pointer(script_function* target, u64 dataSize = 0, void* data = nullptr);
            function_pointer(u32 func_id, u64 dataSize = 0, void* data = nullptr);
            ~function_pointer();

            void bind_this(void* self);
            void* self_obj() const;

            void* data;
            script_function* target;

        protected:
            void* m_this;
    };
};

