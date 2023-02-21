#pragma once
#include <tsn/common/types.h>
#include <utils/robin_hood.h>
#include <utils/String.h>

namespace tsn {
    class Module;
    class Config;
    class Workspace;
    class Pipeline;
    struct script_metadata;

    namespace ffi {
        class DataTypeRegistry;
        class FunctionRegistry;
    };

    namespace backend {
        class IBackend;
    };

    class Context {
        public:
            /**
             * @param apiVersion Arbitrary version number that corresponds to the interface
             *                   you bind for scripts. This is used only to determine whether
             *                   or not cached modules need to be recompiled. If set to 0 then
             *                   caching is disabled and scripts are always recompiled.
             *                         
             * @param cfg        Optional script context configuration data. If null, default
             *                   configuration will be used.
             * 
             * @note If the user-defined API changes, but the apiVersion is not changed, cached
             *       scripts may exhibit undefined behavior.
             */
            Context(u32 apiVersion = 0, Config* cfg = nullptr);
            ~Context();

            /** Gets the version number of the default script API */
            u32 getBuiltinApiVersion() const;

            /** Gets the version number of the user-defined script API */
            u32 getExtendedApiVersion() const;

            const Config* getConfig() const;
            ffi::DataTypeRegistry* getTypes() const;
            ffi::FunctionRegistry* getFunctions() const;
            Workspace* getWorkspace() const;
            Pipeline* getPipeline() const;
            backend::IBackend* getBackend() const;

            Module* getGlobal() const;
            Module* createModule(const utils::String& name, const utils::String& path, const script_metadata* meta);
            Module* createHostModule(const utils::String& name);
            Module* getModule(const utils::String& path, const utils::String& fromDir = utils::String());
            Module* getModule(u32 moduleId);

        protected:
            Workspace* m_workspace;
            Pipeline* m_pipeline;

            ffi::DataTypeRegistry* m_types;
            ffi::FunctionRegistry* m_funcs;
            backend::IBackend* m_backend;
            Module* m_global;
            Config* m_cfg;
            robin_hood::unordered_map<utils::String, Module*> m_modules;
            robin_hood::unordered_map<u32, Module*> m_modulesById;
            u32 m_builtinApiVersion;
            u32 m_userApiVersion;
    };
};