#include <gjs/common/types.h>
#include <list>

namespace gjs {
    class func_stack {
        public:
            struct slot {
                u64 start;
                u64 end;
                bool in_use;
            };

            func_stack();
            ~func_stack();

            u64 alloc(u64 sz);
            void free(u64 addr);
            u64 size() const;

        protected:
            std::list<slot> m_slots;
    };
};