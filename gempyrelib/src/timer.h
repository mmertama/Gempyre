#ifndef _GEMPYRETIMER_H_
#define _GEMPYRETIMER_H_

#include <chrono>
#include <queue>
#include <tuple>
#include <functional>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <future>
#include <set>

#include "gempyre_utils.h"

namespace Gempyre {

class TimeQueue {
public:
    using Function = std::function<void (int)>;
    using TimeType = std::chrono::milliseconds;
    struct DataEntry {
        TimeType currentTime = std::chrono::milliseconds(0);
        Function func = nullptr;
        TimeType initialTime = std::chrono::milliseconds(0);
        int id = 0;
        char _PADDING[4] =  "\0\0\0";
    };
    public:
    TimeQueue() {
#ifdef SINGLETON
        static int assert_count = 0;
        assert(assert_count == 0);
        ++assert_count;
#endif
    }

    int append(const TimeType& ms, const Function& func) {
        std::lock_guard<std::mutex> guard(m_mutex);
        ++m_id;
        m_queue.push(DataEntry{ms, func, ms, m_id});
        m_blessed.emplace(m_id);
        return m_id;
    }

    void reAppend(const TimeType& ms, const Function& func, int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.push(DataEntry{ms, func, ms, id});
    }

    void reduce(const TimeType& sleep) {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::vector<DataEntry> store;
        store.reserve(m_queue.size());
        while(!m_queue.empty()) {
            store.push_back(std::move(m_queue.top()));
            m_queue.pop();
        }
        for(auto& c : store) {
            c.currentTime -= sleep;
            m_queue.push(c);
        }
    }

    void pop() {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_queue.pop();
    }

    void bless(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_blessed.emplace(id);
    }

    bool blessed(int id) const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_blessed.find(id) != m_blessed.end();
    }

    bool takeBless(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto it = m_blessed.find(id);
        if(it != m_blessed.end()) {
            m_blessed.erase(it);
            return true;
        }
        return false;
    }

    void remove(int id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::vector<DataEntry> store;
        store.reserve(m_queue.size());
        while(!m_queue.empty()) {
            store.push_back(std::move(m_queue.top()));
            m_queue.pop();
        }

        for(auto& c : store) {
            if(c.id != id)
                m_queue.push(c);
        }
    }

    void setNow(bool keepBless = true) {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::vector<DataEntry> store;
        store.reserve(m_queue.size());
        while(!m_queue.empty()) {
            store.push_back(std::move(m_queue.top()));
            m_queue.pop();
        }
        for(auto& c : store) {
            if(!keepBless) {
                const auto it = m_blessed.find(c.id);
                if(it != m_blessed.end()) {
                    m_blessed.erase(it);
                }
            }
            c.currentTime = std::chrono::milliseconds{0};
            m_queue.push(c);
        }
    }


    std::optional<DataEntry> peek() const {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peek");
        std::unique_lock<std::mutex> guard(m_mutex);
        if(!m_queue.empty()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue not empty");
            const auto it = m_queue.top();
            const auto value = std::optional(it);
            guard.unlock(); // I have no idea why RAII wont work, unlock does
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue return");
            return value;
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue is empty");
        return std::nullopt;
    }

    void clear() {
        std::lock_guard<std::mutex> guard(m_mutex);
        while(!m_queue.empty())
            m_queue.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_queue.size();
    }


private:
    struct  MsComp {
        bool operator ()(const DataEntry& a, const DataEntry& b) const {
            return a.currentTime > b.currentTime;
        }
    };
    mutable std::mutex m_mutex;
    std::priority_queue<DataEntry, std::vector<DataEntry>, MsComp> m_queue;
    int m_id = 0;
    std::set<int> m_blessed;
};


class TimerMgr {
public:
    int append(const TimeQueue::TimeType& ms, const TimeQueue::Function& func);
    bool remove(int id);
    bool bless(int id);
    bool blessed(int id) const;
    bool takeBless(int id);
    ~TimerMgr() {
        if(!m_queue.empty()) {
            m_queue.clear();
            m_cv.notify_all();
            m_timerThread.wait();
        }
    }
    void flush(bool doRun);
    TimerMgr() {
#ifdef SINGLETON
        static int assert_count = 0;
        assert(assert_count == 0);
        ++assert_count;
#endif
    }
private:
    void start();
private:
    std::condition_variable m_cv;
    std::future<void> m_timerThread;
    TimeQueue m_queue;
    std::mutex m_waitMutex;
    std::atomic<bool> m_exit = false;
};

}

#endif
