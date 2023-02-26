#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/common/Config.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/builtin/Builtin.h>
#include <tsn/io/Workspace.h>

#include <tsn/vm/VMBackend.h>

namespace tsn {
    Context::Context(u32 apiVersion, Config* cfg) {
        m_builtinApiVersion = 2;
        m_userApiVersion = apiVersion;

        if (cfg) m_cfg = new Config(*cfg);
        else m_cfg = new Config();

        m_backend = new vm::Backend(this, 16384);
        m_pipeline = new Pipeline(this, nullptr, nullptr);
        m_workspace = new Workspace(this);
        m_types = new ffi::DataTypeRegistry(this);
        m_funcs = new ffi::FunctionRegistry(this);
        m_global = createHostModule("global");

        AddBuiltInBindings(this);

        // Must be called _AFTER_ builtin bindings
        m_types->updateCachedTypes();
    }

    Context::~Context() {
        if (m_global) delete m_global;
        if (m_funcs) delete m_funcs;
        if (m_types) delete m_types;
        if (m_workspace) delete m_workspace;
        if (m_pipeline) delete m_pipeline;
        if (m_backend) delete m_backend;
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

    Workspace* Context::getWorkspace() const {
        return m_workspace;
    }

    Pipeline* Context::getPipeline() const {
        return m_pipeline;
    }

    backend::IBackend* Context::getBackend() const {
        return m_backend;
    }

    Module* Context::getGlobal() const {
        return m_global;
    }

    Module* Context::createModule(const utils::String& name, const utils::String& _path, const script_metadata* meta) {
        utils::String path = _path;
        enforceDirSeparator(path);

        if (m_modules.count(path) != 0) {
            return nullptr;
        }

        Module* m = new Module(this, name, path, meta);
        m_modules[path] = m;
        m_modulesById[m->getId()] = m;
        return m;
    }
    
    Module* Context::createHostModule(const utils::String& name) {
        return createModule(name, "<host>/" + name + ".tsn", nullptr);
    }

    Module* Context::getModule(const utils::String& _path, const utils::String& fromDir) {
        utils::String path = _path;
        enforceDirSeparator(path);
        
        auto it = m_modules.find(path);
        if (it == m_modules.end()) {
            // Maybe it's a host module
            it = m_modules.find("<host>/" + path + ".tsn");
            if (it == m_modules.end()) {
                Module* m = m_workspace->getModule(path, fromDir);
                if (m) {
                    m_modules[path] = m;
                    m_modulesById[m->getId()] = m;
                }

                return m;
            }
            return it->second;
        }

        return it->second;
    }

    Module* Context::getModule(u32 id) {
        auto it = m_modulesById.find(id);
        if (it == m_modulesById.end()) return nullptr;
        return it->second;
    }
};