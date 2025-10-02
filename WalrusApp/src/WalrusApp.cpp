#define WALRUS_ENABLE_EVENT_LOOP 1
#define WALRUS_ENABLE_PUBSUB 1
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/EventLoop.h"
#include "Walrus/PubSub.h"
#include "Walrus/InMemoryBroker.h"
#include "Walrus/ExampleLayers.h"
#include "Walrus/LayerTree.h"

#include <ftl/task_scheduler.h>
#include <ftl/wait_group.h>

#include <iostream>

// N-ary Tree Demo Layer that stops the application after a few seconds
class TreeDemoLayer : public Walrus::Layer
{
private:
    Walrus::EventId m_TimerId = 0;
    int m_UpdateCount = 0;

public:
    virtual void OnAttach() override
    {
        std::cout << "\n=== N-ary Tree Layer System Demo ===" << std::endl;
        std::cout << "This demonstrates hierarchical layer updates with parallel execution." << std::endl;
        std::cout << "- Layers at the same tree level run in parallel using fibers" << std::endl;
        std::cout << "- Parent layers wait for all children to complete before continuing" << std::endl;
        std::cout << "- Tree structure is printed at startup" << std::endl;
        
        auto& app = Walrus::Application::Get();
        
        // Set a timer to stop the demo after 5 seconds
        m_TimerId = app.SetTimeout([&app]() {
            std::cout << "\n=== Demo Complete - Stopping Application ===\n" << std::endl;
            app.Close();
        }, 5000); // 5 seconds
        
        std::cout << "[TreeDemoLayer] Demo will run for 5 seconds..." << std::endl;
    }

    virtual void OnUpdate(float ts) override
    {
        m_UpdateCount++;
        // Print occasionally to show the demo layer is running
        if (m_UpdateCount % 120 == 0) { // Every ~2 seconds at 60 FPS
            std::cout << "[TreeDemoLayer] Demo running... (update #" << m_UpdateCount << ")" << std::endl;
        }
    }

    virtual void OnDetach() override
    {
        std::cout << "[TreeDemoLayer] Demo completed after " << m_UpdateCount << " updates" << std::endl;
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv)
{
    // Use High Performance preset to demonstrate parallel processing
    Walrus::ApplicationSpecification spec = Walrus::ApplicationSpecification::HighPerformance();
    spec.Name = "N-ary Tree Layer System Demo";
    spec.TargetFPS = 60.0f; // 60 FPS for smooth updates
    
    // Enable PubSub with InMemoryBroker
    spec.PubSubBroker = std::make_shared<Walrus::InMemoryBroker>();

    Walrus::Application* app = new Walrus::Application(spec);
    
    // Create a complex n-ary tree structure to demonstrate parallel updates
    // Tree structure:
    //                    Root
    //                /    |    \
    //         RenderSystem |  AudioSystem
    //          /    |     |    |    \
    //       UI   Physics  |  Sound Effects
    //      /|\     |      |      |
    //   Btn1 Btn2 Collision   Footsteps
    //   |    |     |          |
    // Click Hover Detect    Volume
    
    auto builder = app->CreateLayerTreeBuilder();
    
    // Build the tree using the fluent API
    auto tree = builder->Root(std::make_shared<Walrus::ExampleLayer>("RenderSystem", 1), "render")
            // UI subsystem
            .Child(std::make_shared<Walrus::ExampleLayer>("UI", 0), "ui")
                .Child(std::make_shared<Walrus::ExampleLayer>("Button1", 0), "btn1")
                    .Child(std::make_shared<Walrus::HeavyComputeLayer>("ClickHandler", 100000), "click")
                .Back() // back to Button1
                .Child(std::make_shared<Walrus::ExampleLayer>("HoverHandler", 0), "hover")
            .Back() // back to UI
            .Child(std::make_shared<Walrus::ExampleLayer>("Button2", 0), "btn2")
                .Child(std::make_shared<Walrus::ExampleLayer>("HoverHandler2", 0), "hover2")
        .ToRoot() // back to render system
            // Physics subsystem  
            .Child(std::make_shared<Walrus::ExampleLayer>("Physics", 1), "physics")
                .Child(std::make_shared<Walrus::HeavyComputeLayer>("CollisionDetection", 200000), "collision")
                    .Child(std::make_shared<Walrus::ExampleLayer>("CollisionResponse", 0), "response")
        .ToRoot() // back to root level
        // Another root level system
        .Root(std::make_shared<Walrus::ExampleLayer>("AudioSystem", 0), "audio")
            .Child(std::make_shared<Walrus::ExampleLayer>("SoundEffects", 0), "sfx")
                .Child(std::make_shared<Walrus::ExampleLayer>("Footsteps", 0), "footsteps")
                    .Child(std::make_shared<Walrus::ExampleLayer>("VolumeControl", 0), "volume")
        .ToRoot()
        // Third root level system (lightweight)
        .Root(std::make_shared<Walrus::ExampleLayer>("NetworkSystem", 1), "network")
            .Child(std::make_shared<Walrus::ExampleLayer>("PacketHandler", 0), "packets")
            .Child(std::make_shared<Walrus::ExampleLayer>("ConnectionManager", 0), "connections")
        .Build();
    
    // Replace the application's layer tree with our complex structure
    app->SetLayerTree(tree);
    
    // Add the demo control layer as a simple root layer
    app->PushLayerAsRoot(std::make_shared<TreeDemoLayer>(), "demo_control");
    
    return app;
}