#pragma once
#include <utils/Types.h>
#include <utils/robin_hood.h>

// todo: fix gjs::TransientFunction so std::function isn't needed here
// note: The problem is that 'Args ...args' becomes garbage in the wrapped
//       lambda once 'Create' returns
#include <functional>

namespace utils {
    template <typename T>
    class Singleton {
        public:
            template <typename ...Args>
            static void Create(Args ...args);

            static void Destroy();

            static T* Get();

        private:
            static T* instance;
    };

    
    template <typename T>
    class PerThreadSingleton {
        public:
            template <typename ...Args>
            static void Create(Args&& ...args);

            // Destroys self and all per-thread instances
            static void Destroy();

            // Destroys _only_ the per-thread instance that corresponds to 'tid'
            static void Destroy(thread_id tid);

            static T* Get();

        private:
            static PerThreadSingleton<T>* instance;

            // Hash map of thread id -> PageAllocatorTp*
            robin_hood::unordered_map<thread_id, T*> m_instanceMap;
            std::function<T*()> m_newInstanceFunc;
    };
};