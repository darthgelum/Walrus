#include "Application.h"

#include <iostream>
#include <thread>

#if WALRUS_ENABLE_PUBSUB
#include "InMemoryBroker.h"
#endif

extern bool g_ApplicationRunning;

static Walrus::Application* s_Instance = nullptr;

namespace Walrus {

	Application::Application(const ApplicationSpecification& applicationSpecification)
		: m_Specification(applicationSpecification), m_Running(false)
	{
		s_Instance = this;
		
		// Initialize FiberTaskingLib scheduler with user configuration
		ftl::TaskSchedulerInitOptions initOptions;
		initOptions.FiberPoolSize = applicationSpecification.FiberPoolSize;
		initOptions.ThreadPoolSize = applicationSpecification.ThreadPoolSize;
		initOptions.Behavior = applicationSpecification.QueueBehavior;
		m_TaskScheduler.Init(initOptions);
		
#if WALRUS_ENABLE_PUBSUB
		// Set the PubSub broker from specification
		m_PubSubBroker = applicationSpecification.PubSubBroker;
		
		// Initialize the broker if it's an InMemoryBroker
		if (auto inMemoryBroker = std::dynamic_pointer_cast<InMemoryBroker>(m_PubSubBroker)) {
			inMemoryBroker->Init(&m_TaskScheduler);
		}
#endif
	}

	Application::~Application()
	{
		Shutdown();
		
		// TaskScheduler doesn't need explicit shutdown
	}

	Application& Application::Get()
	{
		return *s_Instance;
	}

	void Application::Run()
	{
		m_Running = true;
		
#if WALRUS_ENABLE_EVENT_LOOP
		m_EventLoop.Init(&m_TaskScheduler);
		m_EventLoop.Start();
#endif
		
		Init();
#if WALRUS_ENABLE_PUBSUB
		// Start the PubSub broker if available
		if (m_PubSubBroker) {
			m_PubSubBroker->Start();
			std::cout << "PubSub broker started" << std::endl;
		}
#endif

		m_StartTime = std::chrono::steady_clock::now();

		std::cout << "Starting " << m_Specification.Name << "..." << std::endl;

		// Simple console-based application loop
		while (m_Running)
		{
			auto now = std::chrono::steady_clock::now();
			float time = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_StartTime).count();
			m_TimeStep = time - m_LastFrameTime;
			m_LastFrameTime = time;
			
			// Update all layers using the hierarchical tree with parallel execution
			if (!m_LayerTree.IsEmpty()) {
				m_LayerTree.UpdateTree(m_TimeStep, &m_TaskScheduler);
			}

			// Update rate limiting based on configuration
			if (m_Specification.EnableFrameRateLimit && m_Specification.TargetFPS > 0.0f) {
				auto targetUpdateTime = std::chrono::duration<float>(1.0f / m_Specification.TargetFPS);
				auto targetUpdateTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(targetUpdateTime);
				std::this_thread::sleep_for(targetUpdateTimeMs);
			} else {
				// Even without frame rate limiting, yield to prevent 100% CPU spinning
				std::this_thread::yield();
			}
		}

		// Detach all layers in the tree
		m_LayerTree.OnDetachAll();
		
#if WALRUS_ENABLE_EVENT_LOOP
		m_EventLoop.Stop();
#endif

#if WALRUS_ENABLE_PUBSUB
		// Stop the PubSub broker if available
		if (m_PubSubBroker) {
			m_PubSubBroker->Stop();
			std::cout << "PubSub broker stopped" << std::endl;
		}
#endif

		Shutdown();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	float Application::GetTime()
	{
		auto now = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<float>>(now - m_StartTime).count();
	}

	void Application::Init()
	{
		// Initialization for console application
		std::cout << "Initializing " << m_Specification.Name << std::endl;
		
		// Display TaskScheduler configuration
		std::cout << "TaskScheduler Configuration:" << std::endl;
		std::cout << "  Fiber Pool Size: " << m_Specification.FiberPoolSize << std::endl;
		std::cout << "  Thread Pool Size: " << (m_Specification.ThreadPoolSize == 0 ? 
			"Hardware Threads (" + std::to_string(std::thread::hardware_concurrency()) + ")" : 
			std::to_string(m_Specification.ThreadPoolSize)) << std::endl;
		std::cout << "  Queue Behavior: " << 
			(m_Specification.QueueBehavior == ftl::EmptyQueueBehavior::Spin ? "Spin" :
			 m_Specification.QueueBehavior == ftl::EmptyQueueBehavior::Yield ? "Yield" : "Sleep") << std::endl;
		std::cout << "  Target Update Rate: " << m_Specification.TargetFPS << " Hz" << 
			(m_Specification.EnableFrameRateLimit ? " (limited)" : " (unlimited)") << std::endl;
		
		// Display Layer Tree structure
		std::cout << std::endl;
		m_LayerTree.PrintTree();
		std::cout << std::endl;
		
		// Now attach all layers in the tree - EventLoop is initialized so they can use SetInterval/SetTimeout
		m_LayerTree.OnAttachAll();
	}

	void Application::Shutdown()
	{
		std::cout << "Shutting down " << m_Specification.Name << std::endl;
	}

}