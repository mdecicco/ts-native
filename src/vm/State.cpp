#include <tsn/vm/State.h>
#include <utils/Allocator.hpp>

namespace tsn {
    namespace vm {
        State::State(u32 stackSize) {
            stackBase = (u8*)utils::Mem::alloc(stackSize);
            memset(stackBase, 0, stackSize);
            memset(registers, 0, sizeof(registers));
        }

        State::~State() {
            utils::Mem::free(stackBase);
        }

        void State::push(vm_register reg) {
            if (m_storedIdx == 255) {
                // todo
                // runtime error
            }

            m_stored[m_storedIdx++] = registers[(u8)(reg)];
        }

        void State::pop(vm_register reg) {
            if (m_storedIdx == 0) {
                // todo
                // runtime error
            }

            registers[(u8)(reg)] = m_stored[--m_storedIdx];
        }
    };
};