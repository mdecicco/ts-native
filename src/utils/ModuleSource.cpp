#include <tsn/utils/ModuleSource.h>
#include <utils/Array.hpp>

namespace tsn {
    ModuleSource::ModuleSource(const utils::String& code, u64 modificationTime) {
        m_modificationTime = modificationTime;
        m_code = code;

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

    ModuleSource::~ModuleSource() {

    }

    u32 ModuleSource::getLineCount() const {
        return m_lines.size();
    }

    u64 ModuleSource::getModificationTime() const {
        return m_modificationTime;
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