#include <instruction_array.h>
#include <allocator.h>
#include <string.h>

#define RESIZE_AMOUNT 32

namespace gjs {
    instruction_array::instruction_array(vm_allocator* allocator) : m_allocator(allocator), m_count(0), m_capacity(RESIZE_AMOUNT) {
        m_instructions = (instruction*)allocator->allocate(m_capacity * sizeof(instruction));
        memset(m_instructions, 0, sizeof(instruction) * m_capacity);
    }

    instruction_array::~instruction_array() {
        if (m_instructions) m_allocator->deallocate(m_instructions);
        m_instructions = nullptr;
    }

    void instruction_array::operator += (const instruction& i) {
        if (!m_instructions) return;
        m_instructions[m_count++] = i;
        if (m_count == m_capacity) {
            m_capacity += RESIZE_AMOUNT;
            m_instructions = (instruction*)m_allocator->reallocate(m_instructions, m_capacity * sizeof(instruction));
            memset(m_instructions + m_count, 0, RESIZE_AMOUNT * sizeof(instruction));
        }
    }

    void instruction_array::remove(u64 from, u64 to) {
        if (to > m_count) {
            m_count = from;
            return;
        }
        for (u64 i = from;i < to;i++) {
            u64 take = to + (i - from);
            if (take < m_count) m_instructions[i] = m_instructions[take];
            else break;
        }
        if (m_count > (to - from)) m_count -= (to - from);
        else m_count = 0;
    }

    void instruction_array::remove(u64 index) {
        for (u64 i = index;i < m_count - 1;i++) {
            m_instructions[i] = m_instructions[i + 1];
        }
        if (m_count > 0) m_count--;
    }
};
    