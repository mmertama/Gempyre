#ifndef IDLIST_H
#define IDLIST_H

#include <unordered_map>
#include <mutex>
#include <thread>
#include <optional>

namespace Gempyre {
template <typename T>
class IdList {
public:
    int append(const std::function<T (int id)>& appender) {
        std::lock_guard<std::mutex> guard(m_mutex);
        ++m_index;
        auto r = appender(m_index);
        m_ids.emplace(m_index, std::move(r));
        return m_index;
    }
    int append(const T& id) {
        std::lock_guard<std::mutex> guard(m_mutex);
        ++m_index;
        m_ids.emplace(m_index, id);
        return m_index;
    }
    bool remove(int r) {
        std::lock_guard<std::mutex> guard(m_mutex);
        auto it = m_ids.find(r);
        if(it != m_ids.end()) {
            m_ids.erase(it);
            return true;
        }
        return  false;
    }

    T operator[](int r) const {   //this cannot be reference as beyond mutex the integrity is not granted
         std::lock_guard<std::mutex> guard(m_mutex);
         return m_ids[r];
    }

    bool contains(int r) const {
        return m_ids.find(r) != m_ids.end();
    }

    /*
    std::optional<T> at(int r) const {   //this cannot be reference as beyond mutex the integrity is not granted
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto it = m_ids.find(r);
        return it != m_ids.end() ? std::optional<T>(it->second) : std::nullopt;
    }
 */
    typename std::unordered_map<int, T>::const_iterator begin() const {return m_ids.begin();}
    typename std::unordered_map<int, T>::const_iterator end() const {return m_ids.end();}

private:
    int m_index = 0;
    std::unordered_map<int, T> m_ids;
    mutable std::mutex m_mutex;
};

}


#endif // IDLIST_H
