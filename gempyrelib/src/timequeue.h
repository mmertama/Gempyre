#ifndef TIMEQUEUE_H
#define TIMEQUEUE_H

#include "timer.h"
#include <cassert>

//Exposed for unittests, otherwise only used for timer.cpp

namespace Gempyre {

class TimeQueue {
public:
    using TimeType = TimerMgr::TimeType;
    using Function = TimerMgr::Function;
    struct Data {
        Data(const Data& other) = default;
        Data(Data&& other) = default;

        Data(const TimeType& t, const Function& f, const TimeType& i, int d) :
            currentTime(t), func(f), initialTime(i), id(d) {}
        TimeType currentTime = std::chrono::milliseconds(0); //when this is 0 the timer elapses
        Function func = nullptr;                             //function to run
        TimeType initialTime = std::chrono::milliseconds(0); //requested time, for recurring timer
        int id = 0;                                          //id of this timer
        bool pending = false;
        char _PADDING[4] =  "\0\0\0";                        //compiler warnings
    };
    using DataPtr = std::shared_ptr<Data>;
private:
    struct Comp {
    bool operator()(const DataPtr& a, const DataPtr& b) const {
            return a->currentTime < b->currentTime;
        };
    };
public:
    TimeQueue() {
    }

    int append(const TimeType& ms, const Function& func) {
        std::lock_guard<std::mutex> guard(m_mutex);
        ++m_lastId; //just a running number
        m_priorityQueue.insert(std::make_shared<Data>(ms, func, ms, m_lastId));  //priorize                                         //set this timer to timer set
        return m_lastId;
    }

    /// restore timer if not removed already
    bool restoreIf(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id == id;});
        if(it == m_priorityQueue.end())
            return false;
        auto data = m_priorityQueue.extract(it);
        data.value()->currentTime = data.value()->initialTime;
        data.value()->pending = false;
        m_priorityQueue.insert(std::move(data));  //priorize
        return true;
    }

    ///move all timers
    void reduce(const TimeType& sleep) {
        std::lock_guard<std::mutex> guard(m_mutex);
        for(auto& d : m_priorityQueue) {
            d->currentTime -= sleep;
        }
    }

    ///Remove a timer
    void remove(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id == id;});
        assert(it != m_priorityQueue.end());
        m_priorityQueue.erase(it);
    }


    bool setPending(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id == id;});
        if(it == m_priorityQueue.end())
            return false;
        (*it)->pending = true;
        return true;
    }

   bool contains(int id) const {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id == id;});
        return it != m_priorityQueue.end();
    }

    /// Change everything executed now
    void setNow(bool executeQueued) {
        std::lock_guard<std::mutex> guard(m_mutex);
        for(auto& c : m_priorityQueue) {
            if(!executeQueued) {
               c->pending = true;
            }
            c->currentTime = std::chrono::milliseconds{0};
        }
    }


    //peek the next item
    [[nodiscard]]
    std::optional<Data> copyTop() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peek", m_priorityQueue.size());
        std::unique_lock<std::mutex> guard(m_mutex);
        for(const auto& d : m_priorityQueue) {
            if(d->pending)
                continue;
            const auto value = std::make_optional(*d);
            guard.unlock(); // I have no idea why RAII wont work, unlock does
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue return");
            return value;
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue is empty");
        return std::nullopt;
    }

    void clear() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer queue clear");
        std::lock_guard<std::mutex> guard(m_mutex);
        m_priorityQueue.clear();
    }

    [[nodiscard]]
    bool empty() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_priorityQueue.empty();
    }

    [[nodiscard]]
    size_t size() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_priorityQueue.size();
    }

private:
    static Comp m_comp;
    mutable std::mutex m_mutex;
    int m_lastId = 0;
    std::multiset<DataPtr, Comp> m_priorityQueue;
};
}

#endif // TIMEQUEUE_H
