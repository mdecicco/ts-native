#pragma once
#include <tsn/common/types.h>
#include <tsn/common/Object.h>
#include <tsn/interfaces/IContextual.h>
#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/interfaces/IDataTypeHolder.h>

#include <utils/String.h>

namespace tsn {
    namespace compiler {
        class Compiler;
    };

    struct module_data {
        void* ptr;
        ffi::DataType* type;
        access_modifier access;
        utils::String name;
    };

    class Module : public IContextual, public IFunctionHolder, public IDataTypeHolder {
        public:
            u32 getId() const;
            const utils::String& getName() const;
            const utils::String& getPath() const;

            u32 getDataSlotCount() const;
            const module_data& getDataInfo(u32 slot) const;
            void setDataAccess(u32 slot, access_modifier access);
            Object getData(u32 slot);
            const utils::Array<module_data>& getData() const;
        
        private:
            friend class compiler::Compiler;
            friend class Context;
            Module(Context* ctx, const utils::String& name, const utils::String& path);
            ~Module();
            
            u32 addData(const utils::String& name, ffi::DataType* tp, access_modifier access);

            utils::String m_name;
            utils::String m_path;
            utils::Array<module_data> m_data;
            u32 m_id;
    };
};