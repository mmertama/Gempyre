#ifndef BASE64_H
#define BASE64_H

//modified from https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/13935718

#include <vector>
#include <string>

namespace Base64 {

using Byte = unsigned char;
using Bytes = std::vector<Byte>;

std::string encode(const Bytes& bytes);
std::string encode(const Byte* bytes, size_t amount);
Bytes decode(std::string_view const& data);
}


#endif // BASE64_H
