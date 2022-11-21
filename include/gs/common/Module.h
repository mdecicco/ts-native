#pragma once
#include <gs/common/types.h>
#include <gs/common/Object.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IFunctionHolder.h>
#include <gs/interfaces/IDataTypeHolder.h>

#include <utils/String.h>

namespace gs {
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
            Module(Context* ctx, const utils::String& name);
            ~Module();

            u32 getId() const;
            const utils::String& getName() const;

            u32 getDataSlotCount() const;
            const module_data& getDataInfo(u32 slot) const;
            void setDataAccess(u32 slot, access_modifier access);
            Object getData(u32 slot);
            const utils::Array<module_data>& getData() const;
        
        private:
            friend class compiler::Compiler;
            u32 addData(const utils::String& name, ffi::DataType* tp, access_modifier access);

            utils::String m_name;
            utils::Array<module_data> m_data;
            u32 m_id;
    };
};