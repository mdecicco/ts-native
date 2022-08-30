#pragma once
#include <gs/common/types.h>
#include <gs/compiler/types.h>
#include <gs/compiler/Value.h>

#include <utils/Array.h>
#include <utils/String.h>
#include <utils/robin_hood.h>

namespace gs {
    class Module;
    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        class FunctionDef;

        enum symbol_type {
            st_variable,
            st_type,
            st_func,
            st_module
        };

        struct symbol {
            symbol_type tp;

            Value* var;
            ffi::DataType* type;
            ffi::Function* func;
            FunctionDef* funcDef;

            Module* mod;
        };

        class Scope {
            public:
                Scope();
                ~Scope();

                void add(const utils::String& name, Value* v);
                void add(const utils::String& name, ffi::DataType* t);
                void add(const utils::String& name, ffi::Function* f);
                void add(const utils::String& name, FunctionDef* f);
                void add(const utils::String& name, Module* m);
                symbol* get(const utils::String& name);

            private:
                robin_hood::unordered_map<utils::String, symbol> m_symbols;
                utils::Array<Value*> m_namedVars;
                utils::Array<Value> m_stackObjs;
        };

        class FunctionDef;
        class ScopeManager {
            public:
                ScopeManager();

                Scope& enter();
                void exit();
                void exit(const Value& v);

                Scope& getBase();
                Scope& get();

                // Proxy functions for current scope
                void add(const utils::String& name, Value* v);
                void add(const utils::String& name, ffi::DataType* t);
                void add(const utils::String& name, ffi::Function* f);
                void add(const utils::String& name, FunctionDef* f);
                void add(const utils::String& name, Module* m);
                symbol* get(const utils::String& name);
            
            private:
                utils::Array<Scope> m_scopes;
        };
    };
};