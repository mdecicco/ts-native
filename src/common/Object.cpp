#include <tsn/common/Object.hpp>
#include <tsn/common/DataType.h>
#include <tsn/bind/calling.hpp>


#include <utils/Allocator.hpp>
#include <utils/Array.hpp>

namespace tsn {
    Object::Object(Context* ctx, ffi::DataType* tp) : ITypedObject(tp), IContextual(ctx) {
        if (tp->getInfo().size > 0) {
            m_data = utils::Mem::alloc(tp->getInfo().size);
            m_dataRefCount = new u32(1);
        } else {
            m_data = nullptr;
            m_dataRefCount = nullptr;
        }
    }

    Object::Object(Context* ctx, bool takeOwnership, ffi::DataType* tp, void* ptr) : ITypedObject(tp), IContextual(ctx) {
        m_data = ptr;

        if (takeOwnership) {
            m_dataRefCount = new u32(1);
        } else m_dataRefCount = nullptr;
    }

    Object::Object(const Object& o) : ITypedObject(o.getType()), IContextual(o.getContext()) {
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
    
    const Object Object::prop(const utils::String& propName) const {
        const auto& props = m_type->getProperties();
        i64 idx = props.findIndex([&propName](const ffi::type_property& p) { return p.name == propName; });
        if (idx < 0) {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has no property named '%s' bound",
                m_type->getName().c_str(), propName.c_str()
            ));
        }

        const auto& p = props[(u32)idx];

        if (p.access == private_access) {
            throw ffi::BindException(utils::String::Format(
                "Cannot read property '%s' of object of type '%s'. The property is private",
                propName.c_str(), m_type->getName().c_str()
            ));
        }

        if (!p.flags.can_read) {
            throw ffi::BindException(utils::String::Format(
                "Cannot read property '%s' of object of type '%s'. The property is not readable",
                propName.c_str(), m_type->getName().c_str()
            ));
        }

        if (p.getter) {
            if (p.flags.is_static) {
                return ffi::call(m_ctx, p.getter);
            } else {
                return ffi::call_method(m_ctx, p.getter, m_data);
            }
        }

        return Object(m_ctx, false, p.type, (void*)((u8*)m_data + p.offset));
    }

    Object Object::prop(const utils::String& propName) {
        const auto& props = m_type->getProperties();
        i64 idx = props.findIndex([&propName](const ffi::type_property& p) { return p.name == propName; });
        if (idx < 0) {
            throw ffi::BindException(utils::String::Format(
                "Object of type '%s' has no property named '%s' bound",
                m_type->getName().c_str(), propName.c_str()
            ));
        }

        const auto& p = props[(u32)idx];
        return Object(m_ctx, false, p.type, (void*)((u8*)m_data + p.offset));
    }

    void* Object::getPtr() const {
        return m_data;
    }

    void Object::destruct() {
        ffi::Function* dtor = m_type->getDestructor();
        if (!dtor) return;

        ffi::call_method(m_ctx, dtor, m_data);
    }
};