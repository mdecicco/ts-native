#pragma once
#include <tsn/common/types.h>
#include <tsn/utils/SourceLocation.h>
#include <tsn/interfaces/IPersistable.h>

#include <utils/robin_hood.h>
#include <utils/String.h>

namespace tsn {
    class Module;

    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        class ParseNode;
        class FunctionDef;
        class Compiler;
        class Value;
        
        struct symbol_lifetime {
            utils::String name;
            Value* sym;
            SourceLocation begin;
            SourceLocation end;
        };

        class CompilerOutput : public IPersistable {
            public:
                CompilerOutput(Compiler* c, Module* m);
                ~CompilerOutput();

                FunctionDef* newFunc(const utils::String& name, ffi::DataType* methodOf = nullptr);
                FunctionDef* newFunc(ffi::Function* preCreated, bool retTpExplicit = true);
                void resolveFunctionDef(FunctionDef* def, ffi::Function* fn);
                FunctionDef* getFunctionDef(ffi::Function* fn);
                void add(ffi::DataType* tp);

                FunctionDef* import(ffi::Function* fn, const utils::String& as);
                void import(ffi::DataType* fn, const utils::String& as);
                u32 addSymbolLifetime(const utils::String& name, ParseNode* scopeRoot, const Value& v);

                const utils::Array<FunctionDef*>& getFuncs() const;
                const utils::Array<ffi::DataType*>& getTypes() const;
                const utils::Array<symbol_lifetime>& getSymbolLifetimeData() const;

                Module* getModule();

                virtual bool serialize(utils::Buffer* out, Context* ctx) const;
                virtual bool deserialize(utils::Buffer* in, Context* ctx);

            protected:
                Compiler* m_comp;
                Module* m_mod;
                utils::Array<symbol_lifetime> m_symbolLifetimes;
                utils::Array<FunctionDef*> m_funcs;
                utils::Array<ffi::DataType*> m_types;
                utils::Array<FunctionDef*> m_importedFuncs;
                robin_hood::unordered_map<ffi::Function*, u32> m_funcDefs;
                utils::Array<FunctionDef*> m_allFuncDefs;
        };
    };
};