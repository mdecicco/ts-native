#include <tsn/ffi/Closure.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/bind/calling.hpp>
#include <tsn/common/Context.h>
#include <tsn/builtin/Builtin.h>

#include <utils/Allocator.hpp>

namespace tsn {
    void freeClosure(ffi::Closure* closure);
    void freeCaptureData(void* data);

    namespace ffi {
        ClosureRef::ClosureRef() {
            m_ref = nullptr;
        }

        ClosureRef::ClosureRef(const ClosureRef& closure) {
            m_captureData = closure.m_captureData;
            m_ref = closure.m_ref;
            if (m_ref) m_ref->m_refCount++;
        }

        ClosureRef::ClosureRef(Closure* closure) {
            m_captureData = closure ? closure->m_captureData : nullptr;
            m_ref = closure;
            if (m_ref) m_ref->m_refCount++;
        }

        ClosureRef::~ClosureRef() {
            if (m_ref) {
                m_ref->m_refCount--;
                if (m_ref->m_refCount == 0) freeClosure(m_ref);
                m_ref = nullptr;
            }
        }
        
        ffi::Function* ClosureRef::getTarget() const {
            if (!m_ref) return nullptr;
            return m_ref->m_target;
        }

        void* ClosureRef::getSelf() const {
            if (!m_ref) return nullptr;
            return m_ref->m_self;
        }

        void ClosureRef::operator=(const ClosureRef& rhs) {
            m_ref = rhs.m_ref;
            if (m_ref) m_ref->m_refCount++;
        }

        void ClosureRef::operator=(Closure* rhs) {
            m_ref = rhs;
            if (m_ref) m_ref->m_refCount++;
        }



        Closure::Closure(ExecutionContext* ectx, function_id targetId, void* captureData)
            : IContextual(ectx->getContext())
        {
            m_self = nullptr;
            m_target = m_ctx->getFunctions()->getFunction(targetId);
            m_captureData = captureData;
            m_refCount = 0;
        }

        Closure::Closure(const Closure& c) : IContextual(nullptr) {
            throw std::exception("Closures are not copy constructible");
        }

        Closure::~Closure() {
            if (m_captureData) {
                u8* data = (u8*)m_captureData;
                u32 count = *(u32*)data;
                data += sizeof(u32);

                for (u32 i = 0;i < count;i++) {
                    type_id tid = *(type_id*)data;
                    data += sizeof(type_id);

                    DataType* tp = m_ctx->getTypes()->getType(tid);
                    Function* dtor = tp->getDestructor();
                    if (dtor) call_method(m_ctx, dtor, data);
                    data += tp->getInfo().size;
                }

                freeCaptureData(m_captureData);
                m_captureData = nullptr;
            }
        }
        
        void Closure::bind(void* self) {
            m_self = self;
        }

        ffi::Function* Closure::getTarget() const {
            return m_target;
        }

        void* Closure::getSelf() const {
            return m_self;
        }
    };
};