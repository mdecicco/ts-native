#pragma once
#include <gs/common/types.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace gs {
    class ProgramSource {
        public:
            ProgramSource(const utils::String& sourceName, const utils::String& rawCode);
            ~ProgramSource();

            u32 getLineCount() const;

            const utils::String& getSourceName() const;
            const utils::String& getRawCode() const;
            const utils::Array<utils::String>& getLines() const;
            const utils::String& getLine(u32 idx) const;
        
        private:
            utils::String m_sourceName;
            utils::String m_rawCode;
            utils::Array<utils::String> m_lines;
    };
};