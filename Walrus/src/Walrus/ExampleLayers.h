#ifndef WALRUS_EXAMPLE_LAYERS_H
#define WALRUS_EXAMPLE_LAYERS_H

#include "Walrus/Layer.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>

namespace Walrus {

	/**
	 * @brief Simple example layer for testing the tree structure
	 */
	class ExampleLayer : public Layer
	{
	public:
		ExampleLayer(const std::string& name, int updateTimeMs = 0)
			: m_Name(name), m_UpdateTimeMs(updateTimeMs), m_UpdateCount(0)
		{
		}

		virtual void OnAttach() override
		{
			std::cout << "[" << m_Name << "] Layer attached" << std::endl;
		}

		virtual void OnDetach() override
		{
			std::cout << "[" << m_Name << "] Layer detached (updated " << m_UpdateCount << " times)" << std::endl;
		}

		virtual void OnUpdate(float ts) override
		{
			m_UpdateCount++;
			
			// Simulate some work if specified
			if (m_UpdateTimeMs > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(m_UpdateTimeMs));
			}
			
			// Print occasionally to show it's working
			if (m_UpdateCount % 60 == 0) { // Every ~1 second at 60 FPS
				std::cout << "[" << m_Name << "] Update #" << m_UpdateCount 
					<< " (ts: " << ts << "s)" << std::endl;
			}
		}

		const std::string& GetName() const { return m_Name; }
		int GetUpdateCount() const { return m_UpdateCount; }

	private:
		std::string m_Name;
		int m_UpdateTimeMs;
		int m_UpdateCount;
	};

	/**
	 * @brief Heavy computation layer for testing parallel execution
	 */
	class HeavyComputeLayer : public Layer
	{
	public:
		HeavyComputeLayer(const std::string& name, int computeIterations = 1000000)
			: m_Name(name), m_ComputeIterations(computeIterations), m_UpdateCount(0)
		{
		}

		virtual void OnAttach() override
		{
			std::cout << "[" << m_Name << "] Heavy compute layer attached" << std::endl;
		}

		virtual void OnDetach() override
		{
			std::cout << "[" << m_Name << "] Heavy compute layer detached (computed " << m_UpdateCount << " times)" << std::endl;
		}

		virtual void OnUpdate(float ts) override
		{
			m_UpdateCount++;
			
			// Simulate heavy computation
			volatile double result = 0.0;
			for (int i = 0; i < m_ComputeIterations; ++i) {
				result += std::sin(i * 0.001) * std::cos(i * 0.002);
			}
			
			// Print occasionally to show it's working
			if (m_UpdateCount % 30 == 0) { // Every ~0.5 seconds at 60 FPS
				std::cout << "[" << m_Name << "] Heavy compute #" << m_UpdateCount 
					<< " (result: " << result << ")" << std::endl;
			}
		}

		const std::string& GetName() const { return m_Name; }
		int GetUpdateCount() const { return m_UpdateCount; }

	private:
		std::string m_Name;
		int m_ComputeIterations;
		int m_UpdateCount;
	};

}

#endif // WALRUS_EXAMPLE_LAYERS_H