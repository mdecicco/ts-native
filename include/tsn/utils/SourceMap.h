#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>
#include <utils/String.h>
#include <utils/Array.h>

namespace tsn {
    class ModuleSource;

    class SourceMap {
        public:
            struct source_range {
                ModuleSource* src;
                u32 line;
                u32 col;
                u32 span;
            };

            SourceMap();
            SourceMap(u64 fileModificationTime);
            ~SourceMap();

            void setSource(ModuleSource* source);
            void add(u32 line, u32 col, u32 span);
            void remove(u32 idx);

            const source_range& get(u32 idx) const;
            u64 getModificationTime() const;

            virtual bool serialize(utils::Buffer* out, Context* ctx) const;
            virtual bool deserialize(utils::Buffer* in, Context* ctx);

        protected:
            u64 m_fileModifiedOn;
            utils::Array<source_range> m_map;
            ModuleSource* m_curSource;
    };
};