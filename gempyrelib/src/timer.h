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

#include "gempyre_utils.h"

namespace Gempyre {


/**
 * Timers for Gempyre event queue.
 * Start time wraps a function into data and push into queue in next-priority order.
 * When the top item is waited it
 */

class TimeQueue;

class TimerMgr {
public:
    using Function = std::function<void (int)>;
    using Callback = std::function<void (const std::function<void()>& f)>;
    using TimeType = std::chrono::milliseconds;
    int append(const TimerMgr::TimeType& ms,    //at this time
               bool singleShot,                 //do re do?
               const TimerMgr::Function& func,  //this is a parameter
               const TimerMgr::Callback& cb);   //this is a cb run in timer thread, that returns a function that call a func executed in event thread
    bool remove(int id);
    void flush(bool doRun); //do run not used - fix if issue - test everything!
    ~TimerMgr();
    TimerMgr();
private:
    void start();
private:
    std::condition_variable m_cv;
    std::future<void> m_timerThread;
    std::unique_ptr<TimeQueue> m_queue;
    std::mutex m_waitMutex;
    std::atomic<bool> m_exit = false;
};
}

#endif
