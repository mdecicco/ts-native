#include <gs/utils/SourceLocation.h>
#include <gs/utils/ProgramSource.h>

namespace gs {
    SourceLocation::SourceLocation() {
        m_ref = nullptr;
        m_linePtr = nullptr;
        m_line = m_col = m_lineLen = m_length = m_endLine = m_endCol = 0;
    }

    SourceLocation::SourceLocation(ProgramSource* src, u32 line, u32 col) {
        m_ref = src;
        m_line = line;
        m_col = col;
        m_length = m_endLine = m_endCol = 0;

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

    u32 SourceLocation::getOffset() const {
        return u32(m_ref->getLine(m_line).c_str() - m_ref->getRawCode().c_str()) + m_col;
    }

    u32 SourceLocation::getLength() const {
        return m_length;
    }
    
    SourceLocation SourceLocation::getEndLocation() const {
        if (m_length == 0) return SourceLocation(m_ref, m_line, m_lineLen);
        return SourceLocation(m_ref, m_endLine, m_endCol);
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