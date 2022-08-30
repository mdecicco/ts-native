#pragma once
#include <gs/common/types.h>
#include <gs/interfaces/IContextual.h>
#include <gs/interfaces/IFunctionHolder.h>
#include <gs/interfaces/IDataTypeHolder.h>

#include <utils/String.h>

namespace gs {
    class Module : public IContextual, public IFunctionHolder, public IDataTypeHolder {
        public:
            Module(Context* ctx, const utils::String& name);
            ~Module();

            const utils::String& getName() const;
        
        private:
            utils::String m_name;
    };
};