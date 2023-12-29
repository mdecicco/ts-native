#include <tsn/compiler/Scope.h>
#include <tsn/compiler/Parser.h>
#include <tsn/compiler/Compiler.h>
#include <tsn/compiler/OutputBuilder.h>
#include <tsn/compiler/FunctionDef.h>
#include <tsn/common/Context.h>
#include <tsn/ffi/DataType.h>
#include <tsn/ffi/DataTypeRegistry.h>
#include <tsn/ffi/Function.h>

#include <utils/Array.hpp>

namespace tsn {
    namespace compiler {
        //
        // Scope
        //

        Scope::Scope(Compiler* comp, Scope* parent) {
            m_parent = parent;
            m_comp = comp;
            m_func = m_comp->currentFunction();
            m_scopeOriginNode = m_comp->currentNode();
            m_didReturn = false;
        }

        Scope::~Scope() {
            m_namedVars.each([](Value* v) {
                if (v) delete v;
            });
        }

        Value& Scope::add(const utils::String& name, Value* v) {
            auto it = m_symbols.find(name);
            if (it != m_symbols.end()) {
                return *it->second.value;
            }

            m_symbols[name] = { v };
            m_namedVars.push(v);
            m_comp->getOutput()->addSymbolLifetime(name, m_scopeOriginNode, *v);
            return *v;
        }

        Value& Scope::add(const utils::String& name, ffi::DataType* t) {
            return add(name, new Value(m_comp->getOutput()->getFuncs()[0], t, true));
        }

        Value& Scope::add(const utils::String& name, FunctionDef* f) {
            return add(name, new Value(m_comp->getOutput()->getFuncs()[0], f));
        }

        Value& Scope::add(const utils::String& name, Module* m) {
            return add(name, new Value(m_comp->getOutput()->getFuncs()[0], m));
        }

        Value& Scope::add(const utils::String& name, Module* m, u32 slotId) {
            return add(name, new Value(m_comp->getOutput()->getFuncs()[0], m, slotId));
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
            emitScopeExitInstructions(m_scopes.last());
            m_scopes.pop();
        }

        void ScopeManager::exit(const Value& _save) {
            const Value* save = &_save;
            if (save && !save->isStack() && save->getSrcSelf() && save->getSrcSelf()->isStack()) {
                save = save->getSrcSelf();
            }

            emitScopeExitInstructions(m_scopes.last(), save);
            m_scopes.pop();
            
            if (save && save->isStack()) m_scopes.last().addToStack(*save);
        }
        
        void ScopeManager::emitReturnInstructions() {
            FunctionDef* functionBound = nullptr;
            for (i32 i = m_scopes.size() - 1;i >= 0;i--) {
                if (functionBound && m_scopes[i].m_func != functionBound) break;

                emitScopeExitInstructions(m_scopes[i]);

                if (i == m_scopes.size() - 1) {
                    m_scopes[i].m_didReturn = true;
                    functionBound = m_scopes[i].m_func;
                }
            }
        }
        
        Scope& ScopeManager::getBase() {
            return m_scopes[0];
        }

        Scope& ScopeManager::get() {
            return m_scopes.last();
        }

        Value& ScopeManager::add(const utils::String& name, Value* v) {
            return m_scopes.last().add(name, v);
        }

        Value& ScopeManager::add(const utils::String& name, ffi::DataType* t) {
            return m_scopes.last().add(name, t);
        }

        Value& ScopeManager::add(const utils::String& name, FunctionDef* f) {
            return m_scopes.last().add(name, f);
        }

        Value& ScopeManager::add(const utils::String& name, Module* m) {
            return m_scopes.last().add(name, m);
        }

        Value& ScopeManager::add(const utils::String& name, Module* m, u32 slotId) {
            return m_scopes.last().add(name, m, slotId);
        }

        symbol* ScopeManager::get(const utils::String& name) {
            for (i32 i = i32(m_scopes.size()) - 1;i >= 0;i--) {
                symbol* s = m_scopes[i].get(name);
                if (s) return s;
            }

            return nullptr;
        }
        
        void ScopeManager::emitScopeExitInstructions(const Scope& s, const Value* save) {
            if (s.m_didReturn) return;

            if (save && !save->isStack() && save->getSrcSelf() && save->getSrcSelf()->isStack()) {
                save = save->getSrcSelf();
            }

            FunctionDef* cf = m_comp->currentFunction();
            // Scope `s` may not be the deepest scope. For example, a break statement in an if
            // statement in a loop. The break exits the loop scope, but the current scope is
            // the if statement body. All scopes inside of scope `s` must also be handled.

            ffi::Function* crefDtor = m_comp->getContext()->getTypes()->getClosureRef()->getDestructor();

            for (i64 i = m_scopes.size() - 1;i >= 0;i--) {
                Scope& cur = m_scopes[(u32)i];

                for (i32 i = cur.m_stackObjs.size() - 1;i >= 0;i--) {
                    const Value& v = cur.m_stackObjs[i];
                    if (save && v.getStackAllocId() == save->getStackAllocId()) continue;

                    if (v.isFunction()) {
                        // runtime function references are actually of type 'ClosureRef'
                        Value ptr = cf->val(v.getType());
                        m_comp->add(ir_stack_ptr).op(ptr).op(cf->imm(v.getStackAllocId()));
                        m_comp->generateCall(crefDtor, {}, &ptr);
                    } else {
                        ffi::Function* dtor = v.getType()->getDestructor();
                        if (dtor) {
                            Value ptr = cf->val(v.getType());
                            m_comp->add(ir_stack_ptr).op(ptr).op(cf->imm(v.getStackAllocId()));
                            m_comp->generateCall(dtor, {}, &ptr);
                        }
                    }

                    m_comp->add(ir_stack_free).op(cf->imm(v.m_allocId));
                }
                
                if (cur.m_parent == s.m_parent) break;
            }
        }
    };
};