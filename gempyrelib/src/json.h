#ifndef JSON_H
#define JSON_H

#include <string>
#include <any>
#include <optional>

namespace Gempyre {
std::optional<std::string> toString(const std::any& any);
std::optional<std::any> toAny(const std::string& str);
}

#endif // JSON_H
