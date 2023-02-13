#include <tsn/compiler/TemplateContext.h>
#include <tsn/compiler/Parser.h>
#include <tsn/common/Context.h>
#include <tsn/common/DataType.h>
#include <tsn/common/Function.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/FunctionRegistry.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        TemplateContext::TemplateContext() {
            m_ast = nullptr;
        }

        TemplateContext::TemplateContext(ParseNode* ast) {
            m_ast = ast->clone();
        }

        TemplateContext::~TemplateContext() {
            if (m_ast) ParseNode::destroyDetachedAST(m_ast);
            m_ast = nullptr;
        }

        void TemplateContext::addModuleDataImport(const utils::String& alias, u32 moduleId, u32 slotId) {
            m_moduleDataImports.push({ moduleId, slotId, alias });
        }

        void TemplateContext::addModuleImport(const utils::String& alias, u32 moduleId) {
            m_moduleImports.push({ moduleId, alias });
        }

        void TemplateContext::addFunctionImport(const utils::String& alias, ffi::Function* fn) {
            m_functionImports.push({ fn, alias });
        }

        void TemplateContext::addTypeImport(const utils::String& alias, ffi::DataType* tp) {
            m_typeImports.push({ tp, alias });
        }
        
        const utils::Array<TemplateContext::moduledata_import>& TemplateContext::getModuleDataImports() const {
            return m_moduleDataImports;
        }

        const utils::Array<TemplateContext::module_import>& TemplateContext::getModuleImports() const {
            return m_moduleImports;
        }

        const utils::Array<TemplateContext::function_import>& TemplateContext::getFunctionImports() const {
            return m_functionImports;
        }

        const utils::Array<TemplateContext::datatype_import>& TemplateContext::getTypeImports() const {
            return m_typeImports;
        }

        ParseNode* TemplateContext::getAST() {
            return m_ast;
        }
                
        bool TemplateContext::serialize(utils::Buffer* out, Context* ctx, void* extra) const {
            if (!out->write(m_moduleDataImports.size())) return false;
            for (u32 i = 0;i < m_moduleDataImports.size();i++) {
                if (!out->write(m_moduleDataImports[i].module_id)) return false;
                if (!out->write(m_moduleDataImports[i].slot_id)) return false;
                if (!out->write(m_moduleDataImports[i].alias)) return false;
            }

            if (!out->write(m_moduleImports.size())) return false;
            for (u32 i = 0;i < m_moduleImports.size();i++) {
                if (!out->write(m_moduleImports[i].id)) return false;
                if (!out->write(m_moduleImports[i].alias)) return false;
            }

            if (!out->write(m_functionImports.size())) return false;
            for (u32 i = 0;i < m_functionImports.size();i++) {
                if (!out->write(m_functionImports[i].fn->getId())) return false;
                if (!out->write(m_functionImports[i].alias)) return false;
            }

            if (!out->write(m_typeImports.size())) return false;
            for (u32 i = 0;i < m_typeImports.size();i++) {
                if (!out->write(m_typeImports[i].tp->getId())) return false;
                if (!out->write(m_typeImports[i].alias)) return false;
            }

            if (!m_ast->serialize(out, ctx, nullptr)) return false;

            return true;
        }

        bool TemplateContext::deserialize(utils::Buffer* in, Context* ctx, void* extra) {
            u32 count;
            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                moduledata_import import;
                if (!in->read(import.module_id)) return false;
                if (!in->read(import.slot_id)) return false;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_moduleDataImports.push(import);
            }

            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                module_import import;
                if (!in->read(import.id)) return false;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_moduleImports.push(import);
            }

            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                function_import import;

                function_id fid;
                if (!in->read(fid)) return false;
                import.fn = ctx->getFunctions()->getFunction(fid);
                if (!import.fn) return false;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_functionImports.push(import);
            }

            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                datatype_import import;
                
                type_id tid;
                if (!in->read(tid)) return false;
                import.tp = ctx->getTypes()->getType(tid);
                if (!import.tp) return false;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_typeImports.push(import);
            }

            m_ast = new ParseNode();
            if (!m_ast->deserialize(in, ctx, nullptr)) {
                delete m_ast;
                m_ast = nullptr;
                return false;
            }

            return true;
        }
    };
};