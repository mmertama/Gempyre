

#include <algorithm>
#include <vector>
#include "gempyre.h"
#include "gempyre_utils.h"
#include "gempyre_internal.h"
#include "data.h"

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

// fwd declaration for recursion
template <class T, std::enable_if_t<hasKey<T>::value, int> = 0>  T copy_value(const Server::Value& d);
template <class T, std::enable_if_t<!hasKey<T>::value, int> = 0> T copy_value(const Server::Value& d);
template <> inline std::string copy_value(const Server::Value& d);

template <class T, std::enable_if_t<hasKey<T>::value, int> = 0>
 T copy_value(const Server::Value& obj) {
    assert(obj.is_object());
    T response {};
    for(const auto& [k, v] : obj.items()) {
        using get_t = typename std::decay_t<decltype(response)>::mapped_type;
        auto value = copy_value<get_t>(v);
        response.emplace(k, std::move(value));
    }
    return response;
}

template <class T, std::enable_if_t<!hasKey<T>::value, int> = 0>
 T copy_value(const Server::Value& array) {
    assert(array.is_array());
    T response {};
    for(const auto& v : array) {
        using get_t = typename std::decay_t<T>::value_type;
        auto value = copy_value<get_t>(v);
        response.push_back(std::move(value));
    }
    return response;
}


template <>
 inline std::string copy_value(const Server::Value& d) {
    return GempyreInternal::to_string(d);
}

template<typename T>
bool is_error(const T&) {return false;}

template<>
inline bool is_error(const std::string& s) {return s == "query_error";}

template<class T>
std::optional<T> Ui::query(const std::string& elId, const std::string& queryString, const std::vector<std::string>& queryParams)  {
    if(*m_ui == State::RUNNING) {
        const auto queryId = m_ui->query_id();

        m_ui->add_request([this, queryId, elId, queryString, queryParams](){
            return m_ui->send({{"type", "query"}, {"query_id", queryId}, {"element", elId},{"query", queryString}},
                                  std::unordered_map<std::string, std::any>{{"query_params", queryParams}});
        });

        while(m_ui->is_running()) {   //start waiting the response
            eventLoop(false);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "query - wait in eventloop done, back in mainloop", m_ui->state_str());
            if(*m_ui != State::RUNNING) {
                m_ui->signal_pending();
                break;
            }

            const auto query_response = m_ui->take_response(queryId);

            if(query_response) {
                const auto item = query_response.value();
                auto response = copy_value<T>(item);
                if(is_error(response)) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Invalid query:", elId, queryString);
                    break;
                }
              return std::make_optional<T>(std::move(response));
            }
        }
    }
    return std::nullopt;
}
}
