#pragma once
#include <utils/Thread.h>
#include <utils/Singleton.hpp>
#include <utils/Array.hpp>
#include <utils/Allocator.hpp>

namespace utils {
    //
    // sThreadPool
    //

    template <typename T, typename F>
    void sThreadPool::processArray(Array<T>& arr, u32 batchSize, F&& cb) {
        static_assert(std::is_invocable_v<F, ArrayView<T>&>, "ThreadPool::processArray<T, F>: F must be a callable object with the signature void (ArrayView<T>&)");
        if (batchSize > arr.size()) {
            ArrayView<T> v = arr.view(0, arr.size());
            cb(v);
        }
        else {
            std::atomic<u32> count = 0;
            std::condition_variable cv;
            std::mutex m;

            m_jobMutex.lock();
            for (u32 i = 0;i < arr.size();i += batchSize, count++) {
                m_pending.push(new (m_jobAllocator.alloc(1)) Job(
                    [&arr, i, batchSize, cb, &cv, &m, &count]{
                        u32 viewSize = batchSize;
                        if (i + batchSize >= arr.size()) viewSize = arr.size() - i;
                        ArrayView<T> v = arr.view(i, viewSize);
                        cb(v);
                        std::lock_guard<std::mutex> l(m);
                        count--;
                        cv.notify_all();
                    }
                ));
            }
            m_jobMutex.unlock();
            m_workCondition.notify_all();

            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&count]{ return count == 0; });
        }
    }

    template <typename T, typename F>
    void sThreadPool::processArray(Array<T>& arr, u32 minBatchSize, u32 maxBatchSize, F&& cb) {
        static_assert(std::is_invocable_v<F, ArrayView<T>&>, "ThreadPool::processArray<T, F>: F must be a callable object with the signature void (ArrayView<T>&)");

        u32 batchSize = arr.size() / Thread::MaxHardwareThreads();
        if (batchSize < minBatchSize) batchSize = minBatchSize;
        else if (batchSize > maxBatchSize) batchSize = maxBatchSize;

        if (batchSize > arr.size()) {
            ArrayView<T> v = arr.view(0, arr.size());
            cb(v);
        }
        else {
            std::atomic<u32> count = 0;
            std::condition_variable cv;
            std::mutex m;

            m_jobMutex.lock();
            for (u32 i = 0;i < arr.size();i += batchSize, count++) {
                m_pending.push(new (m_jobAllocator.alloc(1)) Job(
                    [&arr, i, batchSize, cb, &cv, &m, &count]{
                        u32 viewSize = batchSize;
                        if (i + batchSize >= arr.size()) viewSize = arr.size() - i;
                        ArrayView<T> v = arr.view(i, viewSize);
                        cb(v);
                        std::lock_guard<std::mutex> l(m);
                        count--;
                        cv.notify_all();
                    }
                ));
            }
            m_jobMutex.unlock();
            m_workCondition.notify_all();

            std::unique_lock<std::mutex> l(m);
            cv.wait(l, [&count]{ return count == 0; });
        }
    }
};