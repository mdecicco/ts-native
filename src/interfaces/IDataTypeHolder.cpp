#include <tsn/interfaces/IDataTypeHolder.h>
#include <tsn/common/DataType.h>
#include <utils/Array.hpp>

namespace tsn {
    IDataTypeHolder::IDataTypeHolder() {
    }

    IDataTypeHolder::~IDataTypeHolder() {
    }

    ffi::DataType* IDataTypeHolder::getType(type_id id) const {
        auto it = m_typeIdMap.find(id);
        if (it == m_typeIdMap.end()) return nullptr;
        return m_types[it->second];
    }

    const utils::Array<ffi::DataType*>& IDataTypeHolder::allTypes() const {
        return m_types;
    }

    u32 IDataTypeHolder::typeCount() const {
        return m_types.size();
    }

    void IDataTypeHolder::addHostType(size_t hash, ffi::DataType* tp) {
        if (m_typeHashMap.count(hash) > 0) return;
        m_typeHashMap[hash] = m_types.size();
        m_typeIdMap[tp->getId()] = m_types.size();
        m_types.push(tp);
    }

    void IDataTypeHolder::addForeignType(ffi::DataType* tp) {
        if (m_typeIdMap.count(tp->getId()) > 0) return;
        m_typeIdMap[tp->getId()] = m_types.size();
        m_types.push(tp);
    }

    void IDataTypeHolder::addFuncType(ffi::FunctionType* tp) {
        if (m_typeIdMap.count(tp->getId()) > 0) return;
        m_typeIdMap[tp->getId()] = m_types.size();
        m_types.push(tp);
    }

    void IDataTypeHolder::removeType(ffi::DataType* tp) {
        auto it1 = m_typeIdMap.find(tp->getId());
        if (it1 == m_typeIdMap.end()) return;

        u32 idx = it1->second;
        m_typeIdMap.erase(it1);

        for (auto& p : m_typeIdMap) {
            if (p.second > idx) p.second--;
        }

        for (auto& p : m_typeHashMap) {
            if (p.second > idx) p.second--;
        }

        size_t hash = tp->getInfo().host_hash;
        if (hash) m_typeHashMap.erase(hash);

        m_types.remove(idx);
    }
};