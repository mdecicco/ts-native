#pragma once
#include <utils/Types.h>
#include <utils/Singleton.h>
#include <utils/Array.h>
#include <utils/Allocator.h>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace utils {
    class Thread {
        public:
            Thread();
            ~Thread();

            thread_id getId() const;
            void reset(const std::function<void()>& entry);
            void setAffinity(u32 cpuIdx);

            static thread_id Current();
            static void Sleep(u32 ms);
            static u32 MaxHardwareThreads();
            static u32 CurrentCpuIndex();

        protected:
            std::thread m_thread;
    };

    class Job {
        public:
            Job(std::function<void()> cb);
            ~Job();

            void run();

        protected:
            std::function<void()> m_cb;
    };

    typedef u32 WorkerId;
    class Worker {
        public:

        protected:
            friend class sThreadPool;
            Worker();
            ~Worker();

            void start(WorkerId id, u32 cpuIdx);
            void run();

            WorkerId m_id;
            bool m_doStop;
            Thread m_thread;
    };

    class sThreadPool {
        public:
            template <typename T, typename F>
            void processArray(Array<T>& arr, u32 batchSize, F&& cb);
            template <typename T, typename F>
            void processArray(Array<T>& arr, u32 minBatchSize, u32 maxBatchSize, F&& cb);

        protected:
            friend class Singleton<sThreadPool>;
            friend class Worker;

            sThreadPool();
            ~sThreadPool();

            Job* getWork();
            void release(Job* j);
            bool hasWork() const;
            void waitForWork(Worker* w);

        private:
            Worker* m_workers;
            Array<Job*> m_pending;

            std::mutex m_jobMutex;
            std::condition_variable m_workCondition;
            PagedAllocator<Job, FixedAllocator<Job>> m_jobAllocator;
    };

    typedef Singleton<sThreadPool> ThreadPool;
};