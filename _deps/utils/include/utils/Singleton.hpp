#pragma once
#include <utils/Singleton.h>
#include <utils/Thread.h>

namespace utils {
    template <typename T>
    T* Singleton<T>::instance = nullptr;

    template <typename T>
    template <typename ...Args>
    void Singleton<T>::Create(Args ...args) {
        if (instance) return;
        instance = new T(args...);
    }

    template <typename T>
    void Singleton<T>::Destroy() {
        if (instance) delete instance;
        instance = nullptr;
    }

    template <typename T>
    T* Singleton<T>::Get() {
        return instance;
    }

    
    template <typename T>
    PerThreadSingleton<T>* PerThreadSingleton<T>::instance = nullptr;

    template <typename T>
    template <typename ...Args>
    void PerThreadSingleton<T>::Create(Args&& ...args) {
        if (instance) return;
        instance = new PerThreadSingleton<T>();
        instance->m_newInstanceFunc = [...args = std::forward<Args>(args)]() {
            return new T(args...);
        };
    }

    template <typename T>
    void PerThreadSingleton<T>::Destroy() {
        if (instance) {
            for (auto it : instance->m_instanceMap) delete it.second;
            delete instance;
        }

        instance = nullptr;
    }

    template <typename T>
    void PerThreadSingleton<T>::Destroy(thread_id tid) {
        thread_id tid = Thread::Current();
        auto it = instance->m_instanceMap.find(tid);
        if (it == instance->m_instanceMap.end()) return;
        delete it->second;
        instance->m_instanceMap.erase(it);
    }

    template <typename T>
    T* PerThreadSingleton<T>::Get() {
        thread_id tid = Thread::Current();
        auto it = instance->m_instanceMap.find(tid);

        T* a = nullptr;
        if (it == instance->m_instanceMap.end()) {
            a = instance->m_newInstanceFunc();
            instance->m_instanceMap[tid] = a;
            return a;
        }

        return it->second;
    }
};