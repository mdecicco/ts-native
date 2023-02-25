#include <tsn/utils/SourceMap.h>
#include <tsn/utils/ModuleSource.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/io/Workspace.h>

#include <utils/Buffer.hpp>
#include <utils/Array.hpp>

namespace tsn {
    SourceMap::SourceMap() {
        m_fileModifiedOn = 0;
        m_curSource = nullptr;
    }
    
    SourceMap::SourceMap(u64 fileModificationTime) {
        m_fileModifiedOn = fileModificationTime;
        m_curSource = nullptr;
    }

    SourceMap::~SourceMap() {
    }

    void SourceMap::setSource(ModuleSource* source) {
        m_curSource = source;
    }

    void SourceMap::add(u32 line, u32 col, u32 span) {
        m_map.push({ m_curSource, line, col, span });
    }

    void SourceMap::remove(u32 idx) {
        m_map.remove(idx);
    }

    const SourceMap::source_range& SourceMap::get(u32 idx) const {
        return m_map[idx];
    }

    u64 SourceMap::getModificationTime() const {
        return m_fileModifiedOn;
    }

    bool SourceMap::serialize(utils::Buffer* out, Context* ctx) const {
        if (!out->write(m_fileModifiedOn)) return false;
        if (!out->write(m_map.size())) return false;
        
        for (u32 i = 0;i < m_map.size();i++) {
            u32 modId = m_map[i].src ? m_map[i].src->getMeta()->module_id : 0;
            if (!out->write(modId)) return false;
            if (!out->write(m_map[i].line)) return false;
            if (!out->write(m_map[i].col)) return false;
            if (!out->write(m_map[i].span)) return false;
        }

        return true;
    }

    bool SourceMap::deserialize(utils::Buffer* in, Context* ctx) {
        if (!in->read(m_fileModifiedOn)) return false;

        u32 len;
        if (!in->read(len)) return false;
        
        Module* mod = nullptr;
        for (u32 i = 0;i < len;i++) {
            source_range r;
            
            u32 modId;
            if (!in->read(modId)) return false;
            if (modId != 0) {
                if (!mod || modId != mod->getId()) mod = ctx->getModule(modId);
                if (!mod) return false;
                r.src = mod->getSource();
            } else r.src = nullptr;

            if (!in->read(r.line)) return false;
            if (!in->read(r.col)) return false;
            if (!in->read(r.span)) return false;
            m_map.push(r);
        }

        return true;
    }

};