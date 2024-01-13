#include <tsn/compiler/TemplateContext.h>
#include <tsn/compiler/Parser.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

namespace tsn {
    namespace compiler {
        TemplateContext::TemplateContext() {
            m_ast = nullptr;
        }

        TemplateContext::TemplateContext(ParseNode* ast, Module* module) {
            m_ast = ast->clone();
            m_originModule = module;
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
            m_functionImports.push({ fn->getId(), fn, alias });
        }

        void TemplateContext::addTypeImport(const utils::String& alias, ffi::DataType* tp) {
            m_typeImports.push({ tp->getId(), tp, alias });
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

        Module* TemplateContext::getOrigin() {
            return m_originModule;
        }

        bool TemplateContext::serialize(utils::Buffer* out, Context* ctx) const {
            if (!out->write(m_originModule->getId())) return false;

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
                if (!out->write(m_functionImports[i].id)) return false;
                if (!out->write(m_functionImports[i].alias)) return false;
            }

            if (!out->write(m_typeImports.size())) return false;
            for (u32 i = 0;i < m_typeImports.size();i++) {
                if (!out->write(m_typeImports[i].id)) return false;
                if (!out->write(m_typeImports[i].alias)) return false;
            }

            if (!m_ast->serialize(out, ctx)) return false;

            return true;
        }

        bool TemplateContext::deserialize(utils::Buffer* in, Context* ctx) {
            u32 mid;
            if (!in->read(mid)) return false;
            m_originModule = ctx->getModule(mid);
            if (!m_originModule) return false;

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

                if (!in->read(import.id)) return false;
                import.fn = nullptr;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_functionImports.push(import);
            }

            if (!in->read(count)) return false;
            for (u32 i = 0;i < count;i++) {
                datatype_import import;
                
                if (!in->read(import.id)) return false;
                import.tp = nullptr;
                
                import.alias = in->readStr();
                if (import.alias.size() == 0) return false;

                m_typeImports.push(import);
            }

            m_ast = new ParseNode();
            if (!m_ast->deserialize(in, ctx)) {
                delete m_ast;
                m_ast = nullptr;
                return false;
            }

            return true;
        }
        
        bool TemplateContext::resolveReferences(
            ffi::FunctionRegistry* freg,
            ffi::DataTypeRegistry* treg,
            robin_hood::unordered_map<function_id, ffi::Function*>& funcMap,
            robin_hood::unordered_map<type_id, ffi::DataType*>& typeMap
        ) {
            auto getTpById = [&typeMap, treg](type_id tid) {
                auto it = typeMap.find(tid);
                if (it != typeMap.end()) return it->second;

                return treg->getType(tid);
            };

            for (u32 i = 0;i < m_functionImports.size();i++) {
                auto& im = m_functionImports[i];
                if (im.fn) continue;
                
                auto it = funcMap.find(im.id);
                if (it != funcMap.end()) im.fn = it->second;
                else im.fn = freg->getFunction(im.id);

                if (!im.fn) return false;
            }
            
            for (u32 i = 0;i < m_typeImports.size();i++) {
                auto& im = m_typeImports[i];
                if (im.tp) continue;
                
                auto it = typeMap.find(im.id);
                if (it != typeMap.end()) im.tp = it->second;
                else im.tp = treg->getType(im.id);

                if (!im.tp) return false;
            }

            return true;
        }
    };
};