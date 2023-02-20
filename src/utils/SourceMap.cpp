#include <tsn/utils/SourceMap.h>
#include <utils/Buffer.hpp>
#include <utils/Array.hpp>

namespace tsn {
    SourceMap::SourceMap() {
        m_fileModifiedOn = 0;
    }
    
    SourceMap::SourceMap(u64 fileModificationTime) {
        m_fileModifiedOn = fileModificationTime;
    }

    SourceMap::~SourceMap() {
    }

    void SourceMap::add(u32 line, u32 col, u32 span) {
        m_map.push({ line, col, span });
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
        
        for (u32 i = 0;i < len;i++) {
            source_range r;
            if (!in->read(r.line)) return false;
            if (!in->read(r.col)) return false;
            if (!in->read(r.span)) return false;
            m_map.push(r);
        }

        return true;
    }

};