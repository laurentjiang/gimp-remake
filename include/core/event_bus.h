/**
 * @file event_bus.h
 * @brief Type-safe event bus for decoupled publish/subscribe notifications.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace gimp {

/**
 * @brief Type-safe event bus for decoupled component communication.
 *
 * Supports typed events where subscribers register handlers for specific
 * event types and publishers emit events that are delivered to all matching
 * subscribers.
 */
class EventBus {
  public:
    /*! @brief Subscription handle type for unsubscribing. */
    using SubscriptionId = std::size_t;

    /*! @brief Returns the singleton EventBus instance.
     *  @return Reference to the global EventBus.
     */
    static EventBus& instance()
    {
        static EventBus bus;
        return bus;
    }

    /**
     * @brief Subscribe to events of type T.
     * @tparam T The event type to subscribe to.
     * @param handler Callback invoked when an event of type T is published.
     * @return Subscription ID that can be used to unsubscribe.
     */
    template <typename T>
    SubscriptionId subscribe(std::function<void(const T&)> handler)
    {
        const std::scoped_lock lock(mutex_);
        auto id = nextId_++;
        auto wrapper = [handler = std::move(handler)](const std::any& event) {
            handler(std::any_cast<const T&>(event));
        };
        subscribers_[std::type_index(typeid(T))].push_back({id, std::move(wrapper)});
        return id;
    }

    /**
     * @brief Unsubscribe from events using subscription ID.
     * @param id The subscription ID returned from subscribe().
     */
    void unsubscribe(SubscriptionId id)
    {
        const std::scoped_lock lock(mutex_);
        for (auto& [type, subs] : subscribers_) {
            std::erase_if(subs, [id](const Subscriber& s) { return s.id == id; });
        }
    }

    /**
     * @brief Publish an event to all subscribers of type T.
     * @tparam T The event type.
     * @param event The event instance to publish.
     */
    template <typename T>
    void publish(const T& event)
    {
        std::vector<std::function<void(const std::any&)>> handlers;
        {
            const std::scoped_lock lock(mutex_);
            auto it = subscribers_.find(std::type_index(typeid(T)));
            if (it != subscribers_.end()) {
                for (const auto& sub : it->second) {
                    handlers.push_back(sub.handler);
                }
            }
        }
        for (const auto& handler : handlers) {
            handler(event);
        }
    }

    /**
     * @brief Remove all subscribers (useful for testing).
     */
    void clear()
    {
        const std::scoped_lock lock(mutex_);
        subscribers_.clear();
    }

  private:
    EventBus() = default;

    struct Subscriber {
        SubscriptionId id;
        std::function<void(const std::any&)> handler;
    };

    std::mutex mutex_;
    SubscriptionId nextId_ = 1;
    std::unordered_map<std::type_index, std::vector<Subscriber>> subscribers_;
};

}  // namespace gimp
