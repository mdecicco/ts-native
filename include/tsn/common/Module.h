#pragma once
#include <tsn/common/types.h>
#include <tsn/ffi/Object.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/interfaces/IDataTypeHolder.h>

#include <utils/String.h>

namespace tsn {
    class Pipeline;
    class ModuleSource;

    namespace compiler {
        class Compiler;
        class Output;
    };

    struct module_data {
        void* ptr;
        u32 size;
        ffi::DataType* type;
        access_modifier access;
        utils::String name;
    };

    struct script_metadata;

    class Module : public IContextual, public IFunctionHolder, public IDataTypeHolder {
        public:
            u32 getId() const;
            const utils::String& getName() const;
            const utils::String& getPath() const;
            const script_metadata* getInfo() const;

            u32 getDataSlotCount() const;
            const module_data& getDataInfo(u32 slot) const;
            void setDataAccess(u32 slot, access_modifier access);
            Object getData(u32 slot);
            const utils::Array<module_data>& getData() const;
            ModuleSource* getSource() const;
        
        private:
            friend class compiler::Compiler;
            friend class compiler::Output;
            friend class Context;
            friend class Pipeline;
            Module(Context* ctx, const utils::String& name, const utils::String& path, const script_metadata* meta);
            ~Module();
            
            u32 addData(const utils::String& name, ffi::DataType* tp, access_modifier access);
            u32 addData(const utils::String& name, u32 size);
            void setSrc(ModuleSource* src);

            utils::String m_name;
            utils::String m_path;
            utils::Array<module_data> m_data;
            u32 m_id;
            ModuleSource* m_src;
            const script_metadata* m_meta;
    };
};