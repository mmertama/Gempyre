#include "base64.h"
#include <iostream>

using namespace Base64;

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(Byte c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Base64::encode(const Bytes& bytes) {
    return encode(bytes.data(), bytes.size());
}

std::string Base64::encode(const Byte* buf, size_t bufLen) {
  std::string ret;
  unsigned i = 0;
  Byte char_array_3[3];
  Byte char_array_4[4];

  while (bufLen--) {
    char_array_3[i++] = *(buf++);
    if (i == 3) {
      char_array_4[0] = static_cast<Byte>((char_array_3[0] & 0xfc) >> 2);
      char_array_4[1] = static_cast<Byte>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
      char_array_4[2] = static_cast<Byte>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(unsigned j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = static_cast<Byte>((char_array_3[0] & 0xfc) >> 2);
    char_array_4[1] = static_cast<Byte>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
    char_array_4[2] = static_cast<Byte>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (unsigned j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';
  }

  return ret;
}

Bytes Base64::decode(std::string_view const& encoded_string) {
  Bytes ret;
  auto in_len = encoded_string.size();
  if(in_len == 0) {
      return ret;
  }
  auto i = 0U;
  auto in_ = 0U;
  Byte char_array_4[4], char_array_3[3];

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(static_cast<Byte>(encoded_string[in_]))) {
    char_array_4[i++] = static_cast<Byte>(encoded_string[in_]);
    in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = static_cast<Byte>(base64_chars.find(static_cast<decltype(base64_chars)::value_type>(char_array_4[i])));

      char_array_3[0] = static_cast<Byte>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
      char_array_3[1] = static_cast<Byte>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
      char_array_3[2] = static_cast<Byte>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

      for (i = 0; (i < 3); i++)
          ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (unsigned j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (int j = 0; j < 4; j++)
      char_array_4[j] = static_cast<Byte>(base64_chars.find(static_cast<decltype(base64_chars)::value_type>(char_array_4[j])));

    char_array_3[0] = static_cast<Byte>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
    char_array_3[1] = static_cast<Byte>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
    char_array_3[2] = static_cast<Byte>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

    for (unsigned j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }

  return ret;
}
