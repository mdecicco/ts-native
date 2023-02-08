#include <tsn/utils/ProgramSource.h>
#include <utils/Array.hpp>

namespace tsn {
    ProgramSource::ProgramSource(const utils::String& sourceName, const utils::String& rawCode) {
        m_sourceName = sourceName;
        m_rawCode = rawCode;

        u32 curLineBegin = 0;
        u32 curLineLen = 0;
        for (u32 i = 0;i < rawCode.size();i++) {
            const char& c = m_rawCode[i];
            const char* n = i < m_rawCode.size() - 1 ? &m_rawCode[i + 1] : nullptr;
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
                    m_rawCode.c_str() + curLineBegin,
                    curLineLen
                ));
                curLineBegin = i + 1;
                curLineLen = 0;
            } else curLineLen++;
        }

        if (curLineLen) {
            m_lines.push(utils::String::View(
                m_rawCode.c_str() + curLineBegin,
                curLineLen
            ));
        }
    }

    ProgramSource::~ProgramSource() {

    }

    u32 ProgramSource::getLineCount() const {
        return m_lines.size();
    }

    const utils::String& ProgramSource::getSourceName() const {
        return m_sourceName;
    }

    const utils::String& ProgramSource::getRawCode() const {
        return m_rawCode;
    }

    const utils::Array<utils::String>& ProgramSource::getLines() const {
        return m_lines;
    }

    const utils::String& ProgramSource::getLine(u32 idx) const {
        return m_lines[idx];
    }
    
    const utils::Array<func_range>& ProgramSource::getFunctionInfo() const {
        return m_funcRanges;
    }
};