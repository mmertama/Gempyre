#include "timer.h"
#include "timequeue.h"
#include <cassert>

using namespace Gempyre;
using namespace std::chrono_literals;

void TimerMgr::onElapsed(const TimerData& data)  {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Timer running", data.id());
    if(data.singleShot()) {
        remove(data.id());     // does notify
    }
    data.call();      //call timer
    if(!data.singleShot()) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue->restoreIf(data.id());     // restore priority
        m_cv.notify_all();
    }
    m_callWait.signal();
}

void TimerMgr::start() {
   assert(!m_exit);     // not exiting
   assert(!m_timerThread.valid());  // not running
   GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timers start");

   m_timerThread = std::async(std::launch::async, [this]() {

        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread start");

        while(!m_exit) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop");
            if(m_queue->empty()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop exit");
                break;
            }
            const auto itemOr = m_queue->copyTop(); // take 1st item from prio queue
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peeked");

            if(!itemOr.has_value()) {  // if there is no top
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer idling");
                std::unique_lock<std::mutex> lock(m_waitMutex);
                m_cv.wait(lock);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer idle end");
                continue;            // wait an event and the re-loop
            }

            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread has value");
            const auto data = itemOr.value(); //value is shared pointer thus is floats even killed, and we wait
            const auto currentSleep = data.currentTime(); // expected wait
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread wait", currentSleep.count());

            if(currentSleep > std::chrono::milliseconds{0}) { // if wait is > 0
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread lock active");
                std::unique_lock<std::mutex> lock(m_waitMutex);
                const auto begin = std::chrono::steady_clock::now();
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer wait now:", data.id());
                m_cv.wait_for(lock, std::chrono::duration(currentSleep)); // do actual wait
                const auto end = std::chrono::steady_clock::now(); //we may have had an early  wakeup
                const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer awake id:", data.id() , currentSleep.count(), actualWait.count(), m_queue->size());
                m_queue->reduce(actualWait);  //so we see if we are still there, and restart
                continue; //find a new
            } else {  // no wait is <= 0

                if(!m_queue->setPending(data.id()))
                    continue;

                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer pop id:", data.id(), m_queue->size());

                const auto begin = std::chrono::steady_clock::now();

                onElapsed(data);

                if(!m_exit) {
                    if(!m_callWait.wait(300s)) {  // this is maximum we can wait func
                        GempyreUtils::log(GempyreUtils::LogLevel::Error,
                                          "timer timeout",
                                          data.id(),
                                          m_queue->size()
                                          );
#ifdef GEMPYRE_IS_DEBUG
                        gempyre_utils_fatal("Timer thread blocked");
#endif
                    }
                }

                const auto end = std::chrono::steady_clock::now(); //we may have had an early  wakeup
                const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
                m_queue->reduce(actualWait);  //so we see if we are still there, and restart
                }
            }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread ended", m_queue->size(), m_exit.load());
    });
}

int TimerMgr::append(const TimerData::TimeType& ms, bool singleShot, Callback&& callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if(m_timerThread.valid() && m_queue->empty()) {
        // it timer running / not invalidated, but empty, we wait and invalidate
        // otherwise its not restarted
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Timer wait finish...");
        m_timerThread.get();
    }
    m_exit = false;

    /*
    auto c = [singleShot, timerFunc, this, callback] (int id) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Timer callback", id);
            auto callback_function = [singleShot, timerFunc, id, this]() {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Timer running", id);
                if(singleShot) {
                    remove(id);     // does notify
                }
                timerFunc(id);      //call timer
                if(!singleShot) {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    m_queue->restoreIf(id);     // restore priority
                    m_cv.notify_all();
                }
                m_callWait.signal();
            };
            callback(callback_function);
        };*/

    const auto id = m_queue->append(ms, singleShot, std::move(callback));

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer append", id, m_queue->size());
    if(!m_timerThread.valid()) {
        start();
    }
    m_cv.notify_all(); //if appeded thread is done, priot que may have changed
    return id;
}


bool TimerMgr::remove(int id) {
    bool ok = true;
    if(!m_exit) { //on exit queue is cleaned
        std::lock_guard<std::mutex> lock(m_queueMutex);
        ok = !m_queue->remove(id);
    }
    m_cv.notify_all();  //if currently waiting has been waiting thing may have changed
    return ok;
}

void TimerMgr::flush(bool do_run) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "flush", m_queue->empty());
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if(!m_queue->empty()) {
        m_queue->setNow(do_run);
    }
    m_exit = true;
    m_callWait.signal();
    m_cv.notify_all();
    if(m_timerThread.valid()) {// it CAN get invalidated (at least when some breakpoints are set)
        m_timerThread.wait();
        m_queue->clear();
        m_timerThread.get();
    }
}


TimerMgr::~TimerMgr() {
    m_exit = true;
    flush(false);
}

TimerMgr::TimerMgr() : m_queue(std::make_unique<TimeQueue>()) {
}
