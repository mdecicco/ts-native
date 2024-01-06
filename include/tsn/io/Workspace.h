#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <utils/robin_hood.h>
#include <utils/String.h>
#include <string>

namespace tsn {
    namespace compiler {
        class Output;
    };

    class Module;

    struct script_metadata {
        std::string path;
        u32 module_id;
        size_t size;
        u64 modified_on;
        u64 cached_on;
        bool is_trusted;
        bool is_external;
    };

    void enforceDirSeparator(std::string& path);
    void enforceDirSeparator(utils::String& path);
    std::string enforceDirSeparator(const std::string& _path);
    utils::String enforceDirSeparator(const utils::String& _path);

    class PersistenceDatabase : public IContextual {
        public:
            PersistenceDatabase(Context* ctx);
            ~PersistenceDatabase();

            bool restore();
            bool persist();

            script_metadata* getScript(const std::string& path);
            script_metadata* onFileDiscovered(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted);
            void onFileChanged(script_metadata* script, size_t size, u64 modifiedTimestamp);

            /** Returns true if the compilation output was cached */
            bool onScriptCompiled(script_metadata* script, compiler::Output* output);
        
        protected:
            robin_hood::unordered_map<std::string, script_metadata*> m_scripts;
    };

    class Workspace : public IContextual {
        public:
            Workspace(Context* ctx);
            ~Workspace();

            void service();

            Module* getModule(const utils::String& path, const utils::String& fromDir = utils::String());
            script_metadata* createMeta(const utils::String& path, size_t size, u64 modifiedTimestamp, bool trusted);
            PersistenceDatabase* getPersistor() const;

        protected:
            Module* loadModule(const std::string& path);
            Module* loadCached(script_metadata* script);
            void scanDirectory(const std::string& dir, bool trusted);
            void processScript(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted);
            void initPersistence();

            PersistenceDatabase* m_db;
            u64 m_lastScanMS;
    };
};