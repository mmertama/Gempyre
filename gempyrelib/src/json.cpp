#include <any>
#include "gempyre_utils.h"
#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<class T>
static GempyreUtils::Result<std::string> containertoString(const std::any& any, int indent) {
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
        return array.dump(indent);
    } else if(const auto* h = std::any_cast<std::unordered_map<std::string, T>>(&any)) {
        auto obj = json::object();
        for(const auto& [k, a] : *h) {
            const auto o = GempyreUtils::to_json_string(a);
            if(!o.has_value()) {
                 return GempyreUtils::make_error<std::string>("Not Hash", k);
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump(indent);
    } else if(const auto* h0 = std::any_cast<std::map<std::string, T>>(&any)) {
        auto obj = json::object();
        for(const auto& [k, a] : *h0) {
            const auto o = GempyreUtils::to_json_string(a);
            if(!o.has_value()) {
                return GempyreUtils::make_error<std::string>("Not Map", k);
            }
            obj.emplace(k, json::parse(o.value()));
        }
        return obj.dump(indent);
    }
    return GempyreUtils::make_error<std::string>("Invalid");
}

GempyreUtils::Result<std::string> GempyreUtils::to_json_string(const std::any& any, JsonMode mode) {
    const auto indent = mode == JsonMode::Compact ? -1 : 1;
    if(const auto* i = std::any_cast<int>(&any)) {
        return json(*i).dump(indent);
    } else if(const auto* d = std::any_cast<double>(&any)) {
        return json(*d).dump(indent);
    } else if(const auto* b = std::any_cast<bool>(&any)) {
        return json(*b).dump(indent);
    } else if(const auto* s = std::any_cast<std::string>(&any)) {
        return json(*s).dump(indent);
    } else if(const auto* n = std::any_cast<std::nullptr_t>(&any)) {
        return json(*n).dump(indent);
    } else if(const auto* c = std::any_cast<const char*>(&any)) {
        return json(std::string(*c)).dump(indent);
    } else {
        const auto v1 = containertoString<std::any>(any, indent);
        if(v1.has_value())
            return v1;
        const auto v3 = containertoString<int>(any, indent);
        if(v3.has_value())
            return v3;
        const auto v4 = containertoString<double>(any, indent);
        if(v4.has_value())
            return v4;
        const auto v5 = containertoString<bool>(any, indent);
        if(v5.has_value())
            return v5;
        const auto v2 = containertoString<std::string>(any, indent);
        if(v2.has_value())
            return v2;
        const auto v6 = containertoString<std::nullptr_t>(any, indent);
        if(v6.has_value())
            return v6;
        const auto v7 = containertoString<const char*>(any, indent);
        if(v7.has_value())
            return v7;
        return GempyreUtils::make_error<std::string>("Invalid value:", any.type().name());
    }
}

template <typename T>
GempyreUtils::Result<std::any> make_map(const json& j) {
    T map;
    for( auto it = j.begin(); it != j.end(); ++it) {
        const auto o = GempyreUtils::json_to_any(it.value().dump());
        if(!o.has_value())
            return GempyreUtils::make_error<std::any>("Bad object");
        map.emplace(it.key(), o.value());
    }
    return std::make_any<T> (map);
}

GempyreUtils::Result<std::any> GempyreUtils::json_to_any(std::string_view str, MapType map_type) {
    const auto j = json::parse(str);
    //if(j.empty())
    //    return GempyreUtils::make_error<std::any>("Empty");
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
        switch (map_type) {
        case  MapType::UnorderedMap:
            return make_map<std::unordered_map<std::string, std::any>>(j);
        case MapType::Map:
            return make_map<std::map<std::string, std::any>>(j);
        }
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

template <typename T, typename M, std::enable_if_t<!std::is_same_v<M, std::vector<std::any>>, int> = 0>
T* next(T* j, const std::string& key) {
    if (const auto jj = std::any_cast<M>(j)) {
            const auto it = jj->find(key);
            if (it != jj->end()) {
                return &(it->second);    
            }
    }
    return nullptr;
}

template <typename T, typename V, std::enable_if_t<std::is_same_v<V, std::vector<std::any>>, int> = 0>
T* next(T* j, const std::string& key) {
    if (const auto jj = std::any_cast<std::vector<std::any>>(j)) {
        const auto index = GempyreUtils::parse<unsigned>(key);
        if(index && *index < jj->size())
            return &jj->at(*index);
        }
    return nullptr;
}


template<typename M>
bool append(std::any* j, const std::string& key, std::any&& val) {
    if (auto jj = std::any_cast<M>(j)) {
        (*jj)[key] = std::move(val);
        return true;    
    }
    return false;
}

template<>
bool append<std::vector<std::any>>(std::any* j, const std::string& key, std::any&& val) {
    if (auto jj = std::any_cast<std::vector<std::any>>(j)) {
        const auto index = GempyreUtils::parse<unsigned>(key);
        if (!index)
            return false;
        const auto i = *index;     
        if (jj->size() <= i)
            jj->resize(i + 1);
        (*jj)[i] = std::move(val);
        return true;    
    }
    return false;
}

template<typename M>
bool remove(std::any* j, const std::string& key) {
    if (auto jj = std::any_cast<M>(j)) {
        auto it = jj->find(key);
        if (it == jj->end())
            return false;
        jj->erase(it);    
        return true;    
    }
    return false;
}

template<>
bool remove<std::vector<std::any>>(std::any* j, const std::string& key) {
    if (auto jj = std::any_cast<std::vector<std::any>>(j)) {
        const auto index = GempyreUtils::parse<unsigned>(key);
        if (!index || *index >= jj->size())
            return false;
        jj->erase(jj->begin() + *index);
        return true;    
    }
    return false;
}

template<typename J, typename I>
GempyreUtils::Result<J*> iterate_path(J* j, I begin, I end) {
    ssize_t success_paths = 0;
    for (auto it = begin; it != end; ++it) {
        auto jj = next<J, std::map<std::string, std::any>>(j, *it);
        if (!jj)
            jj = next<J, std::unordered_map<std::string, std::any>>(j, *it);
        if (!jj)
            jj = next<J, std::vector<std::any>>(j, *it);
        if (!jj) {
                return  GempyreUtils::Result<J*>::make_error(GempyreUtils::join(begin, begin + success_paths, "/"));
            }
        j = jj;
        ++success_paths;    
    }
    return j;
}

GempyreUtils::ResultTrue GempyreUtils::set_json_value(std::any& any, std::string_view path, JsonType&& value) {
    auto path_items = GempyreUtils::split(path, '/');
    if (path_items.empty()) {
        return ResultTrue::make_error("");
    }
    const auto value_name = path_items.back();
    
    auto r = iterate_path(&any, path_items.begin(), path_items.end() - 1);
    if(!r) {
        return GempyreUtils::ResultTrue::make_error(r.error());
    }

    auto j = *r;

    std::any any_val = std::visit(overloaded {
        [](bool v) {return std::make_any<bool>(v);},
        [](int v) {return std::make_any<int>(v);},
        [](double v) {return std::make_any<double>(v);},
        [](nullptr_t v) {return std::make_any<nullptr_t>(v);},
        [](std::string&& v) {return std::make_any<std::string>(v);},
        [](std::vector<std::any>&& v) {return std::make_any<std::vector<std::any>>(v);},
        [](std::map<std::string, std::any>&& v) {return std::make_any<std::map<std::string, std::any>>(v);},
        [](std::unordered_map<std::string, std::any>&& v) {return std::make_any<std::unordered_map<std::string, std::any>>(v);}},
         std::move(value));

    if (append<std::map<std::string, std::any>>(j, value_name, std::move(any_val)))
        return ResultTrue::ok();

    if (append<std::unordered_map<std::string, std::any>>(j, value_name, std::move(any_val)))
        return ResultTrue::ok();

    if (append<std::vector<std::any>>(j, value_name, std::move(any_val)))
        return ResultTrue::ok();

    return ResultTrue::make_error(std::string{path});
}

GempyreUtils::Result<GempyreUtils::JsonType> GempyreUtils::get_json_value(const std::any& any, std::string_view path) {

    auto path_items = GempyreUtils::split(path, '/');
    const auto r = iterate_path(&any, path_items.begin(), path_items.end());

    if (!r) {
        return GempyreUtils::Result<GempyreUtils::JsonType>::make_error(r.error());
    }

    auto j = *r;

    if (const auto v = std::any_cast<nullptr_t>(j))
        return JsonType{*v};

    if (const auto v = std::any_cast<bool>(j))
        return JsonType{*v};

    if (const auto v = std::any_cast<int>(j))
        return JsonType{*v};

    if (const auto v = std::any_cast<double>(j))
        return JsonType{*v};

    if (const auto v = std::any_cast<std::string>(j))
        return JsonType{*v};

    if (const auto v = std::any_cast<std::vector<std::any>>(j))
        return JsonType{*v};                    

    if (const auto v = std::any_cast<std::map<std::string, std::any>>(j))
        return JsonType{*v};            

    if (const auto v = std::any_cast<std::unordered_map<std::string, std::any>>(j))
        return JsonType{*v};            

    return Result<GempyreUtils::JsonType>::make_error(std::string{path});
}

GempyreUtils::ResultTrue GempyreUtils::remove_json_value(std::any& any,  std::string_view path) {
    auto path_items = GempyreUtils::split(path, '/');
    if (path_items.empty()) {
        return ResultTrue::make_error("");
    }
    const auto value_name = path_items.back();

    auto r = iterate_path(&any, path_items.begin(), path_items.end() - 1);

     if(!r) {
        return GempyreUtils::ResultTrue::make_error(r.error());
    }

    auto j = *r;

    if (remove<std::map<std::string, std::any>>(j, value_name))
        return ResultTrue::ok();

    if (remove<std::unordered_map<std::string, std::any>>(j, value_name))
        return ResultTrue::ok();

    if (remove<std::vector<std::any>>(j, value_name))
        return ResultTrue::ok();

    return ResultTrue::make_error(std::string{path});
}