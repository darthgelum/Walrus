#include "Walrus/Application.h"
#include "Walrus/EntryPoint.h"
#include "Walrus/Timer.h"
#include "Walrus/EventLoop.h"

#include <iostream>
#include <atomic>
#include <thread>

class EventLoopDemoLayer : public Walrus::Layer
{
private:
	Walrus::Timer m_Timer;
	std::atomic<int> m_Counter{0};
	Walrus::EventId m_IntervalId = 0;
	Walrus::EventId m_TimeoutId = 0;

public:
	virtual void OnAttach() override
	{
		std::cout << "\n=== EventLoop Demo Layer Attached ===" << std::endl;
		std::cout << "Demonstrating SetTimeout, SetInterval, SetImmediate, and parallel execution..." << std::endl;
		
		auto& app = Walrus::Application::Get();
		
		// 1. SetImmediate - execute as soon as possible
		std::cout << "\n1. Setting up immediate callbacks..." << std::endl;
		
		app.SetImmediate([]() {
			std::cout << "   [IMMEDIATE 1] This executes immediately! Thread ID: " << std::this_thread::get_id() << std::endl;
		});
		
		app.SetImmediate([]() {
			std::cout << "   [IMMEDIATE 2] This also executes immediately! Thread ID: " << std::this_thread::get_id() << std::endl;
		});
		
		app.SetImmediate([]() {
			std::cout << "   [IMMEDIATE 3] All immediate callbacks run in parallel threads!" << std::endl;
		});
		
		// 2. SetTimeout - execute once after delay
		std::cout << "\n2. Setting up timeout callbacks..." << std::endl;
		
		m_TimeoutId = app.SetTimeout([]() {
			std::cout << "   [TIMEOUT 1000ms] This executes after 1 second! Thread ID: " << std::this_thread::get_id() << std::endl;
		}, 1000);
		
		app.SetTimeout([]() {
			std::cout << "   [TIMEOUT 2000ms] This executes after 2 seconds!" << std::endl;
		}, 2000);
		
		app.SetTimeout([&app]() {
			std::cout << "   [TIMEOUT 1500ms] This executes after 1.5 seconds and demonstrates callback capture!" << std::endl;
			
			// Nested timeout from within a timeout
			app.SetTimeout([]() {
				std::cout << "      [NESTED TIMEOUT 500ms] Nested timeout executed!" << std::endl;
			}, 500);
		}, 1500);
		
		// 3. SetInterval - execute repeatedly
		std::cout << "\n3. Setting up interval callback..." << std::endl;
		
		m_IntervalId = app.SetInterval([this]() {
			int count = ++m_Counter;
			std::cout << "   [INTERVAL 800ms] Repeated execution #" << count << " - Thread ID: " << std::this_thread::get_id() << std::endl;
			
			// Stop interval after 5 executions
			if (count >= 5) {
				std::cout << "   [INTERVAL] Clearing interval after 5 executions..." << std::endl;
				Walrus::Application::Get().ClearInterval(m_IntervalId);
			}
		}, 800);
		
		// 4. Demonstrate cancellation
		std::cout << "\n4. Setting up cancellation demo..." << std::endl;
		
		auto cancelId = app.SetTimeout([]() {
			std::cout << "   [CANCELLED TIMEOUT] This should NEVER execute!" << std::endl;
		}, 3000);
		
		// Cancel it immediately
		app.ClearTimeout(cancelId);
		std::cout << "   Cancelled timeout with ID: " << cancelId << std::endl;
		
		// 5. Schedule application close
		app.SetTimeout([&app]() {
			std::cout << "\n=== Demo Complete - Closing Application ===" << std::endl;
			app.Close();
		}, 6000); // Close after 6 seconds
		
		m_Timer.Reset();
		std::cout << "\nDemo started! Watch the parallel execution of callbacks..." << std::endl;
	}

	virtual void OnUpdate(float ts) override
	{
		// Layer update still works normally alongside the event loop
		// This demonstrates that both systems can coexist
		static float lastPrint = 0.0f;
		if (m_Timer.Elapsed() - lastPrint >= 2.5f) {
			std::cout << "[LAYER UPDATE] Normal layer update at " << m_Timer.Elapsed() << "s" << std::endl;
			lastPrint = m_Timer.Elapsed();
		}
	}

	virtual void OnDetach() override
	{
		std::cout << "\nEventLoop Demo Layer detached! All timers are automatically cleaned up." << std::endl;
	}
};

Walrus::Application* Walrus::CreateApplication(int argc, char** argv)
{
	Walrus::ApplicationSpecification spec;
	spec.Name = "Walrus EventLoop Demo";

	Walrus::Application* app = new Walrus::Application(spec);
	app->PushLayer<EventLoopDemoLayer>();
	
	return app;
}