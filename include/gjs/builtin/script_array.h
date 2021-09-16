#pragma once
#include <gjs/common/types.h>
#include <gjs/util/template_utils.hpp>

namespace gjs {
    class script_type;
    struct subtype_t;
    namespace compile {
        class var;
    };

    class script_array {
        public:
            script_array(u64 moduletype);
            script_array(u64 moduletype, const script_array& o);
            ~script_array();

            void push(subtype_t* elem);
            void clear();
            subtype_t* operator[](u32 idx);
            u32 length() const;
            void for_each(callback<void(*)(u32, subtype_t*)> cb);
            bool some(callback<bool(*)(u32, subtype_t*)> cb);
            subtype_t* find(callback<bool(*)(u32, subtype_t*)> cb);
            i64 findIndex(callback<bool(*)(u32, subtype_t*)> cb);


        protected:
            friend class compile::var;
            u64 m_size;
            u32 m_count;
            u32 m_capacity;
            u8* m_data;
            script_type* m_type;
        };
};

