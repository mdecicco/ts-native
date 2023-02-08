#pragma once
#include <tsn/common/types.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace tsn {
    namespace ffi {
        class Function;
    };

    struct func_range {
        ffi::Function* fn;
        u32 firstLine;
        u32 lastLine;
    };

    class ProgramSource {
        public:
            ProgramSource(const utils::String& sourceName, const utils::String& rawCode);
            ~ProgramSource();

            u32 getLineCount() const;

            const utils::String& getSourceName() const;
            const utils::String& getRawCode() const;
            const utils::Array<utils::String>& getLines() const;
            const utils::String& getLine(u32 idx) const;
            const utils::Array<func_range>& getFunctionInfo() const;
        
        private:
            utils::String m_sourceName;
            utils::String m_rawCode;
            utils::Array<utils::String> m_lines;
            utils::Array<func_range> m_funcRanges;
    };
};