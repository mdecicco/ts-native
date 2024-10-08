#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/Parser.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/ffi/Function.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/FunctionRegistry.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/utils/function_match.h>

#include <utils/Array.hpp>
#include <utils/Buffer.hpp>

#include <stdarg.h>

using namespace utils;
using namespace tsn::ffi;

namespace tsn {
    namespace compiler {
        //
        // CompilerOutput
        //

        OutputBuilder::OutputBuilder(Compiler* c, Module* m) {
            m_comp = c;
            m_mod = m;
        }

        OutputBuilder::~OutputBuilder() {
            m_allFuncDefs.each([](FunctionDef* f) {
                delete f;
            });

            m_symbolLifetimes.each([](const symbol_lifetime& s) {
                delete s.sym;
            });

            // Types are someone else's responsibility now
        }

        FunctionDef* OutputBuilder::newFunc(const utils::String& name, ParseNode* n, ffi::DataType* methodOf) {
            FunctionDef* fn = new FunctionDef(m_comp, name, methodOf, n);
            
            // Don't add __init__ function to scope
            if (m_funcs.size() > 0) {
                fn->m_scopeRef = &m_comp->scope().add(name, fn);
            }
            m_funcs.push(fn);
            m_allFuncDefs.push(fn);
            
            // def index will be added to m_funcDefs later when output Function* is created

            return fn;
        }
        
        FunctionDef* OutputBuilder::newFunc(ffi::Function* preCreated, ParseNode* n, bool retTpExplicit) {
            FunctionDef* fn = new FunctionDef(m_comp, preCreated, n);
            fn->m_retTpSet = retTpExplicit;
            fn->m_scopeRef = &m_comp->scope().add(preCreated->getName(), fn);
            m_funcs.push(fn);
            m_funcDefs[preCreated] = m_allFuncDefs.size();
            m_allFuncDefs.push(fn);
            return fn;
        }
        
        void OutputBuilder::resolveFunctionDef(FunctionDef* def, ffi::Function* fn) {
            i64 idx = m_allFuncDefs.findIndex([def](FunctionDef* d) { return d == def; });
            m_funcDefs[fn] = (u32)idx;
        }
        
        FunctionDef* OutputBuilder::getFunctionDef(ffi::Function* fn) {
            auto it = m_funcDefs.find(fn);
            if (it != m_funcDefs.end()) return m_allFuncDefs[it->second];

            FunctionDef* def = new FunctionDef(m_comp, fn, nullptr);
            m_funcDefs[fn] = m_allFuncDefs.size();
            m_allFuncDefs.push(def);
            return def;
        }

        void OutputBuilder::add(ffi::DataType* tp) {
            m_comp->scope().add(tp->getName(), tp);
            m_types.push(tp);
        }
        
        FunctionDef* OutputBuilder::import(Function* fn, const utils::String& as) {
            FunctionDef* f = new FunctionDef(m_comp, fn, nullptr);
            m_comp->scope().add(as, f);
            m_funcDefs[fn] = m_allFuncDefs.size();
            m_allFuncDefs.push(f);
            return f;
        }

        void OutputBuilder::import(DataType* tp, const utils::String& as) {
            m_comp->scope().add(as, tp);
        }

        u32 OutputBuilder::addSymbolLifetime(const utils::String& name, ParseNode* scopeRoot, const Value& v) {
            ParseNode* n = m_comp->currentNode();
            scopeRoot->computeSourceLocationRange();

            m_symbolLifetimes.push({
                name,
                new Value(v),
                n->tok.src.getLine() > scopeRoot->tok.src.getLine() ? scopeRoot->tok.src : n->tok.src,
                scopeRoot->tok.src.getEndLocation()
            });

            return m_symbolLifetimes.size() - 1;
        }

        void OutputBuilder::addDependency(Module* mod) {
            if (!mod || mod == m_mod) return;

            for (u32 i = 0;i < m_dependencies.size();i++) {
                if (mod->getId() == m_dependencies[i]->getId()) return;
            }

            m_dependencies.push(mod);
        }

        void OutputBuilder::addHostTemplateSpecialization(ffi::DataType* type) {
            if (!type) return;

            for (u32 i = 0;i < m_specializedHostTypes.size();i++) {
                if (type->getId() == m_specializedHostTypes[i]->getId()) return;
            }

            m_specializedHostTypes.push(type);
        }
        
        const utils::Array<symbol_lifetime>& OutputBuilder::getSymbolLifetimeData() const {
            return m_symbolLifetimes;
        }
        
        const utils::Array<Module*>& OutputBuilder::getDependencies() const {
            return m_dependencies;
        }
        
        const utils::Array<ffi::DataType*>& OutputBuilder::getSpecializedHostTypes() const {
            return m_specializedHostTypes;
        }

        const utils::Array<FunctionDef*>& OutputBuilder::getFuncs() const {
            return m_funcs;
        }

        utils::Array<FunctionDef*>& OutputBuilder::getFuncs() {
            return m_funcs;
        }

        const utils::Array<DataType*>& OutputBuilder::getTypes() const {
            return m_types;
        }

        Module* OutputBuilder::getModule() {
            return m_mod;
        }

        Compiler* OutputBuilder::getCompiler() {
            return m_comp;
        }
    };
};