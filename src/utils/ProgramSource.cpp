#include <gs/utils/ProgramSource.h>

namespace gs {
    ProgramSource::ProgramSource(const utils::String& sourceName, const utils::String& rawCode) {
        m_sourceName = source;
        m_rawCode = rawCode;
        
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
};