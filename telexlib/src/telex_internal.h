
#include "server.h"
#include "semaphore.h"
#include "eventqueue.h"
#include <algorithm>

namespace Telex {


/*
template <class, class = void>
struct has_key : std::false_type{};

// specialized as has_member< T , void > or discarded (sfinae)
template <class T>
struct has_key<T, std::void_t<decltype(T::key_type)>> : std::true_type{};
*/


template<typename T, typename _ = void>
struct hasKey : std::false_type {};

template<typename... Ts>
struct isContainerHelper {};

template<typename T>
struct hasKey<T,
        std::conditional_t<false,
            isContainerHelper<typename T::value_type, typename T::key_type>, void >> : public std::true_type {};

template <class T, std::enable_if_t<hasKey<T>::value, int> = 0>
 void copy(const std::any& d, T& response) {
    const auto attr = std::any_cast<Server::Object>(d);
    std::for_each(attr.begin(), attr.end(), [&response](const auto& p) {
        response.emplace(p.first, std::any_cast<typename std::decay_t<decltype(response)>::mapped_type>(p.second));
    });
}

template <class T, std::enable_if_t<!hasKey<T>::value, int> = 0>
 void copy(const std::any& d, T& response) {
    const auto attr = std::any_cast<Server::Array>(d);
    std::for_each(attr.begin(), attr.end(), [&response](const auto& p) {
        response.push_back(std::any_cast<typename std::decay_t<decltype(response)>::value_type>(p));
    });
}

template <>
 inline void copy(const std::any& d, std::string& response) {
    const auto attr = std::any_cast<std::string>(d);
    response = attr;
}

template<class T>
std::optional<T> Ui::query(const std::string& elId, const std::string& queryString)  {
    T response;
    if(m_status == State::RUNNING) {
        const auto queryId = std::to_string(m_server->queryId());

        addRequest([this, queryId, elId, queryString](){
            return m_server->send({{"type", "query"}, {"query_id", queryId}, {"element", elId},{"query", queryString}});
        });

        for(;;) {   //start waiting the response
            eventLoop();
            TelexUtils::log(TelexUtils::LogLevel::Debug, "query - wait in eventloop done, back in mainloop", toStr(m_status));
            if(m_status != State::RUNNING) {
                m_sema->signal();
                break; //we are gone
            }

            if(m_responsemap->contains(queryId)) {
               const auto item = m_responsemap->take(queryId);
               const auto asString = std::any_cast<std::string>(&item);
               if(asString && *asString == "query_error") {
                   TelexUtils::log(TelexUtils::LogLevel::Debug, "Invalid query:", elId, queryString);
                   return std::nullopt;
               }
               copy(item, response);
               break;
            }
        }
    }
    return std::make_optional<T>(response);
}

inline void Ui::addRequest(std::function<bool()>&& f) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requestqueue.emplace_back(f);
    m_sema->signal();
}

}
