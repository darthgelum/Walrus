#ifndef WALRUS_INMEMORYBROKER_H
#define WALRUS_INMEMORYBROKER_H

#include "PubSub.h"
#include <unordered_map>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>

namespace Walrus {

    // Default in-memory broker implementation
    class InMemoryBroker : public IBroker {
    private:
        // Topic -> Queue of messages
        std::unordered_map<std::string, std::queue<std::shared_ptr<BaseMessage>>> m_Topics;
        
        // Topic -> Type -> List of handlers
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<GenericMessageHandler>>> m_Subscribers;
        
        // Threading
        mutable std::mutex m_Mutex;
        std::condition_variable m_Condition;
        std::thread m_ProcessorThread;
        std::atomic<bool> m_Running{false};
        std::atomic<bool> m_StopRequested{false};

        // Statistics
        std::atomic<size_t> m_MessagesProcessed{0};
        std::atomic<size_t> m_MessagesPublished{0};

    public:
        InMemoryBroker() = default;
        
        ~InMemoryBroker() {
            Stop();
        }

        // IBroker interface implementation
        void Start() override {
            if (m_Running.load()) {
                return; // Already running
            }

            m_Running.store(true);
            m_StopRequested.store(false);
            m_ProcessorThread = std::thread(&InMemoryBroker::ProcessMessages, this);
            
            std::cout << "InMemoryBroker: Started message processing" << std::endl;
        }

        void Stop() override {
            if (!m_Running.load()) {
                return; // Already stopped
            }

            m_StopRequested.store(true);
            m_Condition.notify_all();

            if (m_ProcessorThread.joinable()) {
                m_ProcessorThread.join();
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
            return m_Topics.size(); 
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
            topics.reserve(m_Topics.size());
            for (const auto& topic : m_Topics) {
                topics.push_back(topic.first);
            }
            return topics;
        }

    protected:
        void SubscribeInternal(const std::string& topic, const std::type_info& typeInfo, GenericMessageHandler handler) override {
            std::lock_guard<std::mutex> lock(m_Mutex);
            std::string typeName = typeInfo.name();
            m_Subscribers[topic][typeName].push_back(std::move(handler));
            
            // Initialize topic queue if it doesn't exist
            if (m_Topics.find(topic) == m_Topics.end()) {
                m_Topics[topic] = std::queue<std::shared_ptr<BaseMessage>>();
            }
        }

        void PublishInternal(const std::string& topic, std::shared_ptr<BaseMessage> message) override {
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Topics[topic].push(message);
                m_MessagesPublished.fetch_add(1);
            }
            m_Condition.notify_all();
        }

    private:
        void ProcessMessages() {
            std::unique_lock<std::mutex> lock(m_Mutex);
            
            while (!m_StopRequested.load()) {
                // Wait for messages or stop signal
                m_Condition.wait(lock, [this] {
                    if (m_StopRequested.load()) return true;
                    
                    for (const auto& topicPair : m_Topics) {
                        if (!topicPair.second.empty()) {
                            return true;
                        }
                    }
                    return false;
                });

                if (m_StopRequested.load()) {
                    break;
                }

                // Process all available messages
                for (auto& topicPair : m_Topics) {
                    const std::string& topic = topicPair.first;
                    std::queue<std::shared_ptr<BaseMessage>>& messageQueue = topicPair.second;
                    
                    while (!messageQueue.empty()) {
                        auto message = messageQueue.front();
                        messageQueue.pop();
                        
                        // Find subscribers for this topic and message type
                        auto topicIt = m_Subscribers.find(topic);
                        if (topicIt != m_Subscribers.end()) {
                            std::string typeName = message->GetType().name();
                            auto typeIt = topicIt->second.find(typeName);
                            if (typeIt != topicIt->second.end()) {
                                // Deliver to all subscribers of this type
                                for (const auto& handler : typeIt->second) {
                                    try {
                                        // Release lock during handler execution to avoid deadlocks
                                        lock.unlock();
                                        handler(message);
                                        lock.lock();
                                        
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
                }
            }
        }
    };

}

#endif // WALRUS_INMEMORYBROKER_H