#ifndef WALRUS_EVENTLOOP_H
#define WALRUS_EVENTLOOP_H

#include "Config.h"

#if WALRUS_ENABLE_EVENT_LOOP

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <memory>

namespace Walrus {

    using EventCallback = std::function<void()>;
    using EventId = uint64_t;

    struct TimerEvent {
        EventId id;
        EventCallback callback;
        std::chrono::steady_clock::time_point nextExecution;
        std::chrono::milliseconds interval;
        bool repeat;
        bool cancelled;

        TimerEvent(EventId id, EventCallback cb, std::chrono::steady_clock::time_point next, 
                  std::chrono::milliseconds interval = std::chrono::milliseconds(0), bool repeat = false)
            : id(id), callback(std::move(cb)), nextExecution(next), interval(interval), repeat(repeat), cancelled(false) {}
    };

    struct ImmediateEvent {
        EventId id;
        EventCallback callback;
        bool cancelled;

        ImmediateEvent(EventId id, EventCallback cb)
            : id(id), callback(std::move(cb)), cancelled(false) {}
    };

    class EventLoop {
    public:
        EventLoop();
        ~EventLoop();

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
        void ClearTimeout(EventId id) { ClearInterval(id); } // Same implementation
        
        // Check if event loop is running
        bool IsRunning() const { return m_Running.load(); }

    private:
        void EventLoopThread();
        void ProcessTimerEvents();
        void ProcessImmediateEvents();
        EventId GenerateId();

    private:
        std::atomic<bool> m_Running{false};
        std::thread m_EventThread;
        
        // Timer events management
        std::mutex m_TimerMutex;
        std::priority_queue<std::shared_ptr<TimerEvent>, 
                           std::vector<std::shared_ptr<TimerEvent>>,
                           std::function<bool(const std::shared_ptr<TimerEvent>&, const std::shared_ptr<TimerEvent>&)>> m_TimerQueue;
        std::unordered_map<EventId, std::shared_ptr<TimerEvent>> m_TimerMap;
        
        // Immediate events management
        std::mutex m_ImmediateMutex;
        std::queue<std::shared_ptr<ImmediateEvent>> m_ImmediateQueue;
        std::unordered_map<EventId, std::shared_ptr<ImmediateEvent>> m_ImmediateMap;
        
        // Thread pool for parallel callback execution
        std::vector<std::thread> m_ThreadPool;
        std::queue<std::function<void()>> m_TaskQueue;
        std::mutex m_TaskMutex;
        std::condition_variable m_TaskCondition;
        std::atomic<bool> m_StopThreads{false};
        
        // ID generation
        std::atomic<EventId> m_NextId{1};
        
        // Condition variable for event loop timing
        std::condition_variable m_EventCondition;
        std::mutex m_EventMutex;
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