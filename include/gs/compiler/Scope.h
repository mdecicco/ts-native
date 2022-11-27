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
        class Compiler;

        struct symbol {
            utils::Array<Value*> values;
        };

        class Scope {
            public:
                Scope(Compiler* comp, Scope* parent);
                ~Scope();

                void add(const utils::String& name, Value* v);
                void add(const utils::String& name, ffi::DataType* t);
                void add(const utils::String& name, FunctionDef* f);
                void add(const utils::String& name, Module* m);
                symbol* get(const utils::String& name);

                void addToStack(const Value& v);

            private:
                robin_hood::unordered_map<utils::String, symbol> m_symbols;
                utils::Array<Value*> m_namedVars;
            
            protected:
                friend class ScopeManager;
                utils::Array<Value> m_stackObjs;
                utils::Array<u32> m_symLifetimes;
                ast_node* m_scopeOriginNode;
                Scope* m_parent;
                Compiler* m_comp;
        };

        class ScopeManager {
            public:
                ScopeManager(Compiler* comp);

                Scope& enter();
                void exit();
                void exit(const Value& v);

                Scope& getBase();
                Scope& get();

                // Proxy functions for current scope
                void add(const utils::String& name, Value* v);
                void add(const utils::String& name, ffi::DataType* t);
                void add(const utils::String& name, FunctionDef* f);
                void add(const utils::String& name, Module* m);
                symbol* get(const utils::String& name);

                void emitScopeExitInstructions(const Scope& s, const Value* save = nullptr);
            
            private:
                utils::Array<Scope> m_scopes;
                Compiler* m_comp;
        };
    };
};