#include <tsn/io/Workspace.h>
#include <tsn/common/Context.h>
#include <tsn/common/Config.h>

#include <utils/Buffer.hpp>

#include <filesystem>
#include <algorithm>

namespace tsn {
    constexpr u32 PTSN_MAGIC = 0x4E535450; /* Hex for "PTSN" */

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
                if (!in->read(m->is_trusted)) throw false;

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
        out.write(m_ctx->getConfig()->workspaceRoot);
        out.write((u32)m_scripts.size());
        
        for (auto it : m_scripts) {
            script_metadata* m = it.second;
            out.write((u16)m->path.length());
            out.write(m->path);
            out.write(m->size);
            out.write(m->modified_on);
            out.write(m->is_trusted);
        }

        if (!out.save(m_ctx->getConfig()->supportDir + "/last_state.db")) {
            return false;
        }

        return true;
    }

    PersistenceDatabase::script_metadata* PersistenceDatabase::getScript(const std::string& path) {
        auto it = m_scripts.find(path);
        if (it == m_scripts.end()) return nullptr;
        return it->second;
    }

    void PersistenceDatabase::onFileDiscovered(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted) {
        m_scripts[path] = new script_metadata({
            path,
            size,
            modifiedTimestamp,
            trusted
        });
    }

    void PersistenceDatabase::onFileChanged(script_metadata* script, size_t size, u64 modifiedTimestamp) {
        script->size = size;
        script->modified_on = modifiedTimestamp;
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
        
        return nullptr;
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
                    std::filesystem::relative(entry.path(), root).string(),
                    entry.file_size(),
                    std::chrono::duration_cast<std::chrono::milliseconds>(entry.last_write_time().time_since_epoch()).count(),
                    trusted
                );
            }
        }
    }

    void Workspace::processScript(const std::string& path, size_t size, u64 modifiedTimestamp, bool trusted) {
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