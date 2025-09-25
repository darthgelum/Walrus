#include "EventLoop.h"

#if WALRUS_ENABLE_EVENT_LOOP

#include <iostream>
#include <algorithm>
#include <thread>

namespace Walrus {

    EventLoop::EventLoop() 
        : m_TaskScheduler(nullptr),
          m_Running(false),
          m_NextId(1),
          m_TimerQueue([](const std::shared_ptr<FiberTimer>& a, const std::shared_ptr<FiberTimer>& b) {
            return a->nextExecution > b->nextExecution; // Min-heap based on execution time
        })
    {
    }

    EventLoop::~EventLoop() {
        Stop();
    }

    void EventLoop::Init(ftl::TaskScheduler* scheduler) {
        m_TaskScheduler = scheduler;
    }

    void EventLoop::Start() {
        if (m_Running.load() || !m_TaskScheduler) {
            return; // Already running or not initialized
        }
        
        m_Running.store(true);
        
        // Start the fiber-based event processing loop
        ftl::Task eventLoopTask = { 
            [](ftl::TaskScheduler*, void* arg) {
                static_cast<EventLoop*>(arg)->EventLoopFiber();
            }, 
            this 
        };
        m_TaskScheduler->AddTask(eventLoopTask, ftl::TaskPriority::Normal);
        
        std::cout << "EventLoop: Started with fiber-based scheduling" << std::endl;
    }

    void EventLoop::Stop() {
        if (!m_Running.load()) {
            return; // Already stopped
        }
        
        m_Running.store(false);
        std::cout << "EventLoop: Stopped" << std::endl;
    }

    EventId EventLoop::SetTimeout(EventCallback callback, int milliseconds) {
        if (!m_TaskScheduler) {
            std::cerr << "EventLoop: TaskScheduler not initialized!" << std::endl;
            return 0;
        }
        
        EventId id = GenerateId();
        auto now = std::chrono::steady_clock::now();
        auto executionTime = now + std::chrono::milliseconds(milliseconds);
        
        auto timerEvent = std::make_shared<FiberTimer>();
        timerEvent->id = id;
        timerEvent->callback = std::move(callback);
        timerEvent->nextExecution = executionTime;
        timerEvent->interval = std::chrono::milliseconds(0);
        timerEvent->repeat = false;
        timerEvent->cancelled.store(false);
        
        {
            std::lock_guard<std::mutex> lock(m_TimerMutex);
            m_TimerQueue.push(timerEvent);
            m_TimerMap[id] = timerEvent;
        }
        
        // Ensure EventLoop is running to process this new timer
        EnsureEventLoopRunning();
        
        return id;
    }

    EventId EventLoop::SetInterval(EventCallback callback, int milliseconds) {
        if (!m_TaskScheduler) {
            std::cerr << "EventLoop: TaskScheduler not initialized!" << std::endl;
            return 0;
        }
        
        EventId id = GenerateId();
        auto now = std::chrono::steady_clock::now();
        auto executionTime = now + std::chrono::milliseconds(milliseconds);
        
        auto timerEvent = std::make_shared<FiberTimer>();
        timerEvent->id = id;
        timerEvent->callback = std::move(callback);
        timerEvent->nextExecution = executionTime;
        timerEvent->interval = std::chrono::milliseconds(milliseconds);
        timerEvent->repeat = true;
        timerEvent->cancelled.store(false);
        
        {
            std::lock_guard<std::mutex> lock(m_TimerMutex);
            m_TimerQueue.push(timerEvent);
            m_TimerMap[id] = timerEvent;
        }
        
        // Ensure EventLoop is running to process this new timer
        EnsureEventLoopRunning();
        
        return id;
    }

    EventId EventLoop::SetImmediate(EventCallback callback) {
        if (!m_TaskScheduler) {
            std::cerr << "EventLoop: TaskScheduler not initialized!" << std::endl;
            return 0;
        }
        
        EventId id = GenerateId();
        
        // Execute immediately as a fiber task
        // We need to store the callback in a way that the C-style function can access it
        auto* callbackPtr = new EventCallback(std::move(callback));
        
        ftl::Task immediateTask = {
            [](ftl::TaskScheduler*, void* arg) {
                auto* cb = static_cast<EventCallback*>(arg);
                try {
                    (*cb)();
                } catch (const std::exception& e) {
                    std::cerr << "EventLoop: Exception in immediate callback: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "EventLoop: Unknown exception in immediate callback" << std::endl;
                }
                delete cb; // Clean up the allocated callback
            },
            callbackPtr
        };
        
        m_TaskScheduler->AddTask(immediateTask, ftl::TaskPriority::Normal);
        
        return id;
    }

    void EventLoop::ClearInterval(EventId id) {
        std::lock_guard<std::mutex> lock(m_TimerMutex);
        auto it = m_TimerMap.find(id);
        if (it != m_TimerMap.end()) {
            it->second->cancelled.store(true);
            m_TimerMap.erase(it);
        }
    }

    void EventLoop::ClearTimeout(EventId id) {
        ClearInterval(id); // Same implementation for both
    }

    bool EventLoop::IsRunning() const {
        return m_Running.load();
    }

    void EventLoop::EventLoopFiber() {
        ProcessTimerEvents();
        
        // Only reschedule if we're still running AND we have pending timers
        if (m_Running.load()) {
            bool hasMoreTimers = false;
            std::chrono::steady_clock::time_point nextTimerTime;
            
            {
                std::lock_guard<std::mutex> lock(m_TimerMutex);
                if (!m_TimerQueue.empty()) {
                    hasMoreTimers = true;
                    nextTimerTime = m_TimerQueue.top()->nextExecution;
                }
            }
            
            if (hasMoreTimers) {
                // Calculate delay until next timer
                auto now = std::chrono::steady_clock::now();
                auto delay = nextTimerTime > now ? 
                    std::chrono::duration_cast<std::chrono::milliseconds>(nextTimerTime - now) :
                    std::chrono::milliseconds(0);
                
                // Cap the delay to prevent excessive sleeping (max 100ms for responsiveness)
                if (delay > std::chrono::milliseconds(100)) {
                    delay = std::chrono::milliseconds(100);
                }
                
                // Create a delayed task that will process timers when they're ready
                auto* delayInfo = new std::pair<EventLoop*, std::chrono::milliseconds>(this, delay);
                
                ftl::Task delayedTask = {
                    [](ftl::TaskScheduler* scheduler, void* arg) {
                        auto* info = static_cast<std::pair<EventLoop*, std::chrono::milliseconds>*>(arg);
                        EventLoop* eventLoop = info->first;
                        auto delay = info->second;
                        delete info;
                        
                        // Sleep for the calculated delay (safe when we have multiple threads)
                        if (delay > std::chrono::milliseconds(0)) {
                            std::this_thread::sleep_for(delay);
                        }
                        
                        // Schedule the next EventLoop iteration
                        ftl::Task nextTask = {
                            [](ftl::TaskScheduler*, void* arg) {
                                static_cast<EventLoop*>(arg)->EventLoopFiber();
                            },
                            eventLoop
                        };
                        scheduler->AddTask(nextTask, ftl::TaskPriority::Normal);
                    },
                    delayInfo
                };
                
                m_TaskScheduler->AddTask(delayedTask, ftl::TaskPriority::Normal);
            }
            // If no more timers, stop scheduling - let the task queue become empty for CPU efficiency!
        }
    }

    void EventLoop::ProcessTimerEvents() {
        auto now = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(m_TimerMutex);
        
        while (!m_TimerQueue.empty() && m_TimerQueue.top()->nextExecution <= now) {
            auto event = m_TimerQueue.top();
            m_TimerQueue.pop();
            
            if (event->cancelled.load()) {
                continue;
            }
            
            // Execute callback as a fiber task
            // We need to store the callback in a way that the C-style function can access it
            auto* callbackPtr = new EventCallback(event->callback);
            
            ftl::Task timerTask = {
                [](ftl::TaskScheduler*, void* arg) {
                    auto* cb = static_cast<EventCallback*>(arg);
                    try {
                        (*cb)();
                    } catch (const std::exception& e) {
                        std::cerr << "EventLoop: Exception in timer callback: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "EventLoop: Unknown exception in timer callback" << std::endl;
                    }
                    delete cb; // Clean up the allocated callback
                },
                callbackPtr
            };
            
            m_TaskScheduler->AddTask(timerTask, ftl::TaskPriority::Normal);
            
            // If it's a repeating interval, reschedule it
            if (event->repeat && !event->cancelled.load()) {
                event->nextExecution = now + event->interval;
                m_TimerQueue.push(event);
            } else {
                // Remove from map if it's a timeout or cancelled
                m_TimerMap.erase(event->id);
            }
        }
    }

    EventId EventLoop::GenerateId() {
        return m_NextId.fetch_add(1);
    }

    void EventLoop::EnsureEventLoopRunning() {
        if (m_Running.load() && m_TaskScheduler) {
            // Check if there are any pending EventLoop tasks in the scheduler
            // If not, we need to start a new EventLoop iteration
            // For simplicity, we'll always schedule one - the TaskScheduler will handle duplicates efficiently
            
            ftl::Task eventLoopTask = { 
                [](ftl::TaskScheduler*, void* arg) {
                    static_cast<EventLoop*>(arg)->EventLoopFiber();
                }, 
                this 
            };
            m_TaskScheduler->AddTask(eventLoopTask, ftl::TaskPriority::Normal);
        }
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
    
    void EventLoop::Init(void*) { /* no-op */ }
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