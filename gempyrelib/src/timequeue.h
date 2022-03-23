#ifndef TIMEQUEUE_H
#define TIMEQUEUE_H

#include "timer.h"
#include <cassert>

//Exposed for unittests, otherwise only used for timer.cpp

namespace Gempyre {

class TimerData {
public:
    using Function = TimerMgr::Function;
    using TimeType = TimerMgr::TimeType;
    TimerData(const TimerData& other) = default;
    TimerData(TimerData&& other) = default;
    TimerData(const TimeType& t, const Function& f, const TimeType& i, bool singleShot, int d) :
        m_currentTime{t},
        m_func{f},
        m_initialTime{i},
        m_singleShot{singleShot},
        m_id{d} {}
    TimeType currentTime() const {return m_currentTime;}
    void  reset() {m_currentTime = m_initialTime; m_pending = false;}
    void call() const {m_func(m_id);}
    bool singleShot() const {return m_singleShot;}
    int id() const {return m_id;}
    bool pending() const {return m_pending;}
    void setNow() {
        m_currentTime = std::chrono::milliseconds{0};
    }
    void setPending() {
        m_pending = true;
    }
    void dec(TimeType sleep) {
         m_currentTime -= sleep;
    }
 private:
    TimeType m_currentTime; //when this is 0 the timer elapses
    Function m_func;                             //function to run
    TimeType m_initialTime; //requested time, for recurring timer
    bool m_singleShot;
    int m_id;                                          //id of this timer
    bool m_pending = false;
    //char _PADDING[4] =  "\0\0\0";                        //compiler warnings
};

class TimeQueue {
public:
    using DataPtr = std::shared_ptr<TimerData>;
private:
    struct Comp {
    bool operator()(const DataPtr& a, const DataPtr& b) const {
            return a->currentTime() < b->currentTime();
        };
    };
public:

    TimeQueue() {}

    int append(const TimerData::TimeType& ms, bool is_singleShot, TimerData::Function&& func) {
        std::lock_guard<std::mutex> guard(m_mutex);
        ++m_lastId; //just a running number
        m_priorityQueue.emplace(std::make_shared<TimerData>(ms, func, ms, is_singleShot, m_lastId));  //priorize                                         //set this timer to timer set
        return m_lastId;
    }

    /// restore timer if not removed already
    bool restoreIf(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id() == id;});
        if(it == m_priorityQueue.end())
            return false;
        auto data = m_priorityQueue.extract(it);
        data.value()->reset();
        m_priorityQueue.insert(std::move(data));  //priorize
        return true;
    }

    ///move all timers
    void reduce(const TimerData::TimeType& sleep) {
        std::lock_guard<std::mutex> guard(m_mutex);
        for(auto& d : m_priorityQueue) {
            d->dec(sleep);
        }
    }

    ///Remove a timer
    bool remove(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id() == id;});
        if(it == m_priorityQueue.end()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug,
                              "timer remove - not found",
                              id, m_priorityQueue.size());
            return false;
        }
        m_priorityQueue.erase(it);
        return true;
    }


    bool setPending(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id() == id;});
        if(it == m_priorityQueue.end())
            return false;
        (*it)->setPending();
        return true;
    }

   bool contains(int id) const {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto it = std::find_if(m_priorityQueue.begin(), m_priorityQueue.end(), [&id](const auto& d){return d->id() == id;});
        return it != m_priorityQueue.end();
    }

    /// Change everything executed now
    void setNow(bool executeQueued) {
        std::lock_guard<std::mutex> guard(m_mutex);
        for(auto& c : m_priorityQueue) {
            if(!executeQueued) {
               c->setPending();
            }
               c->setNow();
        }
    }

    //peek the next item
    [[nodiscard]]
    std::optional<TimerData> copyTop() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peek", m_priorityQueue.size());
        std::unique_lock<std::mutex> guard(m_mutex);
        for(const auto& d : m_priorityQueue) {
            if(d->pending())
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
