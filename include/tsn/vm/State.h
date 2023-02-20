#pragma once
#include <tsn/common/types.h>
#include <tsn/vm/types.h>

namespace tsn {
    namespace vm {
        class State {
            public:
                State(u32 stackSize);
                ~State() { }

                // push the value of a register onto stored value stack
                void push(vm_register reg);

                // pop the last value off the stored value stack and put it
                // in a register
                void pop(vm_register reg);

                u8* stackBase;
                u64 registers[u8(vm_register::register_count)];

            protected:
                u64 m_stored[256];
                u8 m_storedIdx;
        };
    };
};