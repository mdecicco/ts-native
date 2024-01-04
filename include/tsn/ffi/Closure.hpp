#include <tsn/ffi/Closure.h>
#include <tsn/ffi/Function.h>

namespace tsn {
    namespace ffi {
        template <typename ...Args>
        Object Closure::call(Args&&... args) {
            if (!m_ref || !m_ref->m_target) throw std::exception("Call to invalid closure");
            if (m_ref->m_target->isThisCall() && !m_ref->m_self) {
                throw std::exception("Call to object method closure without specifying object pointer");
            }

            if (m_ref->m_self && !m_ref->m_target->isThisCall()) {
                throw std::exception("Object pointer specified to call to non-object method closure");
            }

            if (m_self) {
                return call_method(m_ref->getContext(), m_ref->m_target, m_ref->m_self, args...);
            } else {
                return call(m_ref->getContext(), m_ref->m_target, args...);
            }
        }
    };
};