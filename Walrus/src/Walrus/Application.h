#ifndef WALRUS_APPLICATION_H
#define WALRUS_APPLICATION_H

#include "Config.h"
#include "Layer.h"
#include "EventLoop.h"

#if WALRUS_ENABLE_PUBSUB
#include "PubSub.h"
#endif

#include <ftl/task_scheduler.h>
#include <ftl/wait_group.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace Walrus {

	struct ApplicationSpecification
	{
		std::string Name = "Walrus App";
		
		// Update frequency configuration
		float TargetFPS = 60.0f;                    // Target updates per second
		bool EnableFrameRateLimit = true;           // Whether to limit update rate at all
		
		// TaskScheduler configuration
		unsigned FiberPoolSize = 400;               // Size of the fiber pool (default: 400)
		unsigned ThreadPoolSize = 0;               // Size of thread pool (0 = hardware threads)
		ftl::EmptyQueueBehavior QueueBehavior = ftl::EmptyQueueBehavior::Sleep; // Thread behavior when idle
		
#if WALRUS_ENABLE_PUBSUB
		// PubSub broker - passed from application (defaults to nullptr)
		std::shared_ptr<IBroker> PubSubBroker = nullptr;
#endif
		
		// Preset configurations for common use cases
		static ApplicationSpecification HighPerformance() {
			ApplicationSpecification spec;
			spec.Name = "High Performance App";
			spec.TargetFPS = 144.0f;
			spec.EnableFrameRateLimit = true;
			spec.FiberPoolSize = 1000;                              // Large fiber pool for complex tasks
			spec.ThreadPoolSize = 0;                                // Use all hardware threads
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Yield;    // Yield for balanced performance/CPU usage
			return spec;
		}
		
		static ApplicationSpecification PowerEfficient() {
			ApplicationSpecification spec;
			spec.Name = "Power Efficient App";
			spec.TargetFPS = 30.0f;
			spec.EnableFrameRateLimit = true;
			spec.FiberPoolSize = 50;                                // Smaller fiber pool
			spec.ThreadPoolSize = 2;                                // Minimum 2 threads for fiber coordination
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Sleep;    // Sleep to save power
			return spec;
		}
		
		static ApplicationSpecification BackgroundService() {
			ApplicationSpecification spec;
			spec.Name = "Background Service";
			spec.TargetFPS = 60.0f;                                  // Very low update rate
			spec.EnableFrameRateLimit = true;
			spec.FiberPoolSize = 100;                                // Minimal fiber pool
			spec.ThreadPoolSize = 8;                                // At least 2 threads for fiber coordination
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Sleep;    // Sleep most of the time
			return spec;
		}
		
		static ApplicationSpecification MaxThroughput() {
			ApplicationSpecification spec;
			spec.Name = "Max Throughput App";
			spec.EnableFrameRateLimit = false;                      // No update rate limit
			spec.FiberPoolSize = 2000;                              // Very large fiber pool
			spec.ThreadPoolSize = 0;                                // All hardware threads
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Yield;    // Yield for balanced performance
			return spec;
		}
		
		static ApplicationSpecification UltraLowPower() {
			ApplicationSpecification spec;
			spec.Name = "Ultra Low Power App";
			spec.TargetFPS = 1.0f;                                  // Only 1 update per second
			spec.EnableFrameRateLimit = true;
			spec.FiberPoolSize = 10;                                // Absolute minimum fiber pool
			spec.ThreadPoolSize = 2;                                // Minimum 2 threads for fiber coordination
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Sleep;    // Always sleep when idle
			return spec;
		}
		
		static ApplicationSpecification UltraHighPerformance() {
			ApplicationSpecification spec;
			spec.Name = "Ultra High Performance App";
			spec.TargetFPS = 240.0f;
			spec.EnableFrameRateLimit = true;
			spec.FiberPoolSize = 2000;                              // Very large fiber pool
			spec.ThreadPoolSize = 0;                                // Use all hardware threads
			spec.QueueBehavior = ftl::EmptyQueueBehavior::Spin;     // Spin for absolute lowest latency (100% CPU)
			return spec;
		}
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
		~Application();

		static Application& Get();

		void Run();
		
		template<typename T>
		void PushLayer()
		{
			static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
			m_LayerStack.emplace_back(std::make_shared<T>());
		}

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); }

		void Close();

		float GetTime();
		
		// Update frequency configuration
		void SetTargetFPS(float fps) { m_Specification.TargetFPS = fps; }
		float GetTargetFPS() const { return m_Specification.TargetFPS; }
		void SetFrameRateLimit(bool enabled) { m_Specification.EnableFrameRateLimit = enabled; }
		bool IsFrameRateLimited() const { return m_Specification.EnableFrameRateLimit; }
		
		// TaskScheduler configuration access
		unsigned GetFiberPoolSize() const { return m_Specification.FiberPoolSize; }
		unsigned GetThreadPoolSize() const { return m_Specification.ThreadPoolSize; }
		ftl::EmptyQueueBehavior GetQueueBehavior() const { return m_Specification.QueueBehavior; }
		ftl::TaskScheduler& GetTaskScheduler() { return m_TaskScheduler; }

#if WALRUS_ENABLE_EVENT_LOOP
		// Event Loop Access 
		EventLoop& GetEventLoop() { return m_EventLoop; }
		
		// Global event loop methods (convenience)
		EventId SetTimeout(EventCallback callback, int milliseconds) { return m_EventLoop.SetTimeout(std::move(callback), milliseconds); }
		EventId SetInterval(EventCallback callback, int milliseconds) { return m_EventLoop.SetInterval(std::move(callback), milliseconds); }
		EventId SetImmediate(EventCallback callback) { return m_EventLoop.SetImmediate(std::move(callback)); }
		void ClearInterval(EventId id) { m_EventLoop.ClearInterval(id); }
		void ClearTimeout(EventId id) { m_EventLoop.ClearTimeout(id); }
#endif

#if WALRUS_ENABLE_PUBSUB
		// PubSub Access (only available when enabled)
		IBroker* GetPubSubBroker() { return m_PubSubBroker.get(); }
		
		// Global PubSub methods (convenience) - only if broker is available
		template<typename T>
		void Subscribe(const std::string& topic, MessageHandler<T> handler) {
			if (m_PubSubBroker) {
				m_PubSubBroker->Subscribe<T>(topic, std::move(handler));
			}
		}
		
		template<typename T>
		void Publish(const std::string& topic, const T& data) {
			if (m_PubSubBroker) {
				m_PubSubBroker->Publish<T>(topic, data);
			}
		}
		
		template<typename T>
		void Publish(const std::string& topic, T&& data) {
			if (m_PubSubBroker) {
				m_PubSubBroker->Publish<T>(topic, std::forward<T>(data));
			}
		}
		
		void UnsubscribeFromTopic(const std::string& topic) {
			if (m_PubSubBroker) {
				m_PubSubBroker->Unsubscribe(topic);
			}
		}
		
		bool IsPubSubAvailable() const { return m_PubSubBroker != nullptr; }
#endif

	private:
		void Init();
		void Shutdown();
	private:
		ApplicationSpecification m_Specification;
		bool m_Running = false;

		float m_TimeStep = 0.0f;
		float m_FrameTime = 0.0f;
		float m_LastFrameTime = 0.0f;
		std::chrono::steady_clock::time_point m_StartTime;

		std::vector<std::shared_ptr<Layer>> m_LayerStack;
		
		// FiberTaskingLib scheduler - shared across all systems
		ftl::TaskScheduler m_TaskScheduler;
		
		EventLoop m_EventLoop;  // Always present - behavior depends on compile-time flag

#if WALRUS_ENABLE_PUBSUB
		std::shared_ptr<IBroker> m_PubSubBroker;  // PubSub broker - set from ApplicationSpecification
#endif
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}

#endif // WALRUS_APPLICATION_H