#include <gjs/vm/vm_state.h>

namespace gjs {
    vm_memory::vm_memory(u32 size) : m_size(size) {
        m_data = new u8[size];
    }

    vm_memory::~vm_memory() {
        delete [] m_data;
    }

    vm_state::vm_state(u32 memory_size) : memory(memory_size), m_storedIdx(0) {
    }

    void vm_state::push(vm_register reg) {
        if (m_storedIdx == 255) {
            // todo
            // runtime error
        }

        m_stored[m_storedIdx++] = registers[(u8)(reg)];
    }

    void vm_state::pop(vm_register reg) {
        if (m_storedIdx == 0) {
            // todo
            // runtime error
        }

        registers[(u8)(reg)] = m_stored[--m_storedIdx];
    }
};