#pragma once
#include <gs/common/types.h>

#include <utils/String.h>

namespace gs {
    class ProgramSource;
    class SourceLocation {
        public:
            SourceLocation(ProgramSource* src, u32 line, u16 col);
            ~SourceLocation();

            const utils::String& getLineText() const;
            u32 getLine() const;
            u16 getCol() const;

        private:
            ProgramSource* m_ref;
            u32 m_line;
            u16 m_col;
    };
};