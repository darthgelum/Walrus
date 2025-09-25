# Walrus Framework 🦭

A lightweight, console-based C++ application framework featuring a powerful layer system and JavaScript-like event loop for asynchronous programming.

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Layer System](#layer-system)
- [EventLoop System](#eventloop-system)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Building](#building)
- [Project Structure](#project-structure)

## Overview

Walrus is a modern C++ framework designed for console applications that need structured architecture and asynchronous capabilities. Born from simplifying complex UI frameworks, Walrus focuses on core application lifecycle management with powerful async features.

## Features

### 🎯 **Core Framework**
- **Layer-based Architecture**: Organize code into reusable, composable layers
- **Application Lifecycle**: Structured initialization, update, and shutdown
- **Cross-platform**: Works on Windows, Linux, and macOS
- **CMake Build System**: Modern, reliable build configuration

### ⚡ **EventLoop System**
- **SetTimeout**: Execute callbacks once after a delay
- **SetInterval**: Execute callbacks repeatedly at intervals
- **SetImmediate**: Execute callbacks as soon as possible
- **ClearInterval/ClearTimeout**: Cancel scheduled callbacks
- **Thread Pool**: Parallel execution with configurable worker threads
- **Thread Safety**: All operations are thread-safe
- **Exception Handling**: Robust error handling for callbacks

### 🛠 **Utilities**
- **High-Resolution Timer**: Precise timing measurements
- **Random Number Generation**: Thread-safe random utilities
- **Resource Management**: Automatic cleanup and RAII principles

## Quick Start

### 1. Create a Simple Application

```cpp
#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"

class MyLayer : public Walrus::Layer
{
public:
    virtual void OnAttach() override 
    {
        std::cout << "Hello, Walrus!" << std::endl;
        
        // Use the event loop
        auto& app = Walrus::Application::Get();
        app.SetTimeout([]() {
            std::cout << "This runs after 2 seconds!" << std::endl;
        }, 2000);
    }
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv)
{
    Walrus::ApplicationSpecification spec;
    spec.Name = "My Walrus App";

    Walrus::Application* app = new Walrus::Application(spec);
    app->PushLayer<MyLayer>();
    return app;
}
```

### 2. Build and Run

```bash
mkdir build && cd build
cmake ..
make
./bin/WalrusApp
```

## Layer System

The layer system provides a clean way to organize your application logic into modular, reusable components.

### Layer Interface

```cpp
class Layer 
{
public:
    virtual ~Layer() = default;
    
    virtual void OnAttach() {}    // Called when layer is added
    virtual void OnDetach() {}    // Called when layer is removed  
    virtual void OnUpdate(float deltaTime) {}  // Called every frame
};
```

### Creating Layers

```cpp
class GameLayer : public Walrus::Layer 
{
private:
    Walrus::Timer m_Timer;
    
public:
    virtual void OnAttach() override 
    {
        std::cout << "Game initialized!" << std::endl;
        m_Timer.Reset();
    }
    
    virtual void OnUpdate(float deltaTime) override 
    {
        // Game logic here
        if (m_Timer.Elapsed() > 1.0f) {
            std::cout << "Game tick!" << std::endl;
            m_Timer.Reset();
        }
    }
    
    virtual void OnDetach() override 
    {
        std::cout << "Game cleanup!" << std::endl;
    }
};

// Add to application
app->PushLayer<GameLayer>();
```

### Layer Benefits

- ✅ **Modular**: Each layer handles specific functionality
- ✅ **Reusable**: Layers can be used across different applications  
- ✅ **Lifecycle Management**: Automatic attach/detach handling
- ✅ **No Implementation Required**: Override only what you need

## EventLoop System

The EventLoop provides JavaScript-like asynchronous programming capabilities with true parallel execution.

### Core Functions

#### SetTimeout
Execute a callback once after a delay:

```cpp
auto& app = Walrus::Application::Get();

auto timeoutId = app.SetTimeout([]() {
    std::cout << "This executes after 1.5 seconds" << std::endl;
}, 1500);

// Cancel if needed
app.ClearTimeout(timeoutId);
```

#### SetInterval
Execute a callback repeatedly at intervals:

```cpp
auto intervalId = app.SetInterval([]() {
    std::cout << "This repeats every 1 second" << std::endl;
}, 1000);

// Stop the interval
app.ClearInterval(intervalId);
```

#### SetImmediate
Execute a callback as soon as possible:

```cpp
app.SetImmediate([]() {
    std::cout << "This executes immediately!" << std::endl;
});
```

### Advanced Usage

#### Chained Operations
```cpp
void StartProcess() {
    auto& app = Walrus::Application::Get();
    
    app.SetImmediate([]() {
        std::cout << "Step 1: Starting..." << std::endl;
        
        Walrus::Application::Get().SetTimeout([]() {
            std::cout << "Step 2: Processing..." << std::endl;
            
            Walrus::Application::Get().SetTimeout([]() {
                std::cout << "Step 3: Complete!" << std::endl;
            }, 1000);
        }, 2000);
    });
}
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

### 🎯 **Layer Design**
- Keep layers focused on single responsibilities
- Use OnAttach for initialization, OnDetach for cleanup
- OnUpdate should be lightweight and fast
- Prefer EventLoop for heavy/async operations

### ⚡ **EventLoop Usage**  
- Use SetImmediate for breaking up heavy work
- SetInterval for regular monitoring/updates
- SetTimeout for delayed actions and timeouts
- Always store EventIds if you need to cancel
- Callbacks should be exception-safe

### 🔧 **Performance**
- EventLoop callbacks run in parallel - avoid shared state or use atomics
- Use RAII for resource management in layers
- Prefer references over copies in callbacks
- Consider callback frequency for SetInterval

### 🛡️ **Error Handling**
- EventLoop catches and logs callback exceptions
- Layers should handle their own exceptions in lifecycle methods
- Use smart pointers for automatic memory management

## Acknowledgments

This project is inspired by and builds upon the excellent work of:

- **[Yan Chernikov (TheCherno)](https://github.com/TheCherno)** - For the original Walnut framework design and architecture inspiration
- **[Walnut Framework](https://github.com/StudioCherno/Walnut)** - The original Vulkan/ImGui-based application framework that served as the foundation for this console-focused adaptation

The core layer system architecture, application lifecycle management, and overall design philosophy are derived from TheCherno's Walnut framework. This project represents a specialized adaptation focused on console applications with added asynchronous capabilities.

Special thanks to the original Walnut project for providing an excellent foundation for building modern C++ applications!

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style
- New features include examples
- CMake configuration is updated for new files
- Documentation is updated

## License

See LICENSE.txt for details.

---

**Happy coding with Walrus! 🦭**