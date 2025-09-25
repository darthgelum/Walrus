#define WALRUS_ENABLE_EVENT_LOOP 1
#define WALRUS_ENABLE_PUBSUB 1
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/EventLoop.h"
#include "Walrus/PubSub.h"
#include "Walrus/InMemoryBroker.h"

#include <ftl/task_scheduler.h>
#include <ftl/wait_group.h>

#include <iostream>

// Simple message for PubSub demonstration
struct SimpleMessage {
    int id;
    std::string text;
    
    SimpleMessage(int id, const std::string& text) : id(id), text(text) {}
};

// Minimal demo layer showcasing all three components
class CoreDemoLayer : public Walrus::Layer
{
private:
    Walrus::EventId m_TimerId = 0;
    int m_Counter = 0;

public:
    virtual void OnAttach() override
    {
        std::cout << "\n=== Core Demo: EventLoop + PubSub + TaskScheduler ===" << std::endl;
        
        auto& app = Walrus::Application::Get();
        
        // 1. Subscribe to messages (MessageBroker)
        app.Subscribe<SimpleMessage>("demo", [](const Walrus::Message<SimpleMessage>& msg) {
            const auto& data = msg.GetData();
            std::cout << "[PubSub] Received: ID=" << data.id << ", Text='" << data.text << "'" << std::endl;
        });
        
        // 2. Setup timer (EventLoop)
        m_TimerId = app.SetInterval([this, &app]() {
            m_Counter++;
            std::cout << "\n--- Timer Tick #" << m_Counter << " ---" << std::endl;
            
            // Publish message
            app.Publish<SimpleMessage>("demo", SimpleMessage(m_Counter, "Hello from tick " + std::to_string(m_Counter)));
            
            // 3. Run fiber task (TaskScheduler)
            this->RunFiberTask(app.GetTaskScheduler(), m_Counter);
            
            // Stop after 3 ticks
            if (m_Counter >= 3) {
                std::cout << "[EventLoop] Clearing timer" << std::endl;
                app.ClearInterval(m_TimerId);
                
                app.SetTimeout([&app]() {
                    std::cout << "\n=== Demo Complete ===\n" << std::endl;
                    app.Close();
                }, 500);
            }
        }, 1500); // Every 1.5 seconds
        
        std::cout << "[EventLoop] Timer started with ID: " << m_TimerId << std::endl;
    }
    
private:
    void RunFiberTask(ftl::TaskScheduler& scheduler, int taskId) {
        std::cout << "[TaskScheduler] Running fiber task #" << taskId << std::endl;
        
        // Create a simple computational task
        auto* taskData = new int(taskId);
        
        ftl::Task computeTask = {
            [](ftl::TaskScheduler*, void* arg) {
                int* id = static_cast<int*>(arg);
                int taskId = *id;
                delete id;
                
                // Simple computation
                int result = 0;
                for (int i = 0; i < 1000; i++) {
                    result += (taskId * i) % 100;
                }
                
                std::cout << "[TaskScheduler] Task #" << taskId << " computed result: " << result << std::endl;
            },
            taskData
        };
        
        // Run task and wait for completion
        ftl::WaitGroup wg(&scheduler);
        scheduler.AddTask(computeTask, ftl::TaskPriority::Normal, &wg);
        wg.Wait(false);
        
        std::cout << "[TaskScheduler] Task #" << taskId << " completed" << std::endl;
    }

public:
    virtual void OnDetach() override
    {
        std::cout << "[CoreDemoLayer] Detached after " << m_Counter << " ticks" << std::endl;
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv)
{
    // Use BackgroundService preset for efficiency
    Walrus::ApplicationSpecification spec = Walrus::ApplicationSpecification::BackgroundService();
    spec.Name = "Core Demo: EventLoop + PubSub + TaskScheduler";
    
    // Enable PubSub with InMemoryBroker
    spec.PubSubBroker = std::make_shared<Walrus::InMemoryBroker>();

    Walrus::Application* app = new Walrus::Application(spec);
    app->PushLayer<CoreDemoLayer>();
    
    return app;
}