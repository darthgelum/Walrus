#include "Application.h"

#include <iostream>
#include <thread>

extern bool g_ApplicationRunning;

static Walrus::Application *s_Instance = nullptr;

namespace Walrus {

Application::Application(
    const ApplicationSpecification &applicationSpecification)
    : m_Specification(applicationSpecification), m_Running(false) {
  s_Instance = this;

#if WALRUS_ENABLE_PUBSUB
  // Set the PubSub broker from specification
  m_PubSubBroker = applicationSpecification.PubSubBroker;
#endif
}

Application::~Application() { Shutdown(); }

Application &Application::Get() { return *s_Instance; }

void Application::Run() {
  m_Running = true;

  Init();

#if WALRUS_ENABLE_EVENT_LOOP
  m_EventLoop.Start();
#endif
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
  while (m_Running) {
    auto now = std::chrono::steady_clock::now();
    float time = std::chrono::duration_cast<std::chrono::duration<float>>(
                     now - m_StartTime)
                     .count();
    m_TimeStep = time - m_LastFrameTime;
    m_LastFrameTime = time;

    // Update all layers
    LayerTree->OnUpdate(m_TimeStep);
    // for (auto &layer : m_LayerStack) {
    //   layer->OnUpdate(m_TimeStep);
    // }

    // Sleep for a bit to avoid excessive CPU usage
    // std::this_thread::sleep_for(
    //     std::chrono::milliseconds(16)); // ~60 FPS equivalent
  }

  // Detach all layers
  LayerTree->OnDetach();
  // for (auto &layer : m_LayerStack) {
  //   layer->OnDetach();
  // }

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

void Application::Close() { m_Running = false; }

float Application::GetTime() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::duration<float>>(now -
                                                                  m_StartTime)
      .count();
}

void Application::Init() {
  // Initialization for console application
  std::cout << "Initializing " << m_Specification.Name << std::endl;
}

void Application::Shutdown() {
  std::cout << "Shutting down " << m_Specification.Name << std::endl;
}

} // namespace Walrus
