#include <tsn/compiler/Scope.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/common/DataType.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace compiler {
        //
        // Scope
        //

        Scope::Scope(Compiler* comp, Scope* parent) : m_parent(parent), m_comp(comp), m_scopeOriginNode(comp->currentNode()) {
        }

        Scope::~Scope() {
            m_namedVars.each([](Value* v) {
                if (v) delete v;
            });
        }

        void Scope::add(const utils::String& name, Value* v) {
            auto it = m_symbols.find(name);
            if (it != m_symbols.end()) return;

            m_symbols[name] = { v };
            m_namedVars.push(v);
            m_comp->getOutput()->addSymbolLifetime(name, m_scopeOriginNode, *v);
        }

        void Scope::add(const utils::String& name, ffi::DataType* t) {
            add(name, new Value(m_comp->getOutput()->getFuncs()[0], t, true));
        }

        void Scope::add(const utils::String& name, FunctionDef* f) {
            add(name, new Value(m_comp->getOutput()->getFuncs()[0], f));
        }

        void Scope::add(const utils::String& name, Module* m) {
            add(name, new Value(m_comp->getOutput()->getFuncs()[0], m));
        }

        void Scope::add(const utils::String& name, Module* m, u32 slotId) {
            add(name, new Value(m_comp->getOutput()->getFuncs()[0], m, slotId));
        }

        symbol* Scope::get(const utils::String& name) {
            auto it = m_symbols.find(name);
            if (it == m_symbols.end()) return nullptr;
            return &it->second;
        }
        
        void Scope::addToStack(const Value& v) {
            m_stackObjs.push(v);
        }



        //
        // ScopeManager
        //
        ScopeManager::ScopeManager(Compiler* comp) : m_comp(comp) {
        }

        Scope& ScopeManager::enter() {
            m_scopes.push(Scope(m_comp, m_scopes.size() > 1 ?& m_scopes.last() : nullptr));
            return m_scopes.last();
        }

        void ScopeManager::exit() {
            emitScopeExitInstructions(m_scopes[m_scopes.size() - 1]);
            m_scopes.pop();
        }

        void ScopeManager::exit(const Value& save) {
            emitScopeExitInstructions(m_scopes[m_scopes.size() - 1], &save);
            m_scopes.pop();
            
            if (save.isStack()) m_scopes[m_scopes.size() - 1].addToStack(save);
        }
        
        Scope& ScopeManager::getBase() {
            return m_scopes[0];
        }

        Scope& ScopeManager::get() {
            return m_scopes[m_scopes.size() - 1];
        }

        void ScopeManager::add(const utils::String& name, Value* v) {
            m_scopes[m_scopes.size() - 1].add(name, v);
        }

        void ScopeManager::add(const utils::String& name, ffi::DataType* t) {
            m_scopes[m_scopes.size() - 1].add(name, t);
        }

        void ScopeManager::add(const utils::String& name, FunctionDef* f) {
            m_scopes[m_scopes.size() - 1].add(name, f);
        }

        void ScopeManager::add(const utils::String& name, Module* m) {
            m_scopes[m_scopes.size() - 1].add(name, m);
        }

        void ScopeManager::add(const utils::String& name, Module* m, u32 slotId) {
            m_scopes[m_scopes.size() - 1].add(name, m, slotId);
        }

        symbol* ScopeManager::get(const utils::String& name) {
            for (i32 i = i32(m_scopes.size()) - 1;i >= 0;i--) {
                symbol* s = m_scopes[i].get(name);
                if (s) return s;
            }

            return nullptr;
        }
        
        void ScopeManager::emitScopeExitInstructions(const Scope& s, const Value* save) {
            FunctionDef* cf = m_comp->currentFunction();
            // Scope `s` may not be the deepest scope. For example, a break statement in an if
            // statement in a loop. The break exits the loop scope, but the current scope is
            // the if statement body. All scopes inside of scope `s` must also be handled.

            for (i64 i = m_scopes.size() - 1;i >= 0;i--) {
                Scope& cur = m_scopes[(u32)i];

                for (const Value& v : cur.m_stackObjs) {
                    if (save && v.isReg() && v.getRegId() == save->getRegId()) continue;

                    ffi::Function* dtor = v.getType()->getDestructor();
                    if (dtor) m_comp->generateCall(dtor, {}, &v);
                    m_comp->add(ir_stack_free).op(cf->imm(v.m_allocId));
                }
                
                if (cur.m_parent == s.m_parent) break;
            }
        }
    };
};