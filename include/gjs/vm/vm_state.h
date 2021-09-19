#pragma once
#include <gjs/vm/register.h>

namespace gjs {
    class vm_memory {
        public:
            vm_memory(u32 size);
            ~vm_memory();

            inline u8* operator[](u32 addr) { return &m_data[addr]; }
            inline u32 size() const { return m_size; }

        protected:
            u8* m_data;
            u32 m_size;
    };

    class vm_state {
        public:
            vm_state(u32 memory_size);
            ~vm_state() { }

            // push the value of a register onto stored value stack
            void push(vm_register reg);

            // pop the last value off the stored value stack and put it
            // in a register
            void pop(vm_register reg);

            vm_memory memory;
            u64 registers[u8(vm_register::register_count)];

        protected:
            u64 m_stored[256];
            u8 m_storedIdx;
    };
};
