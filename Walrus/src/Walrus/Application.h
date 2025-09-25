#pragma once

#include "Layer.h"
#include "EventLoop.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace Walrus {

	struct ApplicationSpecification
	{
		std::string Name = "Walrus App";
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
			m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
		}

		void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

		void Close();

		float GetTime();

		// Event Loop Access
		EventLoop& GetEventLoop() { return m_EventLoop; }
		
		// Global event loop methods (convenience)
		EventId SetTimeout(EventCallback callback, int milliseconds) { return m_EventLoop.SetTimeout(std::move(callback), milliseconds); }
		EventId SetInterval(EventCallback callback, int milliseconds) { return m_EventLoop.SetInterval(std::move(callback), milliseconds); }
		EventId SetImmediate(EventCallback callback) { return m_EventLoop.SetImmediate(std::move(callback)); }
		void ClearInterval(EventId id) { m_EventLoop.ClearInterval(id); }
		void ClearTimeout(EventId id) { m_EventLoop.ClearTimeout(id); }

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
		EventLoop m_EventLoop;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}