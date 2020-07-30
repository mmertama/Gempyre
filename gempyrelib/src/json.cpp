#include "json.h"
#include "gempyre_utils.h"
#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;
using namespace Gempyre;

template<class T>
std::optional<std::string> ContainertoString(const std::any& any) {
    if(const auto* v = std::any_cast<std::vector<T>>(&any)) {
        auto array = json::array();
        int p = 0;
        for(const auto& a : *v) {
            const auto o = toString(a);
            ++p;
            if(!o.has_value()) {
                 GempyreUtils::log(GempyreUtils::LogLevel::Error, "Vec", p);
                return std::nullopt;
            }
            array.push_back(json::parse(o.value()));
        }
        return array.dump();
    } else if(const auto* h = std::any_cast<std::unordered_map<std::string, T>>(&any)) {
        auto obj = json();
        for(const auto& [k, a] : *h) {
            const auto o = toString(a);
            if(!o.has_value()) {
                 GempyreUtils::log(GempyreUtils::LogLevel::Error, "Hash", k);
                return std::nullopt;
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump();
    } else if(const auto* h = std::any_cast<std::map<std::string, T>>(&any)) {
        auto obj = json();
        for(const auto& [k, a] : *h) {
            const auto o = toString(a);
            if(!o.has_value()) {
                GempyreUtils::log(GempyreUtils::LogLevel::Error, "Map", k);
                return std::nullopt;
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump();
    }
    return std::nullopt;
}

std::optional<std::string> Gempyre::toString(const std::any& any) {
    if(const auto* i = std::any_cast<int>(&any)) {
        return json(*i).dump();
    } else if(const auto* d = std::any_cast<double>(&any)) {
        return json(*d).dump();
    } else if(const auto* b = std::any_cast<bool>(&any)) {
        return json(*b).dump();
    } else if(const auto* s = std::any_cast<std::string>(&any)) {
        return json(*s).dump();
    } else if(const auto* n = std::any_cast<std::nullptr_t>(&any)) {
        return json(*n).dump();
    } else if(const auto* c = std::any_cast<const char*>(&any)) {
        return json(std::string(*c)).dump();
    } else {
        const auto v1 = ContainertoString<std::any>(any);
        if(v1.has_value())
            return v1;
        const auto v3 = ContainertoString<int>(any);
        if(v3.has_value())
            return v3;
        const auto v4 = ContainertoString<double>(any);
        if(v4.has_value())
            return v4;
        const auto v5 = ContainertoString<bool>(any);
        if(v5.has_value())
            return v5;
        const auto v2 = ContainertoString<std::string>(any);
        if(v2.has_value())
            return v2;
        const auto v6 = ContainertoString<std::nullptr_t>(any);
        if(v6.has_value())
            return v6;
        const auto v7 = ContainertoString<const char*>(any);
        if(v7.has_value())
            return v7;
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Invalid value:", any.type().name());
        return std::nullopt;
    }
}


std::optional<std::any> Gempyre::toAny(const std::string& str) {
    const auto j = json::parse(str);
    if(j.empty())
        return std::nullopt;
    if(j.is_null())
        return std::make_any<std::nullptr_t>(nullptr);
    if(j.is_boolean())
        return std::make_any<bool>(j.get<bool>());
    if(j.is_number_integer())
        return std::make_any<int>(j.get<int>());
    if(j.is_number())
        return std::make_any<double>(j.get<double>());
    if(j.is_string())
        return std::make_any<std::string>(j.get<std::string>());
    if(j.is_object()) {
        std::unordered_map<std::string, std::any> map;
        for( auto it = j.begin(); it != j.end(); ++it) {
            const auto o = toAny(it.value().dump());
            if(!o.has_value())
                return std::nullopt;
            map.emplace(it.key(), o.value());
        }
        return std::make_any<decltype (map)> (map);
    }
    if(j.is_array()) {
        std::vector<std::any> vec;
        for( auto it = j.begin(); it != j.end(); ++it) {
            const auto o = toAny(it.value().dump());
            if(!o.has_value())
                return std::nullopt;
            vec.push_back(o.value());
        }
        return std::make_any<decltype (vec)> (vec);
    }
    return std::nullopt;
}

