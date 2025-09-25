#include "EventLoop.h"

#if WALRUS_ENABLE_EVENT_LOOP

#include <iostream>
#include <algorithm>

namespace Walrus {

    EventLoop::EventLoop() 
        : m_TimerQueue([](const std::shared_ptr<TimerEvent>& a, const std::shared_ptr<TimerEvent>& b) {
            return a->nextExecution > b->nextExecution; // Min-heap based on execution time
        })
    {
        // Initialize thread pool (4 threads for parallel execution)
        const size_t numThreads = std::max(2u, std::thread::hardware_concurrency());
        
        for (size_t i = 0; i < numThreads; ++i) {
            m_ThreadPool.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(m_TaskMutex);
                        m_TaskCondition.wait(lock, [this] { return !m_TaskQueue.empty() || m_StopThreads.load(); });
                        
                        if (m_StopThreads.load() && m_TaskQueue.empty()) {
                            break;
                        }
                        
                        if (!m_TaskQueue.empty()) {
                            task = std::move(m_TaskQueue.front());
                            m_TaskQueue.pop();
                        }
                    }
                    
                    if (task) {
                        try {
                            task();
                        } catch (const std::exception& e) {
                            std::cerr << "EventLoop: Exception in callback: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << "EventLoop: Unknown exception in callback" << std::endl;
                        }
                    }
                }
            });
        }
    }

    EventLoop::~EventLoop() {
        Stop();
    }

    void EventLoop::Start() {
        if (m_Running.load()) {
            return; // Already running
        }
        
        m_Running.store(true);
        m_EventThread = std::thread(&EventLoop::EventLoopThread, this);
        std::cout << "EventLoop: Started with " << m_ThreadPool.size() << " worker threads" << std::endl;
    }

    void EventLoop::Stop() {
        if (!m_Running.load()) {
            return; // Already stopped
        }
        
        m_Running.store(false);
        m_EventCondition.notify_all();
        
        if (m_EventThread.joinable()) {
            m_EventThread.join();
        }
        
        // Stop thread pool
        m_StopThreads.store(true);
        m_TaskCondition.notify_all();
        
        for (auto& thread : m_ThreadPool) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        std::cout << "EventLoop: Stopped" << std::endl;
    }

    EventId EventLoop::SetTimeout(EventCallback callback, int milliseconds) {
        EventId id = GenerateId();
        auto now = std::chrono::steady_clock::now();
        auto executionTime = now + std::chrono::milliseconds(milliseconds);
        
        auto timerEvent = std::make_shared<TimerEvent>(id, std::move(callback), executionTime, std::chrono::milliseconds(0), false);
        
        {
            std::lock_guard<std::mutex> lock(m_TimerMutex);
            m_TimerQueue.push(timerEvent);
            m_TimerMap[id] = timerEvent;
        }
        
        m_EventCondition.notify_one();
        return id;
    }

    EventId EventLoop::SetInterval(EventCallback callback, int milliseconds) {
        EventId id = GenerateId();
        auto now = std::chrono::steady_clock::now();
        auto executionTime = now + std::chrono::milliseconds(milliseconds);
        
        auto timerEvent = std::make_shared<TimerEvent>(id, std::move(callback), executionTime, std::chrono::milliseconds(milliseconds), true);
        
        {
            std::lock_guard<std::mutex> lock(m_TimerMutex);
            m_TimerQueue.push(timerEvent);
            m_TimerMap[id] = timerEvent;
        }
        
        m_EventCondition.notify_one();
        return id;
    }

    EventId EventLoop::SetImmediate(EventCallback callback) {
        EventId id = GenerateId();
        auto immediateEvent = std::make_shared<ImmediateEvent>(id, std::move(callback));
        
        {
            std::lock_guard<std::mutex> lock(m_ImmediateMutex);
            m_ImmediateQueue.push(immediateEvent);
            m_ImmediateMap[id] = immediateEvent;
        }
        
        m_EventCondition.notify_one();
        return id;
    }

    void EventLoop::ClearInterval(EventId id) {
        // Mark timer event as cancelled
        {
            std::lock_guard<std::mutex> lock(m_TimerMutex);
            auto it = m_TimerMap.find(id);
            if (it != m_TimerMap.end()) {
                it->second->cancelled = true;
                m_TimerMap.erase(it);
                return;
            }
        }
        
        // Mark immediate event as cancelled
        {
            std::lock_guard<std::mutex> lock(m_ImmediateMutex);
            auto it = m_ImmediateMap.find(id);
            if (it != m_ImmediateMap.end()) {
                it->second->cancelled = true;
                m_ImmediateMap.erase(it);
            }
        }
    }

    void EventLoop::EventLoopThread() {
        while (m_Running.load()) {
            ProcessImmediateEvents();
            ProcessTimerEvents();
            
            // Wait for next event or timeout
            std::unique_lock<std::mutex> lock(m_EventMutex);
            m_EventCondition.wait_for(lock, std::chrono::milliseconds(1), [this] {
                return !m_Running.load() || 
                       (!m_ImmediateQueue.empty()) ||
                       (!m_TimerQueue.empty() && m_TimerQueue.top()->nextExecution <= std::chrono::steady_clock::now());
            });
        }
    }

    void EventLoop::ProcessTimerEvents() {
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(m_TimerMutex);
        
        while (!m_TimerQueue.empty() && m_TimerQueue.top()->nextExecution <= now) {
            auto event = m_TimerQueue.top();
            m_TimerQueue.pop();
            
            if (event->cancelled) {
                continue;
            }
            
            // Schedule callback execution in thread pool
            {
                std::lock_guard<std::mutex> taskLock(m_TaskMutex);
                m_TaskQueue.push([callback = event->callback]() {
                    callback();
                });
            }
            m_TaskCondition.notify_one();
            
            // If it's a repeating interval, reschedule it
            if (event->repeat && !event->cancelled) {
                event->nextExecution = now + event->interval;
                m_TimerQueue.push(event);
            } else {
                // Remove from map if it's a timeout or cancelled
                m_TimerMap.erase(event->id);
            }
        }
    }

    void EventLoop::ProcessImmediateEvents() {
        std::lock_guard<std::mutex> lock(m_ImmediateMutex);
        
        while (!m_ImmediateQueue.empty()) {
            auto event = m_ImmediateQueue.front();
            m_ImmediateQueue.pop();
            
            if (event->cancelled) {
                continue;
            }
            
            // Schedule callback execution in thread pool
            {
                std::lock_guard<std::mutex> taskLock(m_TaskMutex);
                m_TaskQueue.push([callback = event->callback]() {
                    callback();
                });
            }
            m_TaskCondition.notify_one();
            
            // Remove from map
            m_ImmediateMap.erase(event->id);
        }
    }

    EventId EventLoop::GenerateId() {
        return m_NextId.fetch_add(1);
    }

} // namespace Walrus

#else // WALRUS_ENABLE_EVENT_LOOP == 0

// Stub implementations when EventLoop is disabled
#include <functional>
#include <cstdint>

namespace Walrus {
    
    // Stub implementations - implement the methods declared in the header
    EventLoop::EventLoop() {}
    EventLoop::~EventLoop() {}
    
    void EventLoop::Start() { /* no-op */ }
    void EventLoop::Stop() { /* no-op */ }
    
    EventId EventLoop::SetTimeout(EventCallback, int) { return 0; }
    EventId EventLoop::SetInterval(EventCallback, int) { return 0; }
    EventId EventLoop::SetImmediate(EventCallback) { return 0; }
    void EventLoop::ClearInterval(EventId) { /* no-op */ }
    void EventLoop::ClearTimeout(EventId) { /* no-op */ }
    
    bool EventLoop::IsRunning() const { return false; }
    
} // namespace Walrus

#endif // WALRUS_ENABLE_EVENT_LOOP