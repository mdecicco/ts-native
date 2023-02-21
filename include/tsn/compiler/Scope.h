#pragma once
#include <tsn/common/types.h>
#include <tsn/compiler/types.h>
#include <tsn/compiler/Value.h>

#include <utils/Array.h>
#include <utils/String.h>
#include <utils/robin_hood.h>

namespace tsn {
    class Module;

    namespace ffi {
        class DataType;
        class Function;
    };

    namespace compiler {
        class FunctionDef;
        class Compiler;

        struct symbol {
            Value* value;
        };

        class Scope {
            public:
                Scope(Compiler* comp, Scope* parent);
                ~Scope();

                Value& add(const utils::String& name, Value* v);
                Value& add(const utils::String& name, ffi::DataType* t);
                Value& add(const utils::String& name, FunctionDef* f);
                Value& add(const utils::String& name, Module* m);
                Value& add(const utils::String& name, Module* m, u32 slotId);
                symbol* get(const utils::String& name);

                void addToStack(const Value& v);

            private:
                robin_hood::unordered_map<utils::String, symbol> m_symbols;
                utils::Array<Value*> m_namedVars;
            
            protected:
                friend class ScopeManager;
                utils::Array<Value> m_stackObjs;
                ParseNode* m_scopeOriginNode;
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
                Value& add(const utils::String& name, Value* v);
                Value& add(const utils::String& name, ffi::DataType* t);
                Value& add(const utils::String& name, FunctionDef* f);
                Value& add(const utils::String& name, Module* m);
                Value& add(const utils::String& name, Module* m, u32 slotId);
                symbol* get(const utils::String& name);

                void emitScopeExitInstructions(const Scope& s, const Value* save = nullptr);
            
            private:
                utils::Array<Scope> m_scopes;
                Compiler* m_comp;
        };
    };
};