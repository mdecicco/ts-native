#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_type;
    class script_object;
    struct subtype_t;

    class script_pointer {
        public:
            script_pointer(u64 moduletype);
            ~script_pointer();

            void reset(subtype_t* v);
            void share(const script_pointer& v);
            void release();
            bool is_null() const;
            u32 references() const;
            subtype_t* get() const;

            // same as share
            script_pointer& operator=(const script_pointer& o);

        protected:
            void destruct();

            u8* m_data;
            u32* m_refCount;
            script_type* m_type;
    };
};

