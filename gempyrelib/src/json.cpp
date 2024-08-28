#include <any>
#include "gempyre_utils.h"
#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;

template<class T>
static GempyreUtils::Result<std::string> containertoString(const std::any& any) {
    if(const auto* v = std::any_cast<std::vector<T>>(&any)) {
        auto array = json::array();
        int p = 0;
        for(const auto& a : *v) {
            const auto o = GempyreUtils::to_json_string(a);
            ++p;
            if(!o.has_value()) {
                return GempyreUtils::make_error<std::string>("Not Vec", p);
            }
            array.push_back(json::parse(o.value()));
        }
        return array.dump();
    } else if(const auto* h = std::any_cast<std::unordered_map<std::string, T>>(&any)) {
        auto obj = json::object();
        for(const auto& [k, a] : *h) {
            const auto o = GempyreUtils::to_json_string(a);
            if(!o.has_value()) {
                 return GempyreUtils::make_error<std::string>("Not Hash", k);
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump();
    } else if(const auto* h0 = std::any_cast<std::map<std::string, T>>(&any)) {
        auto obj = json::object();
        for(const auto& [k, a] : *h0) {
            const auto o = GempyreUtils::to_json_string(a);
            if(!o.has_value()) {
                return GempyreUtils::make_error<std::string>("Not Map", k);
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump();
    }
    return GempyreUtils::make_error<std::string>("Invalid");
}

GempyreUtils::Result<std::string> GempyreUtils::to_json_string(const std::any& any) {
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
        const auto v1 = containertoString<std::any>(any);
        if(v1.has_value())
            return v1;
        const auto v3 = containertoString<int>(any);
        if(v3.has_value())
            return v3;
        const auto v4 = containertoString<double>(any);
        if(v4.has_value())
            return v4;
        const auto v5 = containertoString<bool>(any);
        if(v5.has_value())
            return v5;
        const auto v2 = containertoString<std::string>(any);
        if(v2.has_value())
            return v2;
        const auto v6 = containertoString<std::nullptr_t>(any);
        if(v6.has_value())
            return v6;
        const auto v7 = containertoString<const char*>(any);
        if(v7.has_value())
            return v7;
        return GempyreUtils::make_error<std::string>("Invalid value:", any.type().name());
    }
}


GempyreUtils::Result<std::any> GempyreUtils::json_to_any(std::string_view str) {
    const auto j = json::parse(str);
    if(j.empty())
        return GempyreUtils::make_error<std::any>("Empty");
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
            const auto o = json_to_any(it.value().dump());
            if(!o.has_value())
                return GempyreUtils::make_error<std::any>("Bad object");
            map.emplace(it.key(), o.value());
        }
        return std::make_any<decltype (map)> (map);
    }
    if(j.is_array()) {
        std::vector<std::any> vec;
        for( auto it = j.begin(); it != j.end(); ++it) {
            const auto o = json_to_any(it.value().dump());
            if(!o.has_value())
                return GempyreUtils::make_error<std::any>("Bad array");
            vec.push_back(o.value());
        }
        return std::make_any<decltype (vec)> (vec);
    }
    return GempyreUtils::make_error<std::any>("Bad value");
}

