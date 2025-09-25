#ifndef WALRUS_INMEMORYBROKER_H
#define WALRUS_INMEMORYBROKER_H

#include "PubSub.h"
#include <unordered_map>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>

#if WALRUS_ENABLE_PUBSUB
#include <ftl/task_scheduler.h>
#include <ftl/task.h>
#endif

namespace Walrus {

    // Default in-memory broker implementation
    class InMemoryBroker : public IBroker {
    private:
        // Topic -> Type -> List of handlers
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<GenericMessageHandler>>> m_Subscribers;
        
        // Fiber-based processing
        mutable std::mutex m_Mutex;
        ftl::TaskScheduler* m_TaskScheduler{nullptr};
        std::atomic<bool> m_Running{false};

        // Statistics
        std::atomic<size_t> m_MessagesProcessed{0};
        std::atomic<size_t> m_MessagesPublished{0};

    public:
        InMemoryBroker() = default;
        
        ~InMemoryBroker() {
            Stop();
        }
        
        void Init(ftl::TaskScheduler* taskScheduler) {
            m_TaskScheduler = taskScheduler;
        }

        // IBroker interface implementation
        void Start() override {
            if (m_Running.load() || !m_TaskScheduler) {
                return; // Already running or not initialized
            }

            m_Running.store(true);
            std::cout << "InMemoryBroker: Started message processing" << std::endl;
        }

        void Stop() override {
            if (!m_Running.load()) {
                return; // Already stopped
            }

            m_Running.store(false);
            std::cout << "InMemoryBroker: Stopped (Processed: " << m_MessagesProcessed.load() 
                      << ", Published: " << m_MessagesPublished.load() << ")" << std::endl;
        }

        bool IsRunning() const override {
            return m_Running.load();
        }

        void Unsubscribe(const std::string& topic, const std::type_info& typeInfo) override {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            if (typeInfo == typeid(void)) {
                // Unsubscribe from all types on this topic
                m_Subscribers[topic].clear();
            } else {
                // Unsubscribe from specific type on this topic
                std::string typeName = typeInfo.name();
                auto topicIt = m_Subscribers.find(topic);
                if (topicIt != m_Subscribers.end()) {
                    auto typeIt = topicIt->second.find(typeName);
                    if (typeIt != topicIt->second.end()) {
                        topicIt->second.erase(typeIt);
                    }
                }
            }
        }

        // Statistics and monitoring
        size_t GetMessagesProcessed() const { return m_MessagesProcessed.load(); }
        size_t GetMessagesPublished() const { return m_MessagesPublished.load(); }
        size_t GetTopicCount() const { 
            std::lock_guard<std::mutex> lock(m_Mutex);
            return m_Subscribers.size(); 
        }
        size_t GetSubscriberCount() const {
            std::lock_guard<std::mutex> lock(m_Mutex);
            size_t count = 0;
            for (const auto& topic : m_Subscribers) {
                for (const auto& type : topic.second) {
                    count += type.second.size();
                }
            }
            return count;
        }

        // Get all topics (for debugging/monitoring)
        std::vector<std::string> GetTopics() const {
            std::lock_guard<std::mutex> lock(m_Mutex);
            std::vector<std::string> topics;
            topics.reserve(m_Subscribers.size());
            for (const auto& topic : m_Subscribers) {
                topics.push_back(topic.first);
            }
            return topics;
        }

    protected:
        void SubscribeInternal(const std::string& topic, const std::type_info& typeInfo, GenericMessageHandler handler) override {
            std::lock_guard<std::mutex> lock(m_Mutex);
            std::string typeName = typeInfo.name();
            m_Subscribers[topic][typeName].push_back(std::move(handler));
        }

        void PublishInternal(const std::string& topic, std::shared_ptr<BaseMessage> message) override {
            if (!m_TaskScheduler || !m_Running.load()) {
                return;
            }
            
            m_MessagesPublished.fetch_add(1);
            
            // Process message immediately using fiber tasks
            m_TaskScheduler->AddTask(ftl::Task{ProcessMessageTask, 
                new MessageTaskData{this, topic, message}}, ftl::TaskPriority::Normal);
        }

    private:
        struct MessageTaskData {
            InMemoryBroker* broker;
            std::string topic;
            std::shared_ptr<BaseMessage> message;
        };
        
        static void ProcessMessageTask(ftl::TaskScheduler* taskScheduler, void* arg) {
            (void)taskScheduler; // Unused
            
            std::unique_ptr<MessageTaskData> data(static_cast<MessageTaskData*>(arg));
            data->broker->ProcessSingleMessage(data->topic, data->message);
        }
        
        void ProcessSingleMessage(const std::string& topic, std::shared_ptr<BaseMessage> message) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            // Find subscribers for this topic and message type
            auto topicIt = m_Subscribers.find(topic);
            if (topicIt != m_Subscribers.end()) {
                std::string typeName = message->GetType().name();
                auto typeIt = topicIt->second.find(typeName);
                if (typeIt != topicIt->second.end()) {
                    // Deliver to all subscribers of this type
                    for (const auto& handler : typeIt->second) {
                        try {
                            handler(message);
                            m_MessagesProcessed.fetch_add(1);
                        } catch (const std::exception& e) {
                            std::cerr << "InMemoryBroker: Exception in message handler: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << "InMemoryBroker: Unknown exception in message handler" << std::endl;
                        }
                    }
                }
            }
        }
        

    };

}

#endif // WALRUS_INMEMORYBROKER_H