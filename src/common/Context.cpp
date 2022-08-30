#include <gs/common/Context.h>
#include <gs/common/FunctionRegistry.h>
#include <gs/common/TypeRegistry.h>
#include <gs/common/Module.h>
#include <gs/interfaces/IDataTypeHolder.hpp>
#include <gs/builtin/Builtin.h>

namespace gs {
    Context::Context() {
        m_types = new ffi::DataTypeRegistry(this);
        m_funcs = new ffi::FunctionRegistry(this);
        m_global = new Module(this, "global");

        AddBuiltInBindings(this);
    }

    Context::~Context() {
        if (m_global) delete m_global;
        if (m_funcs) delete m_funcs;
        if (m_types) delete m_types;
    }

    ffi::DataTypeRegistry* Context::getTypes() const {
        return m_types;
    }

    ffi::FunctionRegistry* Context::getFunctions() const {
        return m_funcs;
    }

    Module* Context::getGlobal() const {
        return m_global;
    }
};