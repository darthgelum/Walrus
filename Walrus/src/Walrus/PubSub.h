#ifndef WALRUS_PUBSUB_H
#define WALRUS_PUBSUB_H

#include <functional>
#include <string>
#include <memory>
#include <typeinfo>
#include <any>
#include <vector>
#include <algorithm>

namespace Walrus {

    // Forward declarations
    template<typename T> class Message;
    template<typename T> class Publisher;
    template<typename T> class Subscriber;

    // Type-erased message base for internal broker storage
    class BaseMessage {
    public:
        virtual ~BaseMessage() = default;
        virtual const std::type_info& GetType() const = 0;
        virtual std::any GetRawData() const = 0;
        virtual const std::string& GetTopic() const = 0;
    };

    // Templated message wrapper for type safety
    template<typename T>
    class Message : public BaseMessage {
    private:
        T m_Data;
        std::string m_Topic;

    public:
        Message(const T& data, const std::string& topic = "") 
            : m_Data(data), m_Topic(topic) {}

        Message(T&& data, const std::string& topic = "") 
            : m_Data(std::move(data)), m_Topic(topic) {}

        const T& GetData() const { return m_Data; }
        T& GetData() { return m_Data; }
        
        const std::string& GetTopic() const override { return m_Topic; }
        void SetTopic(const std::string& topic) { m_Topic = topic; }

        // BaseMessage interface
        const std::type_info& GetType() const override { return typeid(T); }
        std::any GetRawData() const override { return std::any(m_Data); }
    };

    // Callback type for message handlers
    template<typename T>
    using MessageHandler = std::function<void(const Message<T>&)>;

    // Generic callback for type-erased messages
    using GenericMessageHandler = std::function<void(const std::shared_ptr<BaseMessage>&)>;

    // Abstract broker interface for extensibility
    class IBroker {
    public:
        virtual ~IBroker() = default;

        // Subscribe to messages of a specific type on a topic
        template<typename T>
        void Subscribe(const std::string& topic, MessageHandler<T> handler) {
            auto genericHandler = [handler](const std::shared_ptr<BaseMessage>& baseMsg) {
                if (baseMsg->GetType() == typeid(T)) {
                    try {
                        auto data = std::any_cast<T>(baseMsg->GetRawData());
                        Message<T> typedMsg(data, baseMsg->GetTopic());
                        handler(typedMsg);
                    } catch (const std::bad_any_cast& e) {
                        // Type mismatch - ignore or log
                    }
                }
            };
            SubscribeInternal(topic, typeid(T), genericHandler);
        }

        // Publish a message to a topic
        template<typename T>
        void Publish(const std::string& topic, const T& data) {
            auto message = std::make_shared<Message<T>>(data, topic);
            PublishInternal(topic, std::static_pointer_cast<BaseMessage>(message));
        }

        template<typename T>
        void Publish(const std::string& topic, T&& data) {
            auto message = std::make_shared<Message<T>>(std::forward<T>(data), topic);
            PublishInternal(topic, std::static_pointer_cast<BaseMessage>(message));
        }

        // Unsubscribe from a topic (optional - implementation dependent)
        virtual void Unsubscribe(const std::string& topic, const std::type_info& typeInfo = typeid(void)) = 0;

        // Start/stop message processing
        virtual void Start() = 0;
        virtual void Stop() = 0;
        virtual bool IsRunning() const = 0;

    protected:
        // Internal methods for concrete implementations
        virtual void SubscribeInternal(const std::string& topic, const std::type_info& typeInfo, GenericMessageHandler handler) = 0;
        virtual void PublishInternal(const std::string& topic, std::shared_ptr<BaseMessage> message) = 0;
    };

    // Publisher helper class
    template<typename T>
    class Publisher {
    private:
        IBroker* m_Broker;
        std::string m_DefaultTopic;

    public:
        Publisher(IBroker& broker, const std::string& defaultTopic = "") 
            : m_Broker(&broker), m_DefaultTopic(defaultTopic) {}

        void Publish(const T& data, const std::string& topic = "") const {
            std::string actualTopic = topic.empty() ? m_DefaultTopic : topic;
            m_Broker->Publish<T>(actualTopic, data);
        }

        void Publish(T&& data, const std::string& topic = "") const {
            std::string actualTopic = topic.empty() ? m_DefaultTopic : topic;
            m_Broker->Publish<T>(actualTopic, std::forward<T>(data));
        }

        void SetDefaultTopic(const std::string& topic) { m_DefaultTopic = topic; }
        const std::string& GetDefaultTopic() const { return m_DefaultTopic; }
    };

    // Subscriber helper class
    template<typename T>
    class Subscriber {
    private:
        IBroker* m_Broker;
        std::vector<std::string> m_SubscribedTopics;

    public:
        Subscriber(IBroker& broker) : m_Broker(&broker) {}

        ~Subscriber() {
            // Cleanup subscriptions (if broker supports it)
            for (const auto& topic : m_SubscribedTopics) {
                m_Broker->Unsubscribe(topic, typeid(T));
            }
        }

        void Subscribe(const std::string& topic, MessageHandler<T> handler) {
            m_Broker->Subscribe<T>(topic, handler);
            m_SubscribedTopics.push_back(topic);
        }

        void Unsubscribe(const std::string& topic) {
            m_Broker->Unsubscribe(topic, typeid(T));
            auto it = std::find(m_SubscribedTopics.begin(), m_SubscribedTopics.end(), topic);
            if (it != m_SubscribedTopics.end()) {
                m_SubscribedTopics.erase(it);
            }
        }

        const std::vector<std::string>& GetSubscribedTopics() const { return m_SubscribedTopics; }
    };

}

#endif // WALRUS_PUBSUB_H