#include "timer.h"


using namespace Telex;

void TimerMgr::start() {
    m_exit = false;
    m_timerThread = std::async([this](){
        TelexUtils::log(TelexUtils::LogLevel::Debug, "timer thread start");
        for(;;) {
            const auto itemOr = m_queue.peek();
            if(!itemOr.has_value())
                break; //empty
            const auto data = itemOr.value();
            const auto currentSleep = data.currentTime;
            if(currentSleep > std::chrono::milliseconds{0}) {
                std::unique_lock<std::mutex> lock(m_waitMutex);
                const auto begin = std::chrono::steady_clock::now();
                TelexUtils::log(TelexUtils::LogLevel::Debug, "timer wait now:", data.id);
                m_cv.wait_for(lock, std::chrono::duration(currentSleep)); //more or less poll
                const auto end = std::chrono::steady_clock::now();
                const auto actualWait = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
                TelexUtils::log(TelexUtils::LogLevel::Debug, "timer awake id:", data.id , currentSleep.count(), actualWait.count(), m_queue.size());
                m_queue.reduce(actualWait);
                continue; // we have slept
            }
            TelexUtils::log(TelexUtils::LogLevel::Debug, "timer pop id:", data.id, m_queue.size());
            m_queue.pop();
            data.func(data.id);
            if(!m_exit) {
                m_queue.reAppend(data.initialTime, data.func, data.id); //restart it
            } else {
                 TelexUtils::log(TelexUtils::LogLevel::Debug, "timer exit on, id:", data.id, m_queue.size());
            }
        }
        TelexUtils::log(TelexUtils::LogLevel::Debug, "timer thread ended", m_queue.size());
    });
}

int TimerMgr::append(const TimeQueue::TimeType& ms, const TimeQueue::Function& func) {
    const auto doStart = m_queue.empty();
    const auto id = m_queue.append(ms, func);

    TelexUtils::log(TelexUtils::LogLevel::Debug, "timer append", id, m_queue.size());

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
        TelexUtils::log(TelexUtils::LogLevel::Debug, "flush");
        m_queue.setNow(doRun); //There was a feature that on flush (on exit) all timers are run.
        m_exit = true;
        m_cv.notify_all();
        m_timerThread.wait();
    }
}
