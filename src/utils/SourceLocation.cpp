#include <gs/utils/SourceLocation.h>
#include <gs/utils/ProgramSource.h>

namespace gs {
    SourceLocation::SourceLocation(ProgramSource* src, u32 line, u32 col) {
        m_ref = src;
        m_line = line;
        m_col = col;

        if (src->getLineCount() <= line) {
            m_lineLen = 0;
            m_linePtr = nullptr;
        } else {
            const utils::String& ln = src->getLine(line);
            m_lineLen = ln.size();
            m_linePtr = ln.c_str();
        }
    }

    SourceLocation::~SourceLocation() {
    }

    ProgramSource* SourceLocation::getSource() const {
        return m_ref;
    }

    utils::String SourceLocation::getLineText() const {
        return utils::String::View(m_linePtr, m_lineLen);
    }

    const char* SourceLocation::getPointer() const {
        return m_linePtr;
    }

    u32 SourceLocation::getLine() const {
        return m_line;
    }

    u32 SourceLocation::getCol() const {
        return m_col;
    }

    bool SourceLocation::isValid() const {
        return m_col < m_lineLen && m_line < m_ref->getLineCount();
    }

    bool SourceLocation::operator++(int) {
        if (m_col < m_lineLen - 1) {
            m_col++;
            m_linePtr++;
            return true;
        } else if (m_line < m_ref->getLineCount() - 1) {
            m_col = 0;
            m_line++;
            const utils::String& ln = m_ref->getLine(m_line);
            m_lineLen = ln.size();
            m_linePtr = ln.c_str();
            return true;
        }

        return false;
    }

    char SourceLocation::operator*() const {
        return m_linePtr[m_col];
    }



    //
    // SourceException
    //

    SourceException::SourceException(const utils::String& message, const SourceLocation& src) : m_message(message), m_src(src) {
    }

    SourceException::~SourceException() {

    }

    char const* SourceException::what() const {
        return m_message.c_str();
    }

    const SourceLocation& SourceException::getSource() const {
        return m_src;
    }
};