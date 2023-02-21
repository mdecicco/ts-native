#include <tsn/utils/ModuleSource.h>
#include <utils/Array.hpp>
#include <utils/Buffer.h>

namespace tsn {
    ModuleSource::ModuleSource(const utils::String& code, const script_metadata* meta) {
        m_meta = meta;
        m_code = code;
        init();
    }

    ModuleSource::ModuleSource(const utils::Buffer* code, const script_metadata* meta) : m_code((char*)code->data(0), u32(code->size())) {
        m_meta = meta;
        init();
    }

    ModuleSource::~ModuleSource() {

    }

    void ModuleSource::init() {
        u32 curLineBegin = 0;
        u32 curLineLen = 0;
        for (u32 i = 0;i < m_code.size();i++) {
            const char& c = m_code[i];
            const char* n = i < m_code.size() - 1 ? &m_code[i + 1] : nullptr;
            bool endLine = false;

            if ((c == '\n' && n && *n == '\r') || (c == '\r' && n && *n == '\n')) {
                i++;
                curLineLen++;
                endLine = true;
            } else if (c == '\n' || c == '\r') {
                endLine = true;
            } 
            
            if (endLine) {
                curLineLen++;
                m_lines.push(utils::String::View(
                    m_code.c_str() + curLineBegin,
                    curLineLen
                ));
                curLineBegin = i + 1;
                curLineLen = 0;
            } else curLineLen++;
        }

        if (curLineLen) {
            m_lines.push(utils::String::View(
                m_code.c_str() + curLineBegin,
                curLineLen
            ));
        }
    }

    u32 ModuleSource::getLineCount() const {
        return m_lines.size();
    }

    const script_metadata* ModuleSource::getMeta() const {
        return m_meta;
    }

    const utils::String& ModuleSource::getCode() const {
        return m_code;
    }

    const utils::Array<utils::String>& ModuleSource::getLines() const {
        return m_lines;
    }

    const utils::String& ModuleSource::getLine(u32 idx) const {
        return m_lines[idx];
    }
};