#include <tsn/common/Context.h>
#include <tsn/common/FunctionRegistry.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/Module.h>
#include <tsn/common/Config.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/builtin/Builtin.h>
#include <tsn/io/Workspace.h>

namespace tsn {
    Context::Context(u32 apiVersion, Config* cfg) {
        m_builtinApiVersion = 1;
        m_userApiVersion = apiVersion;

        if (cfg) m_cfg = new Config(*cfg);
        else m_cfg = new Config();

        m_workspace = new Workspace(this);
        m_types = new ffi::DataTypeRegistry(this);
        m_funcs = new ffi::FunctionRegistry(this);
        m_global = new Module(this, "global");

        AddBuiltInBindings(this);
    }

    Context::~Context() {
        if (m_global) delete m_global;
        if (m_funcs) delete m_funcs;
        if (m_types) delete m_types;
        if (m_workspace) delete m_workspace;
        if (m_cfg) delete m_cfg;
    }

    u32 Context::getBuiltinApiVersion() const {
        return m_builtinApiVersion;
    }

    u32 Context::getExtendedApiVersion() const {
        return m_userApiVersion;
    }

    const Config* Context::getConfig() const {
        return m_cfg;
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
    
    Module* Context::createModule(const utils::String& name) {
        if (m_modules.count(name) != 0) {
            return nullptr;
        }

        Module* m = new Module(this, name);
        m_modules[name] = m;
        return m;
    }

    Module* Context::getModule(const utils::String& name) {
        // todo:
        //     workspace directory, auto-load modules

        auto it = m_modules.find(name);
        if (it == m_modules.end()) return nullptr;
        return it->second;
    }
};