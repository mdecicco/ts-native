#include <tsn/ffi/Closure.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/bind/calling.hpp>
#include <tsn/common/Context.h>
#include <tsn/builtin/Builtin.h>

#include <utils/Allocator.hpp>

namespace tsn {
    namespace ffi {
        ClosureRef::ClosureRef() {
            m_ref = nullptr;
        }

        ClosureRef::ClosureRef(const ClosureRef& closure) {
            m_ref = closure.m_ref;
            if (m_ref) m_ref->m_refCount++;
        }

        ClosureRef::ClosureRef(Closure* closure) {
            m_ref = closure;
            if (m_ref) m_ref->m_refCount++;
        }

        ClosureRef::~ClosureRef() {
            if (m_ref) {
                m_ref->m_refCount--;
                if (m_ref->m_refCount == 0) delete m_ref;
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



        Closure::Closure(ExecutionContext* ectx, function_id targetId, void* captures, void* captureTypeIds, u32 captureCount)
            : IContextual(ectx->getContext())
        {
            m_self = nullptr;
            m_target = m_ctx->getFunctions()->getFunction(targetId);
            m_captureData = captures;
            m_captureDataTypeIds = (u32*)captureTypeIds;
            m_captureDataCount = captureCount;
            m_refCount = 0;
        }

        Closure::~Closure() {
            if (m_captureData) {
                void* objPtr = m_captureData;
                for (u32 i = 0;i < m_captureDataCount;i++) {
                    DataType* tp = m_ctx->getTypes()->getType(m_captureDataTypeIds[i]);
                    Function* dtor = tp->getDestructor();
                    if (dtor) call_method(m_ctx, dtor, objPtr);
                    objPtr = ((u8*)objPtr) + tp->getInfo().size;
                }

                freeMem(m_captureData);
                freeMem(m_captureDataTypeIds);
                m_captureData = nullptr;
                m_captureDataTypeIds = nullptr;
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