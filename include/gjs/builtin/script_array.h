#pragma once
#include <gjs/common/types.h>

namespace gjs {
    class script_type;
    struct subtype_t;

    class script_array {
        public:
            script_array(u64 moduletype);
            ~script_array();

            void push(subtype_t* elem);
            subtype_t* operator[](u32 idx);
            u32 length() const;

        protected:
            u64 m_size;
            u32 m_count;
            u32 m_capacity;
            u8* m_data;
            script_type* m_type;
        };
};

