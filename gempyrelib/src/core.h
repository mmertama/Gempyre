

#include <algorithm>
#include <vector>
#include "gempyre.h"
#include "gempyre_utils.h"
#include "gempyre_internal.h"

namespace Gempyre {

  
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
std::optional<T> Ui::query(const std::string& elId, const std::string& queryString, const std::vector<std::string>& queryParams)  {
    T response;
    if(*m_ui == State::RUNNING) {
        const auto queryId = m_ui->query_id();

        m_ui->addRequest([this, queryId, elId, queryString, queryParams](){
            return m_ui->send({{"type", "query"}, {"query_id", queryId}, {"element", elId},{"query", queryString}},
                                  std::unordered_map<std::string, std::any>{{"query_params", queryParams}});
        });

        for(;;) {   //start waiting the response
            eventLoop(false);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "query - wait in eventloop done, back in mainloop", m_ui->state_str());
            if(*m_ui != State::RUNNING) {
                m_ui->signal_pending();
                break; //we are gone
            }

            const auto query_response = m_ui->take_response(queryId);

            if(query_response) {
               const auto item = query_response.value();
               const auto asString = std::any_cast<std::string>(&item);
               if(asString && *asString == "query_error") {
                   GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Invalid query:", elId, queryString);
                   return std::nullopt;
               }
               copy(item, response);
               break;
            }
        }
    }
    return std::make_optional<T>(response);
}

}
