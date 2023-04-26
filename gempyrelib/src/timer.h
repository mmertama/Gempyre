#ifndef _GEMPYRETIMER_H_
#define _GEMPYRETIMER_H_

#include <chrono>
#include <set>
#include <tuple>
#include <functional>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <future>
#include <set>

#include "semaphore.h"
#include "gempyre_utils.h"

namespace Gempyre {


/**
 * Timers for Gempyre event queue.
 * Start time wraps a function into data and push into queue in next-priority order.
 * When the top item is waited it
 */

class TimeQueue;
class TimerData;

class TimerMgr {
public:
    using Function = std::function<void (int)>;
    using Callback = std::function<void (int)>;
    using TimeType = std::chrono::milliseconds;
    int append(const TimerMgr::TimeType& ms,    //at this time
               bool singleShot,                 //do re do?
               TimerMgr::Callback&& cb);   //this is a cb run in timer thread, that returns a function that call a func executed in event thread
    bool remove(int id);
    void flush(bool do_run); 
    ~TimerMgr();
    TimerMgr();
    bool isValid() const {return m_timerThread.valid();}
private:
    void start();
    void onElapsed(const TimerData& data) ;
private:
    std::condition_variable m_cv{};
    std::future<void> m_timerThread{};
    std::unique_ptr<TimeQueue> m_queue{};
    std::mutex m_waitMutex{};
    std::atomic<bool> m_exit{false};
    Semaphore m_callWait{};
    std::mutex m_queueMutex{};
};
}

#endif
