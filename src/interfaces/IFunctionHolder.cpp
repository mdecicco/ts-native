#include <tsn/interfaces/IFunctionHolder.h>
#include <tsn/ffi/Function.h>
#include <tsn/utils/function_match.h>
#include <utils/Array.hpp>

namespace tsn {
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

    void IFunctionHolder::addFunction(ffi::Function* fn) {
        if (fn->getId() == 0) {
            throw std::exception("Function has not been added to the registry");
        }

        if (m_funcIdMap.count(fn->getId()) > 0) return;
        m_funcIdMap[fn->getId()] = m_funcs.size();
        m_funcs.push(fn);
    }

    utils::Array<ffi::Function*> IFunctionHolder::findFunctions(
        const utils::String& name,
        const ffi::DataType* retTp,
        const ffi::DataType** argTps,
        u8 argCount,
        function_match_flags flags
    ) {
        return function_match(name, retTp, argTps, argCount, m_funcs, flags);
    }
};