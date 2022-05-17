#include <gs/utils/SourceLocation.h>
#include <gs/utils/ProgramSource.h>

namespace gs {
    SourceLocation::SourceLocation(ProgramSource* src, u32 line, u16 col) {
        m_ref = src;
        m_line = line;
        m_col = col;
    }

    SourceLocation::~SourceLocation() {
    }

    const utils::String& SourceLocation::getLineText() const {
        return m_ref->getLine(m_line);
    }

    u32 SourceLocation::getLine() const {
        return m_line;
    }

    u16 SourceLocation::getCol() const {
        return m_col;
    }
};