#include <gjs/vm/vm_state.h>

namespace gjs {
    vm_memory::vm_memory(u32 size) : m_size(size) {
        m_data = new u8[size];
    }

    vm_memory::~vm_memory() {
        delete [] m_data;
    }

    vm_state::vm_state(u32 memory_size) : memory(memory_size) {
    }
};