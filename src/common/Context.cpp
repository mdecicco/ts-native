#include <gs/common/Context.h>
#include <gs/common/FunctionRegistry.h>
#include <gs/common/TypeRegistry.h>
#include <gs/interfaces/IDataTypeHolder.hpp>

namespace gs {
    Context::Context() {
        m_types = new ffi::DataTypeRegistry(this);
        m_funcs = new ffi::FunctionRegistry(this);
    }

    Context::~Context() {
        if (m_types) delete m_types;
        if (m_funcs) delete m_funcs;
    }

    ffi::DataTypeRegistry* Context::getTypes() const {
        return m_types;
    }

    ffi::FunctionRegistry* Context::getFunctions() const {
        return m_funcs;
    }
};