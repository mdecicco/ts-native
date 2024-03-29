#include <tsn/utils/SourceLocation.h>
#include <tsn/utils/ModuleSource.h>

#include <utils/Buffer.hpp>

namespace tsn {
    SourceLocation::SourceLocation() {
        m_ref = nullptr;
        m_linePtr = nullptr;
        m_line = m_col = m_lineLen = m_length = m_endLine = m_endCol = m_offset = 0;
    }
    
    SourceLocation::SourceLocation(const SourceLocation& o) {
        m_ref = o.m_ref;
        m_linePtr = o.m_linePtr;
        m_line = o.m_line;
        m_col = o.m_col;
        m_lineLen = o.m_lineLen;
        m_length = o.m_length;
        m_endLine = o.m_endLine;
        m_endCol = o.m_endCol;
        m_offset = o.m_offset;
    }
    
    SourceLocation::SourceLocation(ModuleSource* src, u32 line, u32 col) {
        m_ref = src;
        m_line = line;
        m_col = col;
        m_length = m_endLine = m_endCol = m_offset = 0;
        
        if (!src || src->getLineCount() <= line) {
            m_lineLen = 0;
            m_linePtr = nullptr;
        } else {
            const utils::String& ln = src->getLine(line);
            m_lineLen = ln.size();
            m_linePtr = ln.c_str();
            m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
        }
    }

    SourceLocation::~SourceLocation() {
    }

    void SourceLocation::setSource(ModuleSource* src) {
        m_ref = src;
        if (!src || src->getLineCount() <= m_line) {
            m_lineLen = 0;
            m_linePtr = nullptr;
        } else {
            const utils::String& ln = src->getLine(m_line);
            m_lineLen = ln.size();
            m_linePtr = ln.c_str();
            m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
        }
    }

    ModuleSource* SourceLocation::getSource() const {
        return m_ref;
    }

    utils::String SourceLocation::getLineText() const {
        if (!m_linePtr) return utils::String();
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
        return m_offset;
    }

    u32 SourceLocation::getLength() const {
        return m_length;
    }
    
    SourceLocation SourceLocation::getEndLocation() const {
        if (m_length == 0) return SourceLocation(m_ref, m_line, m_lineLen);
        return SourceLocation(m_ref, m_endLine, m_endCol);
    }

    bool SourceLocation::isValid() const {
        if (!m_ref) return false;
        return m_col < m_lineLen && m_line < m_ref->getLineCount();
    }

    bool SourceLocation::operator++(int) {
        if (!m_ref) return false;

        if (m_col < m_lineLen - 1) {
            m_col++;
            m_linePtr++;
            m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
            return true;
        } else if (m_line < m_ref->getLineCount() - 1) {
            m_col = 0;
            m_line++;
            const utils::String& ln = m_ref->getLine(m_line);
            m_lineLen = ln.size();
            m_linePtr = ln.c_str();
            m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
            return true;
        }

        return false;
    }

    char SourceLocation::operator*() const {
        if (!m_linePtr) return 0;
        return m_linePtr[m_col];
    }
    
    SourceLocation& SourceLocation::operator=(const SourceLocation& rhs) {
        m_ref = rhs.m_ref;
        m_linePtr = rhs.m_linePtr;
        m_line = rhs.m_line;
        m_col = rhs.m_col;
        m_lineLen = rhs.m_lineLen;
        m_length = rhs.m_length;
        m_endLine = rhs.m_endLine;
        m_endCol = rhs.m_endCol;
        m_offset = rhs.m_offset;

        return *this;
    }
    
    void SourceLocation::offset(const SourceLocation& by) {
        if (by.m_ref != m_ref) m_ref = by.m_ref;
        m_line += by.m_line;
        m_col += by.m_col;
        m_endLine += by.m_line;
        m_endCol += by.m_col;

        const utils::String& ln = m_ref->getLine(m_line);
        m_lineLen = ln.size();
        m_linePtr = ln.c_str();
        m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
    }

    bool SourceLocation::serialize(utils::Buffer* out, Context* ctx) const {
        if (!out->write(m_length)) return false;
        if (!out->write(m_endLine)) return false;
        if (!out->write(m_endCol)) return false;
        if (!out->write(m_line)) return false;
        if (!out->write(m_lineLen)) return false;
        if (!out->write(m_col)) return false;
        if (!out->write(m_offset)) return false;

        return true;
    }

    bool SourceLocation::deserialize(utils::Buffer* in, Context* ctx) {
        if (!in->read(m_length)) return false;
        if (!in->read(m_endLine)) return false;
        if (!in->read(m_endCol)) return false;
        if (!in->read(m_line)) return false;
        if (!in->read(m_lineLen)) return false;
        if (!in->read(m_col)) return false;
        if (!in->read(m_offset)) return false;

        if (m_ref) {
            if (m_ref->getLineCount() <= m_line) {
                m_lineLen = 0;
                m_linePtr = nullptr;
                return false;
            } else {
                const utils::String& ln = m_ref->getLine(m_line);
                m_lineLen = ln.size();
                m_linePtr = ln.c_str();
                m_offset = u32(m_ref->getLine(m_line).c_str() - m_ref->getCode().c_str()) + m_col;
            }
        }

        return true;
    }



    //
    // SourceException
    //

    SourceException::SourceException(const utils::String& message, const SourceLocation& src) : m_message(message), m_src(src) {
    }

    SourceException::~SourceException() {

    }

    char const* SourceException::what() const noexcept {
        return m_message.c_str();
    }

    const SourceLocation& SourceException::getSource() const {
        return m_src;
    }
};