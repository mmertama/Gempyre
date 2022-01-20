#include "timer.h"
#include "timequeue.h"
#include <cassert>

using namespace Gempyre;

void TimerMgr::start() {
    m_exit = false;
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timers start");
    assert(!m_timerThread.valid());
    m_timerThread = std::async(std::launch::async, [this]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread start");
        while(!m_exit) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop");
            if(m_queue->empty()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop exit");
                break;
            }
            const auto itemOr = m_queue->copyTop();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peeked");
            if(!itemOr.has_value()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer idling");
                std::unique_lock<std::mutex> lock(m_waitMutex);
                m_cv.wait(lock);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer idle end");
                continue;
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread has value");
            const auto data = itemOr.value(); //value is shared pointer thus is floats even killed, and we wait
            const auto currentSleep = data.currentTime;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread wait", currentSleep.count());
            if(currentSleep > std::chrono::milliseconds{0}) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread lock active");
                std::unique_lock<std::mutex> lock(m_waitMutex);
                const auto begin = std::chrono::steady_clock::now();
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer wait now:", data.id);
                m_cv.wait_for(lock, std::chrono::duration(currentSleep));
                const auto end = std::chrono::steady_clock::now(); //we may have had an early  wakeup
                const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer awake id:", data.id , currentSleep.count(), actualWait.count(), m_queue->size());
                m_queue->reduce(actualWait);  //so we see if we are still there, and restart
                continue; //find a new
            }
            if(!m_queue->setPending(data.id))
                continue;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer pop id:", data.id, m_queue->size());

            const auto begin = std::chrono::steady_clock::now();

            data.func(data.id);
            if(!m_exit)
                m_callWait.wait();

            const auto end = std::chrono::steady_clock::now(); //we may have had an early  wakeup
            const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
            m_queue->reduce(actualWait);  //so we see if we are still there, and restart
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread ended", m_queue->size(), m_exit.load());
    });
}

int TimerMgr::append(const TimeQueue::TimeType& ms, bool singleShot, const TimeQueue::Function& timerFunc, const Callback& cb) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_exit = false;
    const auto id = m_queue->append(ms, [singleShot, timerFunc, this, cb] (int id) {
         GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Timer callback", id);
        cb([singleShot, timerFunc, id, this]() {
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
        });
    });

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer append", id, m_queue->size());
    if(!m_timerThread.valid()) {
        start();
    }
    m_cv.notify_all(); //if appeded thread is done, priot que may have changed
    return id;
}


bool TimerMgr::remove(int id) {
    if(!m_exit) { //on exit queue is cleaned
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue->remove(id);
    }
    m_cv.notify_all();  //if currently waiting has been waiting thing may have changed
    return true;
}

void TimerMgr::flush(bool doRun) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "flush", m_queue->empty());
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if(!m_queue->empty()) {
        m_queue->setNow(doRun);
    }
    m_exit = true;
    m_callWait.signal();
    m_cv.notify_all();
    if(m_timerThread.valid()) // it CAN get invalidated (at least when some breakpoints are set)
        m_timerThread.wait();
    m_queue->clear();
    m_timerThread = {};
}

void TimerMgr::clear() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if(!m_queue->empty()) {
        m_queue->clear();
        m_cv.notify_all();
        assert(m_queue->empty());
    }
}

TimerMgr::~TimerMgr() {
    m_exit = true;
    clear();
}

TimerMgr::TimerMgr() : m_queue(std::make_unique<TimeQueue>()) {
}
