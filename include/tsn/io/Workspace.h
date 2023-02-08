#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IContextual.h>
#include <utils/robin_hood.h>
#include <utils/String.h>
#include <string>

namespace tsn {
    class Module;

    class PersistenceDatabase : public IContextual {
        public:
            struct script_metadata {
                std::string path;
                size_t size;
                u64 modified_on;
                bool is_trusted;
            };

            PersistenceDatabase(Context* ctx);
            ~PersistenceDatabase();

            bool restore();
            bool persist();

            script_metadata* getScript(const std::string& path);
            void onFileDiscovered(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted);
            void onFileChanged(script_metadata* script, size_t size, u64 modifiedTimestamp);
        
        protected:
            robin_hood::unordered_map<std::string, script_metadata*> m_scripts;
    };

    class Workspace : public IContextual {
        public:
            Workspace(Context* ctx);
            ~Workspace();

            void service();

            Module* getModule(const utils::String& path, const utils::String& fromDir = utils::String());

        protected:
            void scanDirectory(const std::string& dir, bool trusted);
            void processScript(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted);
            void initPersistence();

            PersistenceDatabase* m_db;
            u64 m_lastScanMS;
    };
};