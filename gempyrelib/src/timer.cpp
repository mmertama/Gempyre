#include "timer.h"


using namespace Gempyre;

void TimerMgr::start() {
    m_exit = false;
    m_timerThread = std::async([this]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread start");
        for(;;) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop");
            const auto itemOr = m_queue.peek();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer queue peeked");
            if(!itemOr.has_value()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread loop exit");
                break; //empty
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread has value");
            const auto data = itemOr.value();
            const auto currentSleep = data.currentTime;
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread wait", currentSleep.count());
            if(currentSleep > std::chrono::milliseconds{0}) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread lock active");
                std::unique_lock<std::mutex> lock(m_waitMutex);
                const auto begin = std::chrono::steady_clock::now();
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer wait now:", data.id);
                m_cv.wait_for(lock, std::chrono::duration(currentSleep)); //more or less poll
                const auto end = std::chrono::steady_clock::now();
                const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer awake id:", data.id , currentSleep.count(), actualWait.count(), m_queue.size());
                m_queue.reduce(actualWait);
                continue; // we have slept
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer pop id:", data.id, m_queue.size());
            m_queue.pop();
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread function");
            data.func(data.id);
            if(!m_exit) {
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "timer thread reappend");
                m_queue.reAppend(data.initialTime, data.func, data.id); //restart it
            } else {
                 GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer exit on, id:", data.id, m_queue.size());
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer thread ended", m_queue.size());
    });
}

int TimerMgr::append(const TimeQueue::TimeType& ms, const TimeQueue::Function& func) {
    const auto doStart = m_queue.empty();
    const auto id = m_queue.append(ms, func);

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "timer append", id, m_queue.size());

    if(doStart) {
        start();
    }
    m_cv.notify_all(); //if appeded thread is done, priot que may have changed
    return id;
}


bool TimerMgr::remove(int id) {
    if(m_queue.empty())
        return false;
    m_queue.remove(id);
    m_cv.notify_all();  //if currently waiting has been waiting thing may have changed
    return true;
}

bool TimerMgr::bless(int id) {
    if(m_queue.empty())
        return false;
    m_queue.bless(id);
    return true;
}

bool TimerMgr::blessed(int id) const {
    if(m_queue.empty())
        return false;
    return m_queue.blessed(id);
}

bool TimerMgr::takeBless(int id) {
    return m_queue.takeBless(id);
}

void TimerMgr::flush(bool doRun) {
    if(!m_queue.empty()) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "flush");
        m_queue.setNow(doRun); //There was a feature that on flush (on exit) all timers are run.
        m_exit = true;
        m_cv.notify_all();
        m_timerThread.wait();
    }
}
