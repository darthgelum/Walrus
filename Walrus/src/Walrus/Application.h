#ifndef WALRUS_APPLICATION_H
#define WALRUS_APPLICATION_H

// TODO: include if
#include "EventLoop.h"
#include "Layer.h"
#include "LayerTree.h"
#include <cstddef>
#if WALRUS_ENABLE_PUBSUB
#include "PubSub.h"
#endif

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Walrus {

struct ApplicationSpecification {
  std::string Name = "Walrus App";

#if WALRUS_ENABLE_PUBSUB
  // PubSub broker - passed from application (defaults to nullptr)
  std::shared_ptr<IBroker> PubSubBroker = nullptr;
#endif
};

class Application {
public:
  Application(const ApplicationSpecification &applicationSpecification =
                  ApplicationSpecification());
  ~Application();

  static Application &Get();

  void Run();

  template <typename T> void PushLayer(std::string name = "") {
    static_assert(std::is_base_of<Layer, T>::value,
                  "Pushed type is not subclass of Layer!");

    auto layer = std::make_shared<T>();

    LayerTree->AddChild(layer, name);

    layer->OnAttach();
  }

  template <typename T>
  void PushLayerAfter(std::string afterName, std::string name = "") {
    static_assert(std::is_base_of<Layer, T>::value,
                  "Pushed type is not subclass of Layer!");

    auto layer = std::make_shared<T>();
    auto node = Walrus::LayerNode::FindByName(afterName);

    node->AddChild(layer, name);

    layer->OnAttach();
  }

  void PushLayer(const std::shared_ptr<Layer> &layer, std::string name = "") {
    LayerTree->AddChild(layer, name);
    layer->OnAttach();
  }

  void PushLayerAfter(const std::shared_ptr<Layer> &layer,
                      std::string afterName, std::string name = "") {
    auto node = Walrus::LayerNode::FindByName(afterName);
    node->AddChild(layer, name);
    layer->OnAttach();
  }
  void Close();

  float GetTime();

#if WALRUS_ENABLE_EVENT_LOOP
  // Event Loop Access
  EventLoop &GetEventLoop() { return m_EventLoop; }

  // Global event loop methods (convenience)
  EventId SetTimeout(EventCallback callback, int milliseconds) {
    return m_EventLoop.SetTimeout(std::move(callback), milliseconds);
  }
  EventId SetInterval(EventCallback callback, int milliseconds) {
    return m_EventLoop.SetInterval(std::move(callback), milliseconds);
  }
  EventId SetImmediate(EventCallback callback) {
    return m_EventLoop.SetImmediate(std::move(callback));
  }
  void ClearInterval(EventId id) { m_EventLoop.ClearInterval(id); }
  void ClearTimeout(EventId id) { m_EventLoop.ClearTimeout(id); }
#endif

#if WALRUS_ENABLE_PUBSUB
  // PubSub Access (only available when enabled)
  IBroker *GetPubSubBroker() { return m_PubSubBroker.get(); }

  // Global PubSub methods (convenience) - only if broker is available
  template <typename T>
  void Subscribe(const std::string &topic, MessageHandler<T> handler) {
    if (m_PubSubBroker) {
      m_PubSubBroker->Subscribe<T>(topic, std::move(handler));
    }
  }

  template <typename T> void Publish(const std::string &topic, const T &data) {
    if (m_PubSubBroker) {
      m_PubSubBroker->Publish<T>(topic, data);
    }
  }

  template <typename T> void Publish(const std::string &topic, T &&data) {
    if (m_PubSubBroker) {
      m_PubSubBroker->Publish<T>(topic, std::forward<T>(data));
    }
  }

  void UnsubscribeFromTopic(const std::string &topic) {
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

  // std::vector<std::shared_ptr<Layer>> m_LayerStack;
  std::shared_ptr<LayerNode> LayerTree =
      std::make_shared<LayerNode>(nullptr, nullptr, "walrus_root_layer_node_t");
  EventLoop
      m_EventLoop; // Always present - behavior depends on compile-time flag

#if WALRUS_ENABLE_PUBSUB
  std::shared_ptr<IBroker>
      m_PubSubBroker; // PubSub broker - set from ApplicationSpecification
#endif
};

// Implemented by CLIENT
Application *CreateApplication(int argc, char **argv);
} // namespace Walrus

#endif // WALRUS_APPLICATION_H
