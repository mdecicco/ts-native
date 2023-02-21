#include <tsn/interfaces/ITypedObject.h>

namespace tsn {
    ITypedObject::ITypedObject(ffi::DataType* type) {
        m_type = type;
    }

    ITypedObject::~ITypedObject() { }

    ffi::DataType* ITypedObject::getType() const {
        return m_type;
    }
};