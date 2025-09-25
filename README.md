# Walrus Framework 🦭

A lightweight, console-based C++ application framework featuring layer system, EventLoop for asynchronous programming, and PubSub messaging.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Layer System](#layer-system)
- [EventLoop System](#eventloop-system)
- [PubSub Messaging](#pubsub-messaging)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [Building](#building)

## Overview

Walrus is a modern C++ framework for console applications with structured architecture, asynchronous capabilities, and type-safe messaging. It provides configurable EventLoop and PubSub systems that can be enabled/disabled at build time.

## Features

### 🎯 **Core Framework**
- **Layer-based Architecture**: Organize code into reusable, composable layers
- **Application Lifecycle**: Structured initialization, update, and shutdown
- **Cross-platform**: Works on Windows, Linux, and macOS
- **Configurable Build**: Enable/disable features via CMake

### ⚡ **EventLoop System** (Optional)
- **SetTimeout**: Execute callbacks once after a delay
- **SetInterval**: Execute callbacks repeatedly at intervals
- **SetImmediate**: Execute callbacks as soon as possible
- **Thread Pool**: Parallel execution with automatic scaling
- **Thread Safety**: All operations are thread-safe

### 📬 **PubSub Messaging** (Optional)
- **Type-Safe Messages**: Templated message system with compile-time safety
- **Topic-Based Routing**: Organize messages by topics
- **Multiple Subscribers**: Many-to-many messaging patterns
- **Configurable Brokers**: Pluggable broker implementations
- **Thread-Safe Operations**: Concurrent publish/subscribe support

## Quick Start

### 1. Simple EventLoop + PubSub Example

```cpp
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/InMemoryBroker.h"

struct MyData {
    int id;
    std::string message;
};

class SenderLayer : public Walrus::Layer {
    Walrus::EventId m_IntervalId = 0;
    int m_Counter = 0;
    
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        
        // Send data every 1000ms
        m_IntervalId = app.SetInterval([this, &app]() {
            MyData data{++m_Counter, "Message #" + std::to_string(m_Counter)};
            app.Publish<MyData>("my_topic", data);
            
            if (m_Counter >= 5) {
                app.ClearInterval(m_IntervalId);
                app.SetTimeout([&app]() { app.Close(); }, 1000);
            }
        }, 1000);
    }
};

class ReceiverLayer : public Walrus::Layer {
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        
        // Subscribe to messages
        app.Subscribe<MyData>("my_topic", [](const Walrus::Message<MyData>& msg) {
            const MyData& data = msg.GetData();
            std::cout << "Received: " << data.id << " - " << data.message << std::endl;
        });
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv) {
    Walrus::ApplicationSpecification spec;
    spec.Name = "EventLoop + PubSub Demo";
    spec.PubSubBroker = std::make_shared<Walrus::InMemoryBroker>();

    auto* app = new Walrus::Application(spec);
    app->PushLayer<ReceiverLayer>();
    app->PushLayer<SenderLayer>();
    return app;
}
```

### 2. Build and Run

```bash
mkdir build && cd build
cmake -DWALRUS_ENABLE_EVENT_LOOP=ON -DWALRUS_ENABLE_PUBSUB=ON ..
cmake --build .
./bin/WalrusApp
```

## Layer System

Organize your application into modular, reusable components.

```cpp
class GameLayer : public Walrus::Layer {
public:
    virtual void OnAttach() override {
        std::cout << "Game initialized!" << std::endl;
    }
    
    virtual void OnUpdate(float deltaTime) override {
        // Called every frame - keep lightweight
    }
    
    virtual void OnDetach() override {
        std::cout << "Game cleanup!" << std::endl;
    }
};

// Add to application
app->PushLayer<GameLayer>();
```

## EventLoop System

JavaScript-like asynchronous programming with true parallel execution.

```cpp
auto& app = Walrus::Application::Get();

// Execute once after delay
EventId timeoutId = app.SetTimeout([]() {
    std::cout << "Delayed execution!" << std::endl;
}, 2000);

// Execute repeatedly
EventId intervalId = app.SetInterval([]() {
    std::cout << "Repeated execution!" << std::endl;
}, 1000);

// Execute ASAP
app.SetImmediate([]() {
    std::cout << "Immediate execution!" << std::endl;
});

// Cancel timers
app.ClearTimeout(timeoutId);
app.ClearInterval(intervalId);
```

## PubSub Messaging

Type-safe publish/subscribe messaging system for decoupled communication.

### Basic Usage

```cpp
// Define your message type
struct PlayerEvent {
    int playerId;
    std::string action;
    float timestamp;
};

// Subscribe to messages
app.Subscribe<PlayerEvent>("player_events", [](const Walrus::Message<PlayerEvent>& msg) {
    const PlayerEvent& event = msg.GetData();
    std::cout << "Player " << event.playerId << " performed: " << event.action << std::endl;
});

// Publish messages
PlayerEvent event{1, "jump", app.GetTime()};
app.Publish<PlayerEvent>("player_events", event);
```

### Advanced Patterns

#### Multiple Subscribers
```cpp
// Multiple objects can subscribe to the same topic
class UI : public Walrus::Layer {
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        app.Subscribe<GameEvent>("game", [](const auto& msg) {
            // Update UI based on game events
        });
    }
};

class AudioSystem : public Walrus::Layer {
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        app.Subscribe<GameEvent>("game", [](const auto& msg) {
            // Play sounds based on game events
        });
    }
};
```

#### Topic Organization
```cpp
// Use descriptive topic names
app.Subscribe<PlayerEvent>("player/movement", handler);
app.Subscribe<PlayerEvent>("player/combat", handler);
app.Subscribe<SystemEvent>("system/error", handler);
app.Subscribe<SystemEvent>("system/debug", handler);

// Publish to specific topics
app.Publish<PlayerEvent>("player/movement", moveEvent);
app.Publish<SystemEvent>("system/error", errorEvent);
```

#### Combining with EventLoop
```cpp
class DataProcessor : public Walrus::Layer {
    Walrus::EventId m_ProcessorId;
    
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        
        // Subscribe to raw data
        app.Subscribe<RawData>("input", [&app](const auto& msg) {
            // Process and republish
            ProcessedData result = processData(msg.GetData());
            app.Publish<ProcessedData>("output", result);
        });
        
        // Periodic status updates
        m_ProcessorId = app.SetInterval([&app]() {
            StatusUpdate status{getSystemStatus()};
            app.Publish<StatusUpdate>("system/status", status);
        }, 5000);
    }
    
    virtual void OnDetach() override {
        auto& app = Walrus::Application::Get();
        app.ClearInterval(m_ProcessorId);
    }
};
```

### Custom Brokers

Implement your own broker for specialized needs:

```cpp
class NetworkBroker : public Walrus::IBroker {
public:
    void Start() override {
        // Initialize network connections
    }
    
    void Stop() override {
        // Cleanup network resources
    }
    
    // Implement required virtual methods...
};

// Use custom broker
Walrus::ApplicationSpecification spec;
spec.PubSubBroker = std::make_shared<NetworkBroker>();
```

## Configuration

Control which features are compiled into your application:

```bash
# Enable both EventLoop and PubSub (default)
cmake -DWALRUS_ENABLE_EVENT_LOOP=ON -DWALRUS_ENABLE_PUBSUB=ON ..

# EventLoop only
cmake -DWALRUS_ENABLE_EVENT_LOOP=ON -DWALRUS_ENABLE_PUBSUB=OFF ..

# PubSub only  
cmake -DWALRUS_ENABLE_EVENT_LOOP=OFF -DWALRUS_ENABLE_PUBSUB=ON ..

# Basic framework only
cmake -DWALRUS_ENABLE_EVENT_LOOP=OFF -DWALRUS_ENABLE_PUBSUB=OFF ..
```

### Runtime Behavior

- **Features Enabled**: Full functionality available
- **Features Disabled**: Compile-time errors if you try to use disabled features  
- **No Runtime Checks**: Clean, fast code - errors caught at compile time

## API Reference

### Application Class

```cpp
class Application {
public:
    // Core
    static Application& Get();
    void Run();
    void Close();
    float GetTime();
    
    // Layer Management
    template<typename T> void PushLayer();
    void PushLayer(const std::shared_ptr<Layer>& layer);
    
    // EventLoop (when enabled)
    EventId SetTimeout(EventCallback callback, int milliseconds);
    EventId SetInterval(EventCallback callback, int milliseconds);
    EventId SetImmediate(EventCallback callback);
    void ClearInterval(EventId id);
    void ClearTimeout(EventId id);
    
    // PubSub (when enabled)
    template<typename T> void Subscribe(const std::string& topic, MessageHandler<T> handler);
    template<typename T> void Publish(const std::string& topic, const T& data);
    void UnsubscribeFromTopic(const std::string& topic);
};
```

### ApplicationSpecification

```cpp
struct ApplicationSpecification {
    std::string Name = "Walrus App";
    std::shared_ptr<IBroker> PubSubBroker = nullptr;  // When PubSub enabled
};
```

### Layer Interface

```cpp
class Layer {
public:
    virtual ~Layer() = default;
    virtual void OnAttach() {}    // Override to initialize
    virtual void OnDetach() {}    // Override to cleanup
    virtual void OnUpdate(float deltaTime) {}  // Override for per-frame logic
};
```

### PubSub Types

```cpp
// Message wrapper
template<typename T>
class Message {
public:
    const T& GetData() const;
    const std::string& GetTopic() const;
};

// Message handler callback
template<typename T>
using MessageHandler = std::function<void(const Message<T>&)>;

// Broker interface for custom implementations
class IBroker {
public:
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;
    // Subscribe/Publish methods available via templates
};
```

## Building

### Requirements
- **C++17 or later**
- **CMake 3.15+**
- **GCC, Clang, or MSVC**

### Quick Build
```bash
git clone <repository-url>
cd Walnut
mkdir build && cd build
cmake -DWALRUS_ENABLE_EVENT_LOOP=ON -DWALRUS_ENABLE_PUBSUB=ON ..
cmake --build .
./bin/WalrusApp
```

### Project Structure
```
Walnut/
├── CMakeLists.txt
├── Walrus/                    # Framework library
│   └── src/Walrus/
│       ├── Application.h/.cpp
│       ├── Layer.h
│       ├── EventLoop.h/.cpp
│       ├── PubSub.h
│       ├── InMemoryBroker.h
│       └── Config.h
└── WalrusApp/                 # Example application
    └── src/WalrusApp.cpp
```

#### Self-Managing Intervals
```cpp
class DataProcessor : public Walrus::Layer {
private:
    Walrus::EventId m_ProcessorId = 0;
    int m_ItemsProcessed = 0;
    
public:
    virtual void OnAttach() override {
        auto& app = Walrus::Application::Get();
        
        m_ProcessorId = app.SetInterval([this]() {
            ProcessNextItem();
            m_ItemsProcessed++;
            
            if (m_ItemsProcessed >= 100) {
                std::cout << "Processing complete!" << std::endl;
                Walrus::Application::Get().ClearInterval(m_ProcessorId);
            }
        }, 100); // Process every 100ms
    }
    
private:
    void ProcessNextItem() {
        std::cout << "Processing item " << m_ItemsProcessed + 1 << std::endl;
    }
};
```

#### Parallel Processing
```cpp
void ProcessFilesInParallel() {
    auto& app = Walrus::Application::Get();
    
    // All execute in parallel threads
    app.SetImmediate([]() { ProcessFile("file1.txt"); });
    app.SetImmediate([]() { ProcessFile("file2.txt"); });
    app.SetImmediate([]() { ProcessFile("file3.txt"); });
    
    // Check completion after delay
    app.SetTimeout([]() {
        std::cout << "All files should be processed!" << std::endl;
    }, 5000);
}
```

### EventLoop Features

- 🚀 **High Performance**: Thread pool with hardware concurrency detection
- 🔒 **Thread Safe**: All operations can be called from any thread
- ⚡ **Parallel Execution**: True multi-threaded callback execution  
- 🛡️ **Exception Safe**: Callbacks that throw are safely handled
- 🧹 **Auto Cleanup**: Resources automatically freed on shutdown
- ⏱️ **Precise Timing**: High-resolution timer backend

## API Reference

### Application Class

```cpp
class Application {
public:
    // Lifecycle
    static Application& Get();
    void Run();
    void Close();
    float GetTime();
    
    // Layer Management  
    template<typename T> void PushLayer();
    void PushLayer(const std::shared_ptr<Layer>& layer);
    
    // EventLoop Access
    EventLoop& GetEventLoop();
    
    // Timer Functions
    EventId SetTimeout(EventCallback callback, int milliseconds);
    EventId SetInterval(EventCallback callback, int milliseconds);  
    EventId SetImmediate(EventCallback callback);
    void ClearInterval(EventId id);
    void ClearTimeout(EventId id);
};
```

### Layer Class

```cpp
class Layer {
public:
    virtual ~Layer() = default;
    virtual void OnAttach() {}     // Override to initialize
    virtual void OnDetach() {}     // Override to cleanup
    virtual void OnUpdate(float deltaTime) {}  // Override for per-frame logic
};
```

### Timer Utility

```cpp
class Timer {
public:
    Timer();
    void Reset();
    float Elapsed();           // Seconds since reset/construction
    float ElapsedMillis();     // Milliseconds since reset/construction
};
```

### Random Utility

```cpp
class Random {
public:
    static void Init();
    static uint32_t UInt();
    static uint32_t UInt(uint32_t min, uint32_t max);
    static float Float();      // 0.0f to 1.0f
    static float Float(float min, float max);
};
```

## Examples

### Complete Application Example

```cpp
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/Timer.h"

class LoggingLayer : public Walrus::Layer {
public:
    virtual void OnAttach() override {
        std::cout << "[LOG] Application started" << std::endl;
    }
    
    virtual void OnDetach() override {
        std::cout << "[LOG] Application ended" << std::endl;  
    }
};

class GameLogicLayer : public Walrus::Layer {
private:
    Walrus::Timer m_GameTimer;
    Walrus::EventId m_UpdateId = 0;
    int m_Score = 0;
    
public:
    virtual void OnAttach() override {
        std::cout << "Game starting..." << std::endl;
        m_GameTimer.Reset();
        
        auto& app = Walrus::Application::Get();
        
        // Game update every 500ms
        m_UpdateId = app.SetInterval([this]() {
            GameTick();
        }, 500);
        
        // End game after 10 seconds
        app.SetTimeout([this]() {
            EndGame();
        }, 10000);
    }
    
private:
    void GameTick() {
        m_Score += 10;
        std::cout << "Score: " << m_Score << " (Time: " 
                  << m_GameTimer.Elapsed() << "s)" << std::endl;
    }
    
    void EndGame() {
        std::cout << "Game Over! Final Score: " << m_Score << std::endl;
        Walrus::Application::Get().ClearInterval(m_UpdateId);
        Walrus::Application::Get().Close();
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv) {
    Walrus::ApplicationSpecification spec;
    spec.Name = "Walrus Game Demo";
    
    Walrus::Application* app = new Walrus::Application(spec);
    app->PushLayer<LoggingLayer>();
    app->PushLayer<GameLogicLayer>();
    return app;
}
```

### Async Data Processing Example

```cpp
class DataLayer : public Walrus::Layer {
private:
    std::vector<std::string> m_Data;
    std::atomic<int> m_ProcessedCount{0};
    
public:
    virtual void OnAttach() override {
        // Initialize data
        for (int i = 0; i < 100; ++i) {
            m_Data.push_back("Data item " + std::to_string(i));
        }
        
        StartProcessing();
    }
    
private:
    void StartProcessing() {
        auto& app = Walrus::Application::Get();
        
        std::cout << "Starting parallel processing of " << m_Data.size() << " items..." << std::endl;
        
        // Process items in parallel batches
        for (size_t i = 0; i < m_Data.size(); i += 10) {
            app.SetImmediate([this, i]() {
                ProcessBatch(i, std::min(i + 10, m_Data.size()));
            });
        }
        
        // Monitor progress
        MonitorProgress();
    }
    
    void ProcessBatch(size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            // Simulate processing
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            m_ProcessedCount++;
        }
    }
    
    void MonitorProgress() {
        auto& app = Walrus::Application::Get();
        
        app.SetInterval([this]() {
            std::cout << "Progress: " << m_ProcessedCount << "/" << m_Data.size() << std::endl;
            
            if (m_ProcessedCount >= static_cast<int>(m_Data.size())) {
                std::cout << "All items processed!" << std::endl;
                Walrus::Application::Get().Close();
                return;
            }
        }, 200);
    }
};
```

## Building

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- Make or Ninja build system

### Build Steps

1. **Clone and navigate:**
   ```bash
   git clone <repository>
   cd Walnut
   ```

2. **Configure with CMake:**
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. **Build:**
   ```bash
   make
   # or
   cmake --build .
   ```

4. **Run:**
   ```bash
   ./bin/WalrusApp
   ```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build  
cmake -DCMAKE_BUILD_TYPE=Release ..

# Custom compiler
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

## Project Structure

```
Walnut/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # This file
├── Walrus/                     # Core framework library
│   ├── CMakeLists.txt          # Library build config
│   └── src/Walrus/
│       ├── Application.h/cpp   # Main application class
│       ├── Layer.h             # Layer interface
│       ├── EntryPoint.h        # Application entry point
│       ├── EventLoop.h/cpp     # Async event system
│       ├── Timer.h             # High-resolution timer
│       └── Random.h/cpp        # Random utilities
├── WalrusApp/                  # Example application
│   ├── CMakeLists.txt          # App build config
│   └── src/WalrusApp.cpp       # Demo application
└── build/                      # Build artifacts (generated)
    ├── bin/WalrusApp           # Final executable
    └── lib/libWalrus.a         # Static library
```

### Key Components

- **Walrus Library**: Core framework as static library
- **WalrusApp**: Example/demo application
- **CMake Integration**: Modern build system with proper dependency management
- **Header-only Utilities**: Timer and some utilities are header-only for performance

## Migration from Other Frameworks

If you're coming from other frameworks:

### From Node.js/JavaScript
- `setTimeout()` → `app.SetTimeout()`
- `setInterval()` → `app.SetInterval()`  
- `setImmediate()` → `app.SetImmediate()`
- `clearTimeout()/clearInterval()` → `app.ClearTimeout()/app.ClearInterval()`

### From Game Engines
- Update loops → Layer `OnUpdate()` methods
- Scene management → Layer system
- Component systems → Multiple specialized layers

### From Qt/GUI Frameworks
- Main event loop → Built-in application loop + EventLoop for async
- Widgets/Components → Layers
- Signal/Slot → EventLoop callbacks

## Best Practices

### 🎯 **Design Principles**
- **Layer Separation**: Use layers for logical separation of concerns
- **EventLoop for Async**: Use SetTimeout/SetInterval instead of manual threading  
- **PubSub for Communication**: Decouple components with type-safe messaging
- **RAII Everything**: Leverage C++ destructors for automatic cleanup

### ⚡ **Performance Tips**
- EventLoop callbacks run in parallel - avoid shared mutable state
- PubSub messages are copied - consider move semantics for large objects
- Use references in callbacks to avoid unnecessary copies
- SetInterval frequency should match actual needs

### 🛡️ **Error Handling**
- Framework catches and logs callback exceptions safely
- Always clean up EventIds in OnDetach if needed
- Use smart pointers for automatic memory management
- Design layers to be exception-safe

### 📬 **PubSub Patterns**
- Use descriptive topic names: `"player/movement"`, `"system/error"`
- Keep message types simple and copyable  
- Consider message frequency and size for performance
- Unsubscribe in OnDetach if using manual subscriptions

## Acknowledgments

This project builds upon the excellent work of:
- **[Yan Chernikov (TheCherno)](https://github.com/TheCherno)** - Original Walnut framework design
- **[Walnut Framework](https://github.com/StudioCherno/Walnut)** - Architecture inspiration

Core layer system and application lifecycle derived from the original Walnut project, adapted for console applications with async and messaging capabilities.

## License

See LICENSE.txt for details.

---

**Happy coding with Walrus! 🦭**