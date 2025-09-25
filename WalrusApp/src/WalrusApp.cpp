#define WALRUS_ENABLE_EVENT_LOOP 1
#define WALRUS_ENABLE_PUBSUB 1
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/Config.h"

#include "Walrus/EventLoop.h"

#include "Walrus/PubSub.h"
#include "Walrus/InMemoryBroker.h"

#include <iostream>
#include <string>

// Data structure to be sent between objects
struct DataPacket {
    int id;
    std::string message;
    float timestamp;
    
    DataPacket(int id, const std::string& msg, float time) 
        : id(id), message(msg), timestamp(time) {}
};

// ObjectA - Sender that uses SetInterval to send data via PubSub
class ObjectA : public Walrus::Layer
{
private:
    Walrus::EventId m_IntervalId = 0;
    int m_Counter = 0;

public:
    virtual void OnAttach() override
    {
        std::cout << "\n=== ObjectA Attached ===" << std::endl;
        
        auto& app = Walrus::Application::Get();
        
        std::cout << "Starting interval to send data every 1000ms..." << std::endl;
        
        // Set up interval to send data packets every 1000ms
        m_IntervalId = app.SetInterval([this, &app]() {
            m_Counter++;
            
            // Create and send data packet
            DataPacket packet(m_Counter, "Message from ObjectA #" + std::to_string(m_Counter), app.GetTime());
            
            std::cout << "[ObjectA] Sending packet " << packet.id << ": '" << packet.message 
                     << "' at time " << packet.timestamp << std::endl;
            
            app.Publish<DataPacket>("data_channel", packet);
            
            // Stop after 5 messages
            if (m_Counter >= 5) {
                std::cout << "[ObjectA] Stopping interval after 5 messages" << std::endl;
                app.ClearInterval(m_IntervalId);
                
                // Schedule application shutdown
                app.SetTimeout([&app]() {
                    std::cout << "\n=== Demo Complete - Shutting Down ===" << std::endl;
                    app.Close();
                }, 2000);
            }
        }, 1000);
    }
    
    virtual void OnDetach() override
    {
        std::cout << "[ObjectA] Detached" << std::endl;
    }
};

// ObjectB - Receiver that subscribes to PubSub messages
class ObjectB : public Walrus::Layer
{
public:
    virtual void OnAttach() override
    {
        std::cout << "[ObjectB] Attached and subscribing to data_channel" << std::endl;
        
        auto& app = Walrus::Application::Get();
        
        // Subscribe to DataPacket messages on "data_channel"
        app.Subscribe<DataPacket>("data_channel", [](const Walrus::Message<DataPacket>& msg) {
            const DataPacket& packet = msg.GetData();
            std::cout << "[ObjectB] Received packet " << packet.id << ": '" << packet.message 
                     << "' (sent at " << packet.timestamp << ")" << std::endl;
        });
        
        std::cout << "[ObjectB] Successfully subscribed to data_channel" << std::endl;
    }
    
    virtual void OnDetach() override
    {
        std::cout << "[ObjectB] Detached" << std::endl;
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv)
{
    Walrus::ApplicationSpecification spec;
    spec.Name = "Simple SetInterval PubSub Demo";

    // Create and configure the InMemoryBroker for PubSub - will throw exception if PUBSUB not enabled
    spec.PubSubBroker = std::make_shared<Walrus::InMemoryBroker>();

    Walrus::Application* app = new Walrus::Application(spec);
    
    // Add both objects as layers
    app->PushLayer<ObjectB>();  // Add receiver first
    app->PushLayer<ObjectA>();  // Add sender second
    
    return app;
}