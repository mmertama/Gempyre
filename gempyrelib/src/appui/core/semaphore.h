#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#ifdef WASM
    #error "Semaphores are not supported in WASM builds"
#endif


#include <mutex>
#include <condition_variable>


namespace Gempyre {

class Semaphore {
public:
    explicit Semaphore(int count = 0) : m_count(count) {}
    void signal() {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_count++;
        //notify the waiting threads
        m_cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(m_mtx);
        while(m_count == 0) {
            //wait on the mutex until notify is called
            m_cv.wait(lock);
        }
        --m_count;
    }

    bool wait(const std::chrono::milliseconds& ms) {
        std::unique_lock<std::mutex> lock(m_mtx);
        while(m_count == 0) {
            //wait on the mutex until notify is called
            if(m_cv.wait_for(lock, ms) ==  std::cv_status::timeout) {
                return false;
            }
        }
        --m_count;
        return true;
    }

    bool undo() {
        std::unique_lock<std::mutex> lock(m_mtx);
        if(m_count < 0)
            return true;
        m_count = 0;
        return false;
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(m_mtx);
        return m_count == 0;
    }

private:
    mutable std::mutex m_mtx{};
    std::condition_variable m_cv{};
    int m_count{0};
};
}
#endif // SEMAPHORE_H
