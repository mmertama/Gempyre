

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

template <class T, std::enable_if_t<hasKey<T>::value, int>>
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

template <class T, std::enable_if_t<!hasKey<T>::value, int>>
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
std::optional<T> GempyreInternal::query(std::string_view elId, std::string_view queryString, const std::vector<std::string>& queryParams)  {
    
    if(*this != State::RUNNING) {
        return std::nullopt;
    }
    
    const auto queryId = query_id();
    int re_tries = 5;
    for (auto i = 0; i < re_tries && is_running(); ++i) {
        
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Send query", queryId);
        
        add_request([this, queryId, elId, queryString, queryParams]() {
            return send_to(TargetSocket::Ui, json{
                    {"type", "query"},
                    {"query_id", queryId},
                    {"element", elId},
                    {"query", queryString},
                    {"query_params", queryParams}
                    });
        });

        while(is_running()) {   //start waiting the response
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Wait query", queryId);
            const auto wait_status = eventLoop(false, 10000ms);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "query - wait in event loop done, back in main loop", state_str(), wait_status);
            if(*this != State::RUNNING) {
                signal_pending();
                return std::nullopt;
            }

            if (!wait_status)
                break;

            const auto query_response = take_response(queryId);

            if(query_response) {
                const auto item = query_response.value();
                auto response = copy_value<T>(item);
                if(is_error(response)) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Invalid query:", elId, queryString);
                    return std::nullopt;
                }
            return std::make_optional<T>(std::move(response));
            }
        }
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Retry", re_tries,  is_running());
    }
    return std::nullopt;
}
}
