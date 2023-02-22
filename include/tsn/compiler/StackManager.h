#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <list>

namespace tsn {
    namespace compiler {
        class StackManager {
            public:
                struct slot {
                    u32 start;
                    u32 end;
                    bool in_use;
                };

                StackManager();
                ~StackManager();

                void reset();
                alloc_id alloc(u32 sz);
                void free(u32 addr);
                u32 size() const;

            protected:
                std::list<slot> m_slots;
        };
    };
};