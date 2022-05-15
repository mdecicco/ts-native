#include <gs/interfaces/ITypedObject.h>

namespace gs {
    ITypedObject::ITypedObject(ffi::DataType* type) {
        m_type = type;
    }

    ITypedObject::~ITypedObject() { }

    ffi::DataType* ITypedObject::getType() const {
        return m_type;
    }
};