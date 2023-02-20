#include <tsn/io/Workspace.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/compiler/Output.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/pipeline/Pipeline.h>

#include <utils/Buffer.hpp>

#include <filesystem>
#include <algorithm>

namespace tsn {
    constexpr u32 PTSN_MAGIC = 0x4E535450; /* Hex for "PTSN" */

    void enforceDirSeparator(std::string& path) {
        size_t pos = 0;
        while ((pos = path.find_first_of('\\')) != std::string::npos) {
            path[pos] = '/';
        }
    }

    void enforceDirSeparator(utils::String& path) {
        path.replaceAll("\\", "/");
    }

    std::string enforceDirSeparator(const std::string& _path) {
        std::string path = _path;

        size_t pos = 0;
        while ((pos = path.find_first_of('\\')) != std::string::npos) {
            path[pos] = '/';
        }

        return path;
    }

    utils::String enforceDirSeparator(const utils::String& _path) {
        utils::String path = _path;

        path.replaceAll("\\", "/");

        return path;
    }

    //
    // PersistenceDatabase
    //
    PersistenceDatabase::PersistenceDatabase(Context* ctx) : IContextual(ctx) {
    }

    PersistenceDatabase::~PersistenceDatabase() {
        persist();

        for (auto it : m_scripts) delete it.second;
    }

    bool PersistenceDatabase::restore() {
        if (m_ctx->getExtendedApiVersion() == 0) return false;

        script_metadata* m = nullptr;
        utils::Buffer* in = utils::Buffer::FromFile(m_ctx->getConfig()->supportDir + "/last_state.db");
        if (!in) return false;

        try {
            u32 magic = 0;
            if (!in->read(magic)) throw false;
            if (magic != PTSN_MAGIC) throw false;

            u32 ver = 0;
            if (!in->read(ver) || ver != m_ctx->getBuiltinApiVersion()) throw false;
            if (!in->read(ver) || ver != m_ctx->getExtendedApiVersion()) throw false;

            u16 wslen = 0;
            if (!in->read(wslen)) throw false;

            utils::String prevWorkspaceDir = in->readStr(wslen);
            if (prevWorkspaceDir != m_ctx->getConfig()->workspaceRoot) {
                // All paths are now invalid, exit early
                throw true;
            }

            u32 count = 0;
            if (!in->read(count)) throw false;

            for (u32 i = 0;i < count;i++) {
                m = new script_metadata();

                u16 plen = 0;
                if (!in->read(plen)) throw false;

                m->path = in->readStr(plen);
                if (m->path.size() != plen) throw false;

                if (!in->read(m->size)) throw false;
                if (!in->read(m->modified_on)) throw false;
                if (!in->read(m->cached_on)) throw false;
                if (!in->read(m->is_trusted)) throw false;
                m->is_external = false;

                m->module_id = (u32)std::hash<utils::String>()(m->path);

                m_scripts[m->path] = m;
            }
        } catch (bool success) {
            if (!success) {
                if (in) delete in;
                if (m) delete m;
                for (auto it : m_scripts) delete it.second;
                m_scripts.clear();
                return false;
            }
        }
            
        delete in;
        return true;
    }

    bool PersistenceDatabase::persist() {
        if (m_ctx->getExtendedApiVersion() == 0) return false;

        utils::Buffer out;
        out.write(PTSN_MAGIC);
        out.write(m_ctx->getBuiltinApiVersion());
        out.write(m_ctx->getExtendedApiVersion());
        out.write((u16)m_ctx->getConfig()->workspaceRoot.length());
        out.write(m_ctx->getConfig()->workspaceRoot.c_str(), m_ctx->getConfig()->workspaceRoot.length());
        out.write((u32)m_scripts.size());
        
        for (auto it : m_scripts) {
            script_metadata* m = it.second;
            out.write((u16)m->path.length());
            out.write(m->path.c_str(), m->path.length());
            out.write(m->size);
            out.write(m->modified_on);
            out.write(m->cached_on);
            out.write(m->is_trusted);
        }

        if (!out.save(m_ctx->getConfig()->supportDir + "/last_state.db")) {
            return false;
        }

        return true;
    }

    script_metadata* PersistenceDatabase::getScript(const std::string& path) {
        auto it = m_scripts.find(path);
        if (it == m_scripts.end()) return nullptr;
        return it->second;
    }

    void PersistenceDatabase::onFileDiscovered(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted) {
        m_scripts[path] = new script_metadata({
            path,
            (u32)std::hash<utils::String>()(path),
            size,
            modifiedTimestamp,
            0,
            trusted,
            false
        });
    }

    void PersistenceDatabase::onFileChanged(script_metadata* script, size_t size, u64 modifiedTimestamp) {
        script->size = size;
        script->modified_on = modifiedTimestamp;
    }
    
    bool PersistenceDatabase::onScriptCompiled(script_metadata* script, compiler::Output* output) {
        utils::Buffer cached;
        if (!output->serialize(&cached, m_ctx)) return false;

        std::filesystem::path p = m_ctx->getConfig()->supportDir;
        p /= utils::String::Format("%u.tsnc", script->module_id).c_str();

        if (!cached.save(p.string())) return false;

        script->cached_on = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::filesystem::last_write_time(p).time_since_epoch()
        ).count();

        return true;
    }


    //
    // Workspace
    //
    Workspace::Workspace(Context* ctx) : IContextual(ctx) {
        std::filesystem::path supportDir = ctx->getConfig()->supportDir;
        std::filesystem::path workspaceRoot = ctx->getConfig()->workspaceRoot;

        if (!std::filesystem::exists(supportDir)) {
            if (!std::filesystem::create_directory(supportDir)) {
                throw std::exception("Failed to create support directory");
            }
        }
        
        if (!std::filesystem::exists(workspaceRoot / "trusted")) {
            if (!std::filesystem::create_directory(workspaceRoot / "trusted")) {
                throw std::exception("Failed to create trusted module directory");
            }
        }

        m_db = new PersistenceDatabase(ctx);
        m_lastScanMS = 0;

        initPersistence();
    }

    Workspace::~Workspace() {
        delete m_db;
    }

    void Workspace::service() {
        if (!m_ctx->getConfig()->scanForChanges && m_lastScanMS > 0) return;

        u64 currentMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        u64 delta = currentMS - m_lastScanMS;

        if (delta > m_ctx->getConfig()->scanIntervalMS) {
            m_lastScanMS = currentMS;
            if (!std::filesystem::exists(m_ctx->getConfig()->workspaceRoot)) return;
            scanDirectory(m_ctx->getConfig()->workspaceRoot, false);
        }
    }

    Module* Workspace::getModule(const utils::String& path, const utils::String& fromDir) {
        using fspath = std::filesystem::path;

        utils::String pathWithExt = path;
        if (path.toLowerCase().firstIndexOf(".tsn") < 0) pathWithExt += ".tsn";

        fspath p;

        // Try relative to the current path first
        if (fromDir.size() > 0) {
            p = fromDir.c_str() / fspath(pathWithExt.c_str());
            if (std::filesystem::exists(p)) return loadModule(p.string());
        }

        // Then relative to the workspace root
        p = fspath(m_ctx->getConfig()->workspaceRoot) / fspath(pathWithExt.c_str());
        if (std::filesystem::exists(p)) return loadModule(p.string());

        // Then relative to the trusted folder
        p = fspath(m_ctx->getConfig()->workspaceRoot) / "trusted" / fspath(pathWithExt.c_str());
        if (std::filesystem::exists(p)) return loadModule(p.string());

        return nullptr;
    }
    
    PersistenceDatabase* Workspace::getPersistor() const {
        return m_db;
    }

    Module* Workspace::loadModule(const std::string& path) {
        std::filesystem::path relpath = std::filesystem::relative(path, m_ctx->getConfig()->workspaceRoot);
        script_metadata* meta = m_db->getScript(enforceDirSeparator(relpath.string()));

        if (meta) {
            // find out if it's cached, and if the cache is still valid
            u64 lastModifiedOn = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::filesystem::last_write_time(path).time_since_epoch()
            ).count();

            if (lastModifiedOn < meta->cached_on) {
                Module* mod = loadCached(meta);
                if (mod) return mod;

                // fallback to just loading the script normally
            }
        }

        return m_ctx->getPipeline()->buildFromSource(meta);
    }
    
    Module* Workspace::loadCached(script_metadata* script) {
        utils::String cachePath = utils::String::Format(
            "%s/%u.tsnc",
            m_ctx->getConfig()->supportDir.c_str(),
            script->module_id
        );

        if (!std::filesystem::exists(cachePath.c_str())) return nullptr;

        return m_ctx->getPipeline()->buildFromCached(script);
    }

    void Workspace::initPersistence() {
        m_db->restore();

        service();
    }

    void Workspace::scanDirectory(const std::string& dir, bool trusted) {
        std::filesystem::path root = m_ctx->getConfig()->workspaceRoot;

        std::filesystem::directory_iterator list(dir);
        for (const auto& entry : list) {
            if (entry.is_directory()) {
                scanDirectory(
                    entry.path().string(),
                    trusted || entry.path().filename().string() == "trusted"
                );
            } else if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".tsn") continue;

                processScript(
                    enforceDirSeparator(std::filesystem::relative(entry.path(), root).string()),
                    entry.file_size(),
                    std::chrono::duration_cast<std::chrono::milliseconds>(entry.last_write_time().time_since_epoch()).count(),
                    trusted
                );
            }
        }
    }

    void Workspace::processScript(const std::string& _path, size_t size, u64 modifiedTimestamp, bool trusted) {
        std::string path = enforceDirSeparator(_path);
        
        auto script = m_db->getScript(path);
        if (!script) {
            m_db->onFileDiscovered(path, size, modifiedTimestamp, trusted);
            return;
        }

        if (script->modified_on < modifiedTimestamp) {
            m_db->onFileChanged(script, size, modifiedTimestamp);
        }
    }
}