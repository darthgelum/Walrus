#ifndef WALRUS_EVENTLOOP_H
#define WALRUS_EVENTLOOP_H

#include "Config.h"

#if WALRUS_ENABLE_EVENT_LOOP

#include <ftl/task_scheduler.h>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <queue>
#include <vector>

namespace Walrus {

    using EventCallback = std::function<void()>;
    using EventId = uint64_t;

    struct FiberTimer {
        FiberTimer() = default;
        FiberTimer(EventId id, EventCallback cb, std::chrono::steady_clock::time_point next,
                  std::chrono::milliseconds interval, bool repeat)
            : id(id), callback(cb), nextExecution(next), interval(interval), repeat(repeat), cancelled(false) {}
        
        EventId id = 0;
        EventCallback callback;
        std::chrono::steady_clock::time_point nextExecution;
        std::chrono::milliseconds interval = std::chrono::milliseconds(0);
        bool repeat = false;
        std::atomic<bool> cancelled{false};
    };

    class EventLoop {
    public:
        EventLoop();
        ~EventLoop();

        // Initialize with TaskScheduler reference
        void Init(ftl::TaskScheduler* scheduler);

        // Start the event loop (called automatically by Application)
        void Start();
        
        // Stop the event loop
        void Stop();

        // SetTimeout - execute callback once after delay
        EventId SetTimeout(EventCallback callback, int milliseconds);
        
        // SetInterval - execute callback repeatedly with interval
        EventId SetInterval(EventCallback callback, int milliseconds);
        
        // SetImmediate - execute callback as soon as possible in next event loop iteration
        EventId SetImmediate(EventCallback callback);
        
        // ClearInterval/ClearTimeout - cancel a timer by ID
        void ClearInterval(EventId id);
        void ClearTimeout(EventId id);
        
        // Check if event loop is running
        bool IsRunning() const;

    private:
        void EventLoopFiber();
        void ProcessTimerEvents();
        EventId GenerateId();
        void EnsureEventLoopRunning(); // Helper to restart EventLoop if needed

    private:
        ftl::TaskScheduler* m_TaskScheduler;
        std::atomic<bool> m_Running;
        std::atomic<EventId> m_NextId;
        
        // Timer management using fibers
        std::mutex m_TimerMutex;
        std::priority_queue<std::shared_ptr<FiberTimer>, 
                           std::vector<std::shared_ptr<FiberTimer>>,
                           std::function<bool(const std::shared_ptr<FiberTimer>&, const std::shared_ptr<FiberTimer>&)>> m_TimerQueue;
        std::unordered_map<EventId, std::shared_ptr<FiberTimer>> m_TimerMap;
        std::atomic<bool> m_TimerManagerRunning{false};
    };

} // namespace Walrus

#else // WALRUS_ENABLE_EVENT_LOOP == 0

// Stub declarations when EventLoop is disabled
#include <functional>
#include <cstdint>

namespace Walrus {
    
    using EventCallback = std::function<void()>;
    using EventId = uint64_t;
    
    class EventLoop {
    public:
        EventLoop();
        ~EventLoop();
        
        void Init(void* scheduler); // Stub - unused but keeps interface consistent
        void Start();
        void Stop();
        
        EventId SetTimeout(EventCallback callback, int milliseconds);
        EventId SetInterval(EventCallback callback, int milliseconds);
        EventId SetImmediate(EventCallback callback);
        void ClearInterval(EventId id);
        void ClearTimeout(EventId id);
        
        bool IsRunning() const;
    };
    
} // namespace Walrus

#endif // WALRUS_ENABLE_EVENT_LOOP

#endif // WALRUS_EVENTLOOP_H