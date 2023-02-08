#include <tsn/compiler/Output.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/FunctionDef.hpp>
#include <tsn/compiler/Parser.h>
#include <tsn/common/Context.h>
#include <tsn/common/Module.h>
#include <tsn/common/Function.h>
#include <tsn/common/DataType.h>
#include <tsn/common/TypeRegistry.h>
#include <tsn/common/FunctionRegistry.h>
#include <tsn/compiler/Value.hpp>
#include <tsn/interfaces/IDataTypeHolder.hpp>
#include <tsn/interfaces/ICodeHolder.h>
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

        CompilerOutput::CompilerOutput(Compiler* c, Module* m) {
            m_comp = c;
            m_mod = m;
        }

        CompilerOutput::~CompilerOutput() {
            m_allFuncDefs.each([](FunctionDef* f) {
                delete f;
            });

            m_symbolLifetimes.each([](const symbol_lifetime& s) {
                delete s.sym;
            });

            // Types are someone else's responsibility now
        }

        FunctionDef* CompilerOutput::newFunc(const utils::String& name, ffi::DataType* methodOf) {
            FunctionDef* fn = new FunctionDef(m_comp, name, methodOf);
            // Don't add __init__ function to scope
            if (m_funcs.size() > 0) m_comp->scope().add(name, fn);
            m_funcs.push(fn);
            m_allFuncDefs.push(fn);
            
            // def index will be added to m_funcDefs later when output Function* is created

            return fn;
        }
        
        FunctionDef* CompilerOutput::newFunc(ffi::Function* preCreated, bool retTpExplicit) {
            FunctionDef* fn = new FunctionDef(m_comp, preCreated);
            fn->m_retTpSet = retTpExplicit;
            m_comp->scope().add(preCreated->getName(), fn);
            m_funcs.push(fn);
            m_funcDefs[preCreated] = m_allFuncDefs.size();
            m_allFuncDefs.push(fn);
            return fn;
        }
        
        void CompilerOutput::resolveFunctionDef(FunctionDef* def, ffi::Function* fn) {
            i64 idx = m_allFuncDefs.findIndex([def](FunctionDef* d) { return d == def; });
            m_funcDefs[fn] = (u32)idx;
        }
        
        FunctionDef* CompilerOutput::getFunctionDef(ffi::Function* fn) {
            auto it = m_funcDefs.find(fn);
            if (it != m_funcDefs.end()) return m_allFuncDefs[it->second];

            FunctionDef* def = new FunctionDef(m_comp, fn);
            m_funcDefs[fn] = m_allFuncDefs.size();
            m_allFuncDefs.push(def);
            return def;
        }

        void CompilerOutput::add(ffi::DataType* tp) {
            m_comp->scope().add(tp->getName(), tp);
            m_types.push(tp);
        }
        
        FunctionDef* CompilerOutput::import(Function* fn, const utils::String& as) {
            FunctionDef* f = new FunctionDef(m_comp, fn);
            m_comp->scope().add(as, f);
            m_funcDefs[fn] = m_allFuncDefs.size();
            m_allFuncDefs.push(f);
            return f;
        }

        void CompilerOutput::import(DataType* tp, const utils::String& as) {
            m_comp->scope().add(as, tp);
        }

        u32 CompilerOutput::addSymbolLifetime(const utils::String& name, ParseNode* scopeRoot, const Value& v) {
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
        
        const utils::Array<symbol_lifetime>& CompilerOutput::getSymbolLifetimeData() const {
            return m_symbolLifetimes;
        }

        const utils::Array<FunctionDef*>& CompilerOutput::getFuncs() const {
            return m_funcs;
        }

        const utils::Array<DataType*>& CompilerOutput::getTypes() const {
            return m_types;
        }

        Module* CompilerOutput::getModule() {
            return m_mod;
        }

        bool CompilerOutput::serialize(utils::Buffer* out, Context* ctx) const {
            auto writeFunc = [out, ctx](const FunctionDef* f) {
                if (!f->serialize(out, ctx)) return true;
                return false;
            };
            
            auto writeType = [out, ctx](const DataType* f) {
                if (!f->serialize(out, ctx)) return true;
                return false;
            };

            if (!out->write(m_mod->getId())) return false;
            if (!out->write(m_mod->getName())) return false;

            // Symbols lifetimes ignored, only used for autocompletion
            // when editing scripts. ie. Only relevant in a context where
            // the script would not be cached.

            if (!out->write(m_funcs.size())) return false;
            if (m_funcs.some(writeFunc)) return false;
            if (!out->write(m_types.size())) return false;
            if (m_types.some(writeType)) return false;

            // m_importedFuncs, m_funcDefs, m_allFuncDefs are not relevant
            // post-compilation.

            return true;
        }

        bool CompilerOutput::deserialize(utils::Buffer* in, Context* ctx) {
            return false;
        }
    };
};