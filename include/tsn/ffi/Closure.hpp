#include <tsn/ffi/Closure.h>
#include <tsn/ffi/Function.h>
#include <tsn/bind/calling.hpp>

namespace tsn {
    namespace ffi {
        template <typename Ret, typename ...Args>
        Object Callable<Ret, Args...>::operator()(Args&&... args) const {
            ffi::Function* target = m_ref ? m_ref->getTarget() : nullptr;
            if (!target) throw "Call to invalid closure";

            void* self = m_ref->getSelf();
            if (target->isThisCall() && !self) {
                throw "Call to object method closure without specifying object pointer";
            } else if (self && !target->isThisCall()) {
                throw "Object pointer specified to call to non-object method closure";
            }

            if (self) {
                return call_method(m_ref->getContext(), target, self, args...);
            } else {
                return call(m_ref->getContext(), target, args...);
            }
        }
    };
};