#include <gs/interfaces/IFunctionHolder.h>
#include <gs/common/Function.h>
#include <utils/Array.hpp>

namespace gs {
    IFunctionHolder::IFunctionHolder() {
        m_funcs.push(nullptr);
    }

    IFunctionHolder::~IFunctionHolder() {
    }

    ffi::Function* IFunctionHolder::getFunction(function_id id) const {
        auto it = m_funcIdMap.find(id);
        if (it == m_funcIdMap.end()) return nullptr;
        return m_funcs[it->second];
    }
    
    ffi::Function* IFunctionHolder::getFunctionByIndex(u32 index) const {
        return m_funcs[index];
    }

    u32 IFunctionHolder::getFunctionIndex(function_id id) const {
        auto it = m_funcIdMap.find(id);
        if (it == m_funcIdMap.end()) return 0;
        return it->second;
    }

    const utils::Array<ffi::Function*>& IFunctionHolder::allFunctions() const {
        return m_funcs;
    }

    u32 IFunctionHolder::functionCount() const {
        return m_funcs.size();
    }

    void IFunctionHolder::mergeFunctions(IFunctionHolder& funcs) {
        utils::Array<ffi::Function*>& t = funcs.m_funcs;
    }

    void IFunctionHolder::addFunction(ffi::Function* fn) {
        if (m_funcIdMap.count(fn->getId()) > 0) return;
        m_funcIdMap[fn->getId()] = m_funcs.size();
        m_funcs.push(fn);
    }
};