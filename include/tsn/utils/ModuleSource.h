#pragma once
#include <tsn/common/types.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace tsn {
    class ModuleSource {
        public:
            ModuleSource(const utils::String& code, u64 modificationTime);
            ~ModuleSource();

            u32 getLineCount() const;
            u64 getModificationTime() const;

            const utils::String& getCode() const;
            const utils::Array<utils::String>& getLines() const;
            const utils::String& getLine(u32 idx) const;
        
        private:
            u64 m_modificationTime;
            utils::String m_code;
            utils::Array<utils::String> m_lines;
    };
};