#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>

#include <utils/String.h>

namespace tsn {
    namespace compiler {
        class Lexer;
        class ParseNode;
    };

    class ModuleSource;
    class SourceLocation : public IPersistable {
        public:
            SourceLocation();
            SourceLocation(const SourceLocation& o);
            SourceLocation(ModuleSource* src, u32 line, u32 col);
            ~SourceLocation();

            void setSource(ModuleSource* src);
            ModuleSource* getSource() const;
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
            SourceLocation& operator=(const SourceLocation& rhs);
            void offset(const SourceLocation& by);

            virtual bool serialize(utils::Buffer* out, Context* ctx) const;
            virtual bool deserialize(utils::Buffer* in, Context* ctx);
        
        protected:
            friend class compiler::ParseNode;
            u32 m_length;
            u32 m_endLine;
            u32 m_endCol;

        private:
            ModuleSource* m_ref;
            const char* m_linePtr;
            u32 m_line;
            u32 m_lineLen;
            u32 m_col;
            u32 m_offset;
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