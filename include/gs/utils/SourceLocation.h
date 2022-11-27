#pragma once
#include <gs/common/types.h>

#include <utils/String.h>

namespace gs {
    namespace compiler {
        class Lexer;
        struct ast_node;
    };

    class ProgramSource;
    class SourceLocation {
        public:
            SourceLocation();
            SourceLocation(ProgramSource* src, u32 line, u32 col);
            ~SourceLocation();

            ProgramSource* getSource() const;
            utils::String getLineText() const;
            const char* getPointer() const;
            u32 getLine() const;
            u32 getCol() const;
            u32 getOffset() const;
            u32 getLength() const;
            SourceLocation getEndLocation() const;
            bool isValid() const;

            bool operator++(int);
            char operator*() const;
        
        protected:
            friend struct compiler::ast_node;
            u32 m_length;
            u32 m_endLine;
            u32 m_endCol;

        private:
            ProgramSource* m_ref;
            const char* m_linePtr;
            u32 m_line;
            u32 m_lineLen;
            u32 m_col;
    };
    
    class SourceException : public std::exception {
        public:
            SourceException(const utils::String& message, const SourceLocation& src);
            ~SourceException();

            virtual char const* what() const;

            const SourceLocation& getSource() const;

        private:
            utils::String m_message;
            SourceLocation m_src;
    };
};