#include <tsn/io/Workspace.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>
#include <tsn/compiler/Output.h>
#include <tsn/interfaces/IPersistable.h>
#include <tsn/pipeline/Pipeline.h>
#include <tsn/utils/ModuleSource.h>

#include <utils/Buffer.hpp>
#include <utils/Array.hpp>

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
                m->is_persistent = true;

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

            utils::Array<std::string> paths;

            if (!in->read(count)) throw false;
            for (u32 i = 0;i < count;i++) {
                u16 plen;
                if (!in->read(plen)) throw false;

                utils::String path = in->readStr(plen);
                if (path.size() != plen) throw false;

                paths.push(path);
            }

            if (!in->read(count)) throw false;
            for (u32 i = 0;i < count;i++) {
                u32 moduleId;
                if (!in->read(moduleId)) throw false;

                u16 pathIdx;
                if (!in->read(pathIdx)) throw false;

                m_moduleIdSourcePathMap[moduleId] = paths[pathIdx];
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

        u32 count = 0;
        for (auto it : m_scripts) {
            script_metadata* m = it.second;
            if (!m->is_persistent) continue;
            count++;
        }
        out.write(count);
        
        for (auto it : m_scripts) {
            script_metadata* m = it.second;
            if (!m->is_persistent) continue;
            
            out.write((u16)m->path.length());
            out.write(m->path.c_str(), m->path.length());
            out.write(m->size);
            out.write(m->modified_on);
            out.write(m->cached_on);
            out.write(m->is_trusted);
        }

        utils::Array<std::string> paths;
        robin_hood::unordered_map<std::string, u32> pathIndices;
        robin_hood::unordered_map<u32, u16> idMap;
        for (auto it : m_moduleIdSourcePathMap) {
            auto p = pathIndices.find(it.second);
            if (p != pathIndices.end()) {
                idMap[it.first] = pathIndices[it.second];
            } else {
                paths.push(it.second);
                pathIndices[it.second] = paths.size() - 1;
                idMap[it.first] = paths.size() - 1;
            }
        }

        out.write((u32)paths.size());
        for (u32 i = 0;i < paths.size();i++) {
            out.write((u16)paths[i].size());
            out.write(paths[i].c_str(), paths[i].size());
        }

        out.write((u32)idMap.size());
        for (auto it : idMap) {
            out.write(it.first);
            out.write(it.second);
        }

        if (!out.save(m_ctx->getConfig()->supportDir + "/last_state.db")) {
            return false;
        }

        return true;
    }

    void PersistenceDatabase::destroyMetadata(const script_metadata* meta) {
        for (auto it : m_scripts) {
            if (it.second != meta) continue;

            std::filesystem::path cachePath = m_ctx->getWorkspace()->getCachePath(meta).c_str();
            if (std::filesystem::exists(cachePath)) {
                if (!std::filesystem::remove(cachePath)) {
                    // todo: warning?
                    // hanging file shouldn't cause any issues since it'll
                    // either be overwritten or never used again
                }
            }

            delete it.second;
            m_scripts.erase(it.first);
            return;
        }
    }

    script_metadata* PersistenceDatabase::getScript(const std::string& path) {
        auto it = m_scripts.find(path);
        if (it == m_scripts.end()) return nullptr;
        return it->second;
    }

    script_metadata* PersistenceDatabase::onFileDiscovered(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted) {
        script_metadata* out = new script_metadata({
            path,
            (u32)std::hash<utils::String>()(path),
            size,
            modifiedTimestamp,
            0,
            trusted,
            false,
            true
        });
        
        m_scripts[path] = out;
        return out;
    }

    void PersistenceDatabase::onFileChanged(script_metadata* script, size_t size, u64 modifiedTimestamp) {
        script->size = size;
        script->modified_on = modifiedTimestamp;
    }

    void PersistenceDatabase::mapSourcePath(u32 moduleId, const std::string& path) {
        m_moduleIdSourcePathMap[moduleId] = path;
    }

    std::string PersistenceDatabase::getSourcePath(u32 moduleId) const {
        auto it = m_moduleIdSourcePathMap.find(moduleId);
        if (it != m_moduleIdSourcePathMap.end()) return it->second;

        for (auto s : m_scripts) {
            if (s.second->module_id == moduleId) {
                return s.second->path;
            }
        }

        return "";
    }
    
    bool PersistenceDatabase::onScriptCompiled(script_metadata* script, compiler::Output* output) {
        utils::Buffer cached;
        if (!output->serialize(&cached, m_ctx)) return false;

        std::filesystem::path p = m_ctx->getWorkspace()->getCachePath(script).c_str();

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
                throw "Failed to create support directory";
            }
        }
        
        if (!std::filesystem::exists(supportDir / "cached")) {
            if (!std::filesystem::create_directory(supportDir / "cached")) {
                throw std::exception("Failed to create cache directory");
            }
        }
        
        if (!std::filesystem::exists(workspaceRoot / "trusted")) {
            if (!std::filesystem::create_directory(workspaceRoot / "trusted")) {
                throw "Failed to create trusted module directory";
            }
        }

        m_db = new PersistenceDatabase(ctx);
        m_lastScanMS = 0;

        initPersistence();
    }

    Workspace::~Workspace() {
        delete m_db;

        for (auto s : m_sources) delete s.second;
        m_sources.clear();
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

    ModuleSource* Workspace::getSource(const utils::String& path) {
        auto it = m_sources.find(path);
        if (it != m_sources.end()) return it->second;

        script_metadata* meta = m_db->getScript(path);
        if (!meta) return nullptr;

        std::filesystem::path absSourcePath = m_ctx->getConfig()->workspaceRoot;
        absSourcePath /= m_ctx->getWorkspace()->getSourcePath(meta->module_id);
        absSourcePath = std::filesystem::absolute(absSourcePath);

        if (!std::filesystem::exists(absSourcePath)) return nullptr;
        utils::Buffer* code = utils::Buffer::FromFile(absSourcePath.string());
        if (!code) return nullptr;

        ModuleSource* src = new ModuleSource(code, meta);
        delete code;

        m_sources[path] = src;

        return src;
    }
    
    utils::String Workspace::getWorkspacePath(const utils::String& path, const utils::String& fromDir) {
        using fspath = std::filesystem::path;

        utils::String pathWithExt = enforceDirSeparator(path);
        if (path.toLowerCase().firstIndexOf(".tsn") < 0) pathWithExt += ".tsn";

        fspath p;

        // Try relative to the current path first
        if (fromDir.size() > 0) {
            p = fromDir.c_str() / fspath(pathWithExt.c_str());
            if (std::filesystem::exists(p)) {
                return enforceDirSeparator(std::filesystem::relative(p, m_ctx->getConfig()->workspaceRoot).string());
            }
        }

        // Then relative to the workspace root
        p = fspath(m_ctx->getConfig()->workspaceRoot) / fspath(pathWithExt.c_str());
        if (std::filesystem::exists(p)) {
            return enforceDirSeparator(std::filesystem::relative(p, m_ctx->getConfig()->workspaceRoot).string());
        }

        // Then relative to the trusted folder
        p = fspath(m_ctx->getConfig()->workspaceRoot) / "trusted" / fspath(pathWithExt.c_str());
        if (std::filesystem::exists(p)) {
            return enforceDirSeparator(std::filesystem::relative(p, m_ctx->getConfig()->workspaceRoot).string());
        }

        return utils::String();
    }

    Module* Workspace::getModule(const utils::String& path, const utils::String& fromDir) {
        using fspath = std::filesystem::path;

        utils::String wsPath = getWorkspacePath(path, fromDir);

        if (wsPath.size() > 0) return loadModule(wsPath);

        // Maybe it's not a real path but was synthesized by the host
        script_metadata* meta = m_db->getScript(enforceDirSeparator(path));
        if (meta) {
            // Oh?
            Module* mod = loadCached(meta);
            if (mod) return mod;
        }

        return nullptr;
    }
    
    script_metadata* Workspace::createMeta(const utils::String& path, size_t size, u64 modifiedTimestamp, bool trusted, bool persistent) {
        script_metadata* meta = m_db->getScript(path);
        if (meta) return meta;

        meta = m_db->onFileDiscovered(path, size, modifiedTimestamp, trusted);
        meta->is_persistent = persistent;
        return meta;
    }
    
    PersistenceDatabase* Workspace::getPersistor() const {
        return m_db;
    }

    utils::String Workspace::getCachePath(const script_metadata* script) const {
        return utils::String::Format(
            "%s/cached/%u.tsnc",
            m_ctx->getConfig()->supportDir.c_str(),
            script->module_id
        );
    }

    void Workspace::mapSourcePath(u32 moduleId, const std::string& path) {
        m_db->mapSourcePath(moduleId, path);
    }

    std::string Workspace::getSourcePath(u32 moduleId) const {
        return m_db->getSourcePath(moduleId);
    }

    Module* Workspace::loadModule(const std::string& path) {
        script_metadata* meta = m_db->getScript(path);

        if (meta) {
            // find out if it's cached, and if the cache is still valid
            u64 lastModifiedOn = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::filesystem::last_write_time(m_ctx->getConfig()->workspaceRoot + '/' + path).time_since_epoch()
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
        utils::String cachePath = getCachePath(script);

        if (!std::filesystem::exists(getCachePath(script).c_str())) return nullptr;

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