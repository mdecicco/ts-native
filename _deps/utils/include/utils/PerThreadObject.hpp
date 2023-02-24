#pragma once
#include <utils/robin_hood.h>
#include <utils/Types.h>
#include <utils/Thread.h>
#include <utils/TransientFunction.hpp>

#include <functional>

namespace utils {
    template <typename T>
    class PerThreadObject {
        public:
            template <typename F>
            PerThreadObject(F&& objCreator) : m_creator(objCreator) {
            }

            ~PerThreadObject() {
                for (auto& ele : m_objects) {
                    delete ele->second;
                }
                m_objects.clear();
            }

            T* get() {
                auto it = m_objects.find(Thread::Current());
                if (it == m_objects.end()) {
                    T* newObj = m_creator();
                    m_objects[Thread::Current()] = newObj;
                    return newObj;
                }

                return it->second;
            }

            T* getWithoutCreating() {
                auto it = m_objects.find(Thread::Current());
                if (it == m_objects.end()) return nullptr;

                return it->second;
            }
        
        protected:
            std::function<T*()> m_creator;
            robin_hood::unordered_map<thread_id, T*> m_objects;
    };
};