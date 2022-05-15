#include <gs/common/Object.h>
#include <gs/common/DataType.h>
#include <gs/bind/calling.hpp>
#include <utils/Allocator.hpp>

namespace gs {
    Object::Object(ffi::DataType* tp) : ITypedObject(tp) {
        if (tp->getInfo().size > 0) {
            m_data = utils::Mem::alloc(tp->getInfo().size);
            m_dataRefCount = new u32(1);
        } else {
            m_data = nullptr;
            m_dataRefCount = nullptr;
        }
    }

    Object::Object(ffi::DataType* tp, void* ptr) : ITypedObject(tp) {
        m_data = nullptr;
        m_dataRefCount = nullptr;
    }

    Object::Object(const Object& o) : ITypedObject(o.getType()) {
        m_data = o.m_data;
        m_dataRefCount = o.m_dataRefCount;

        if (m_dataRefCount) (*m_dataRefCount)++;
    }

    Object::~Object() {
        if (m_dataRefCount) {
            if ((*m_dataRefCount)-- == 0) {
                destruct();
                utils::Mem::free(m_data);
                delete m_dataRefCount;
            }
        }

        m_dataRefCount = nullptr;
        m_data = nullptr;
    }

    void* Object::getPtr() const {
        return m_data;
    }

    void Object::destruct() {
        ffi::Function* dtor = m_type->getDestructor();
        if (!dtor) return;

        ffi::call_method(dtor, m_data);
    }
};