#pragma once
#include <tsn/common/types.h>

#include <utils/String.h>
#include <utils/Array.h>

namespace utils {
    class Buffer;
};

namespace tsn {
    struct script_metadata;
    
    class ModuleSource {
        public:
            ModuleSource(const utils::String& code, const script_metadata* meta);
            ModuleSource(const utils::Buffer* code, const script_metadata* meta);
            ~ModuleSource();

            u32 getLineCount() const;
            const script_metadata* getMeta() const;

            const utils::String& getCode() const;
            const utils::Array<utils::String>& getLines() const;
            const utils::String& getLine(u32 idx) const;
        
        private:
            void init();
            
            const script_metadata* m_meta;
            utils::String m_code;
            utils::Array<utils::String> m_lines;
    };
};