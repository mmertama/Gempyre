#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <deque>
#include <unordered_map>
#include <string>
#include <mutex>



namespace Gempyre {

template <class Event>
class EventQueue {
public:
    Event take() {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto event = std::move(m_events.back());
        m_events.pop_back();
        return event;
       }

    bool empty() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_events.empty();
    }

    void push(Event&& event) {
        m_events.emplace_front(std::forward<Event>(event));
    }

    size_t size() const {
        return m_events.size();
    }


private:
    std::deque<Event> m_events;
    mutable std::mutex m_mutex;
};

template <class Key, class Event>
class EventMap {
public:
    bool contains(const Key& key) const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_events.find(key) != m_events.end();
       }
    Event take(const Key& key) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = m_events.find(key);
        const auto event = std::move(it->second);
        m_events.erase(it);
        return event;
       }
    bool empty() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_events.empty();
    }
    void push(const Key& key, Event&& event) {
        m_events.emplace(key, std::move(event));
    }

    size_t size() const {
        return m_events.size();
    }

private:
    std::unordered_map<Key, Event> m_events;
    mutable std::mutex m_mutex;
};

}

#endif // EVENTQUEUE_H
