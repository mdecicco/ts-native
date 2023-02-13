#pragma once
#include <tsn/common/types.h>
#include <tsn/interfaces/IPersistable.h>

#include <utils/Array.h>
#include <utils/String.h>

namespace tsn {
    namespace ffi {
        class Function;
        class DataType;
    };

    namespace compiler {
        class ParseNode;

        class TemplateContext : public IPersistable {
            public:
                struct moduledata_import {
                    u32 module_id;
                    u32 slot_id;
                    utils::String alias;
                };

                struct module_import {
                    u32 id;
                    utils::String alias;
                };

                struct function_import {
                    ffi::Function* fn;
                    utils::String alias;
                };

                struct datatype_import {
                    ffi::DataType* tp;
                    utils::String alias;
                };

                TemplateContext();
                TemplateContext(ParseNode* ast);
                ~TemplateContext();

                void addModuleDataImport(const utils::String& alias, u32 moduleId, u32 slotId);
                void addModuleImport(const utils::String& alias, u32 moduleId);
                void addFunctionImport(const utils::String& alias, ffi::Function* fn);
                void addTypeImport(const utils::String& alias, ffi::DataType* tp);

                const utils::Array<moduledata_import>& getModuleDataImports() const;
                const utils::Array<module_import>& getModuleImports() const;
                const utils::Array<function_import>& getFunctionImports() const;
                const utils::Array<datatype_import>& getTypeImports() const;
                ParseNode* getAST();
                
                virtual bool serialize(utils::Buffer* out, Context* ctx, void* extra) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx, void* extra);
            
            protected:
                utils::Array<moduledata_import> m_moduleDataImports;
                utils::Array<module_import> m_moduleImports;
                utils::Array<function_import> m_functionImports;
                utils::Array<datatype_import> m_typeImports;
                ParseNode* m_ast;
        };
    };
};