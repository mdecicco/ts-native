#include <gs/compiler/Scope.h>
#include <gs/compiler/Parser.h>
#include <gs/compiler/Compiler.h>

#include <utils/Array.hpp>

namespace gs {
    namespace compiler {
        //
        // Scope
        //

        Scope::Scope() {
        }

        Scope::~Scope() {
            m_namedVars.each([](Value* v) {
                if (v) delete v;
            });
        }

        void Scope::add(const utils::String& name, Value* v) {
            m_symbols[name] = {
                st_variable,
                v,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            };
            m_namedVars.push(v);
        }

        void Scope::add(const utils::String& name, ffi::DataType* t) {
            m_symbols[name] = {
                st_type,
                nullptr,
                t,
                nullptr,
                nullptr,
                nullptr
            };
        }

        void Scope::add(const utils::String& name, ffi::Function* f) {
            m_symbols[name] = {
                st_func,
                nullptr,
                nullptr,
                f,
                nullptr,
                nullptr
            };
        }

        void Scope::add(const utils::String& name, FunctionDef* f) {
            m_symbols[name] = {
                st_func,
                nullptr,
                nullptr,
                nullptr,
                f,
                nullptr
            };
        }

        void Scope::add(const utils::String& name, Module* m) {
            m_symbols[name] = {
                st_module,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                m
            };
        }

        symbol* Scope::get(const utils::String& name) {
            auto it = m_symbols.find(name);
            if (it == m_symbols.end()) return nullptr;
            return &it->second;
        }



        //
        // ScopeManager
        //
        ScopeManager::ScopeManager() {
        }

        Scope& ScopeManager::enter() {
            m_scopes.push(Scope());
            return m_scopes[m_scopes.size() - 1];
        }

        void ScopeManager::exit() {
            m_scopes.pop();
        }

        void ScopeManager::exit(const Value& v) {
            m_scopes.pop();
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

        void ScopeManager::add(const utils::String& name, ffi::Function* f) {
            m_scopes[m_scopes.size() - 1].add(name, f);
        }

        void ScopeManager::add(const utils::String& name, FunctionDef* f) {
            m_scopes[m_scopes.size() - 1].add(name, f);
        }

        void ScopeManager::add(const utils::String& name, Module* m) {
            m_scopes[m_scopes.size() - 1].add(name, m);
        }

        symbol* ScopeManager::get(const utils::String& name) {
            for (i32 i = i32(m_scopes.size()) - 1;i >= 0;i--) {
                symbol* s = m_scopes[i].get(name);
                if (s) return s;
            }

            return nullptr;
        }
    };
};