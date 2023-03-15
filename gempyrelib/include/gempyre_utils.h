#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <chrono>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <optional>
#include <variant>
#include <functional>
#include <algorithm>
#include <future>
#include <limits>
#include <iomanip>
#include <any>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * gempyre_utils.h
  * =====
  * Gempyre GUI Framework
  * -------------
  *
  * gempyre_utils.h contains a collection utility functions used internally within Gempyre
  * implementation and test applications.
  *
  */

#define GEMPYREUTILSDEBUG(x) GempyreUtils::log(Utils::LogLevel::Debug, x, __FILE__, __LINE__)
//also release build assert
#define gempyre_utils_assert(b) (b || GempyreUtils::do_fatal("Panic!", nullptr, __FILE__, __LINE__))
#define gempyre_utils_assert_x(b, x) (b || GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__))
#define gempyre_utils_assert_x_f(b, x, f) (b || GempyreUtils::do_fatal(x, f, __FILE__, __LINE__))
#define gempyre_utils_fatal(x) GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__)
#define gempyre_utils_fatal_f(x, f) GempyreUtils::do_fatal(x, f, __FILE__, __LINE__)
#define gempyre_utils_auto_clean(p, f) std::unique_ptr<std::remove_pointer<decltype(p)>::type, decltype(&f)> _ ## p (p, &f)
#define gempyre_utils_auto_close(p, f) GempyreUtils::_Close<std::decay_t<decltype(p)>, decltype(&f)> _ ## p (p, &f)


#ifdef WINDOWS_EXPORT
	#define UTILS_EX __declspec(dllexport)
#else
    #define UTILS_EX
#endif

#ifdef _MSC_VER
using SSIZE_T = long long;
#else
using SSIZE_T = ssize_t;
#endif

namespace GempyreUtils {
/**
 * @brief The LogLevel enum
 */
enum class LogLevel : int {None, Fatal, Error, Warning, Info, Debug, Debug_Trace};

/// Parent class for LogWriters
class LogWriter {
public:
    LogWriter();
    ~LogWriter();
    /// Return header of class, called before every line, default just returns a timestamp and loglevel string.
    virtual std::string header(LogLevel logLevel);
    /// Implement to do the write to the medium. The buffer is 0 terminated, at position count.
    virtual bool do_write(const char* buffer, size_t count) = 0;
    /// override to return true if this write supports ANSI colors, default false
    virtual bool has_ansi() const;
private:
    LogWriter* m_previousLogWriter;
};

/// Courtesy class to write log into files, see  setLogWriter
class UTILS_EX FileLogWriter : public LogWriter {
public:
    FileLogWriter(const std::string& path);
protected:
    bool do_write(const char* buffer, size_t count) override;
protected:
    std::ofstream m_file;
};

class UTILS_EX StreamLogWriter : public LogWriter {
public:
    StreamLogWriter(std::ostream& os);
protected:
    bool do_write(const char* buffer, size_t count) override;
protected:
    std::ostream& m_os;
};

UTILS_EX void set_log_level(LogLevel level);
UTILS_EX LogLevel log_level(); 
UTILS_EX std::string to_str(LogLevel l);
UTILS_EX std::ostream log_stream(LogLevel logLevel);

template <typename T, typename ...Args>
inline void log_line(LogLevel level, std::ostream& os, const T& e, Args... args) {
    os << e << " ";
    log_line(level, os, args...);
}

// predeclation
UTILS_EX void process_exit(int);
template<typename T>
inline void log_line(LogLevel level, std::ostream& os, const T& e) {
    os << e << std::endl;
    if(level == LogLevel::Fatal)  {
        process_exit(99);
    }
}

template <typename T, typename ...Args>
inline void log(LogLevel level, const T& e, Args... args) {
    if(level <= log_level()) {
        auto os = log_stream(level);
        log_line(level, os, e, args...);
    }
}


template <LogLevel level, typename T, typename ...Args>
inline void write_log(const T& e, Args... args) {
    log(level, e, args...);
}

template <typename T, typename ...Args>
inline void log_debug(const T& e, Args... args) {
    write_log<LogLevel::Debug, T, Args...>(e, args...);
}


inline bool do_fatal(const std::string_view txt, std::function<void()> f, const char* file, int line) {
    if(f) f();
    log(LogLevel::Fatal, txt, "at", file, "line:", line);
    return false;
}


#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define GEM_DEBUG(...) GempyreUtils::log(GempyreUtils::LogLevel::Debug, __PRETTY_FUNCTION__, __VA_ARGS__)

}




/**
 * @namespace GempyreUtils
 */
namespace GempyreUtils {

//Helper for C memory management
template <typename T, typename A>
class _Close {
public:
    _Close(T ref, A&& f) : t(ref), d(f) {}
    ~_Close() {(void) this->d(this->t);}
private:
    T t;
    A d;
};

enum class ArgType{NO_ARG, REQ_ARG, OPT_ARG};
using ParamList = std::vector<std::string>;
using Options = std::multimap<std::string, std::string>;
using Params = std::tuple<ParamList, Options>;
/// parse arguments
UTILS_EX Params parse_args(int argc, char* argv[], const std::initializer_list<std::tuple<std::string, char, ArgType>>& args);


UTILS_EX void init();
UTILS_EX std::string current_time_string();

UTILS_EX std::string last_error();


/**
  * String Utils
  */

UTILS_EX std::string qq(const std::string& s);

UTILS_EX std::string chop(const std::string& s);

UTILS_EX std::string chop(const std::string& s, const std::string& chopped);

UTILS_EX std::string substitute(const std::string& str, const std::string& substring,  const std::string& substitution);

UTILS_EX std::string trimmed(const std::string& s);

template <typename T>
T convert(const std::string& source) {
    std::istringstream ss(source);
    typename std::remove_const<T>::type v{};
    ss.operator>>(v); //overloads have similar conversions MSVC19
    return v;
}

template <>
inline std::string convert<std::string>(const std::string& source)
{
    return source;
}

template <typename T>
std::optional<T> parse(const std::string& source) {
    std::istringstream ss(source);
    T v;
    static_cast<std::istream&>(ss) >> v;   //MSVC said it would be othwerwise ambiguous
    return !ss.fail() ? std::make_optional(v) : std::nullopt;
}


template <class T>
std::string to_low(const T& str) {
    std::string n;
    std::transform(str.begin(), str.end(), std::back_inserter(n),
                                    [](auto c){return std::tolower(c);});
    return n;
}


 template< typename T >
 std::string to_hex(T ival) {
   std::stringstream stream;
   stream << std::setfill('0') << std::setw(sizeof(T) * 2)
          << std::hex << ival;
   return stream.str();
 }


/// Get a levenshtein distance of strings
UTILS_EX int levenshtein_distance(std::string_view s1, std::string_view s2);

/**
 * Container Utils
 */

template<typename C, typename T>
std::optional<T> at(const C& container, const std::string& s, unsigned index = 0) {
    const auto range = container.equal_range(s);
    const auto it = std::next(range.first, index);
    return (std::distance(range.first, it) < std::distance(range.first, range.second)) ?
                std::make_optional(it->second) : std::nullopt;
}

template<typename C, typename T>
T at_or(const C& container, const std::string& s, const T& defaultValue, unsigned index = 0) {
    const auto v = at<C, T>(container, s, index);
    return v.has_value() ? *v : defaultValue;
}

template<typename C, typename T>
T atOr(const C& container, const std::string& s, const T& defaultValue, unsigned index = 0) {
    return at_or(container, s, defaultValue, index);
}


template <class Container = std::vector<std::string>>
Container split(const std::string& str, const char splitChar = ' ') {
    Container con;
    std::istringstream iss(str);
    for(std::string token; iss.good() && std::getline(iss, token, splitChar);) {
        con.insert(con.end(), convert<typename Container::value_type>(token));
    }
    return con;
}


template <typename IT>
inline constexpr std::string_view make_string_view(IT begin, IT end)
{
    return   (begin == end) ? std::string_view{nullptr} : std::string_view{&*begin, std::distance(begin, end)};
}


template <class T, typename K = typename T::key_type>
std::vector<K> keys(const T& map) {
    std::vector<K> ks; ks.resize(map.size());
    std::transform(map.begin(), map.end(), ks.begin(), [](const auto& p) {return p.first;});
    return ks;
}

template <class IT, typename In=typename IT::value_type, typename Out=typename IT::value_type>
std::string join(const IT& begin,
                 const IT& end,
                 const std::string joinChar = "",
                 const std::function<Out (const In&)>& f = [](const In& k)->Out{return k;}) {
    std::string s;
    std::ostringstream iss(s);
    if(begin != end) {
        for(auto it = begin;;) {
            iss << f(*it);
            if(!(++it != end)) break;
            if(!joinChar.empty())
                iss << joinChar;
        }
    }
    return iss.str();
}

template <class T, typename In=typename T::value_type, typename Out=typename T::value_type>
std::string join(const T& t,
                 const std::string joinChar = "",
                 const std::function<Out (const In&)>& f = [](const In& v)->Out{return v;}) {
    return join(t.begin(), t.end(), joinChar, f);
}

template <class IT, typename In=typename std::remove_pointer<IT>::type,
          typename Out=typename std::remove_pointer<IT>::type,
          typename = std::enable_if_t<std::is_pointer<IT>::value>>
std::string join(const IT begin,
                 const IT end,
                 const std::string joinChar = "",
                 const std::function<Out (const In&)>& f = [](const In& k)->Out{return k;}) {
    std::string s;
    std::ostringstream iss(s);
    if(begin != end) {
        for(auto it = begin;;) {
            iss << f(*it);
            if(!(++it != end)) break;
            if(!joinChar.empty())
                iss << joinChar;
        }
    }
    return iss.str();
}


template <class T>
T merge(const T& b1, const T& b2) {
       T bytes(b1.size() + b2.size());
       auto begin = bytes.begin();
       std::copy(b1.begin(), b1.end(), begin);
       std::advance(begin, std::distance(b1.begin(), b1.end()));
       std::copy(b2.begin(), b2.end(), begin);
       return bytes;
   }


template <class T, typename ...Arg>
T merge(const T& b1, const T& b2, Arg ...args) {
    T bytes(b1.size() + b2.size());
    auto begin = bytes.begin();
    std::copy(b1.begin(), b1.end(), begin);
    std::advance(begin, std::distance(b1.begin(), b1.end()));
    std::copy(b2.begin(), b2.end(), begin);
    return merge(bytes, args...);
}

/// Const version of std::advance
template <typename IT> IT advanced(IT it, int distance) {
    std::advance(it, distance);
    return it;
}

template <typename K, typename V >
/// Get a value from a multimap, especially helper for Parameter Options
std::optional<V> get_value(const std::multimap<K, V>& map, const K& key, int index = 0)
{
     const auto range = map.equal_range(key);
     return std::distance(range.first, range.second) > index ?
                 std::make_optional((advanced(range.first, index))->second) : std::nullopt;
}


template <typename K, typename V >
std::optional<V> getValue(const std::multimap<K, V>& map, const K& key, int index = 0) {
    return get_value(map, key, index);
}

 /*
 *  Misc Utils
 */

class expiror;
[[nodiscard]]
UTILS_EX std::shared_ptr<expiror> wait_expire(std::chrono::seconds s, const std::function<void ()>& onExpire);
class expiror {
public:
    ~expiror() {m_f.wait();}
private:
    expiror() = default;
    expiror(std::future<void>&& f) : m_f(std::move(f)) {}
    std::future<void> m_f;
    friend std::shared_ptr<GempyreUtils::expiror> GempyreUtils::wait_expire(std::chrono::seconds s, const std::function<void ()>& onExpire);
};

UTILS_EX  std::string hexify(const std::string& src, const std::string pat);
UTILS_EX  std::string unhexify(const std::string& src);

enum class OS {OtherOs, MacOs, WinOs, LinuxOs, AndroidOs, RaspberryOs};

UTILS_EX OS current_os();


UTILS_EX std::string html_file_launch_cmd();


enum AddressType{Ipv4 = 0x1, Ipv6 = 0x2};
UTILS_EX std::vector<std::string> ip_addresses(int addressType);


/**
  * File Utils
  */

#if 0
UTILS_EX long timeStamp(const std::string& filename);
UTILS_EX std::string appPath();
#endif

UTILS_EX std::string get_link(const std::string& fname);
UTILS_EX bool is_dir(const std::string& fname);
UTILS_EX std::string working_dir();
UTILS_EX std::string home_dir();
UTILS_EX std::string root_dir();
UTILS_EX std::string abs_path(const std::string& rpath);
UTILS_EX std::string path_pop(const std::string& filename, int steps = 1);
UTILS_EX std::vector<std::string> directory(const std::string& dirname);
UTILS_EX std::optional<std::string> read_process(const std::string& processName, const std::vector<std::string>& params);
UTILS_EX std::string base_name(const std::string& filename);
UTILS_EX std::tuple<std::string, std::string> split_name(const std::string& filename);
/// Generate unique name (prefer <filesystem> if available)
UTILS_EX std::string temp_name();
UTILS_EX std::string host_name();
UTILS_EX std::optional<std::string> system_env(const std::string& env);
UTILS_EX bool is_hidden_entry(const std::string& filename);
UTILS_EX bool is_executable(const std::string& filename);
UTILS_EX SSIZE_T file_size(const std::string& filename);
UTILS_EX bool rename(const std::string& of, const std::string& nf);
UTILS_EX void remove_file(const std::string& filename);
UTILS_EX bool file_exists(const std::string& filename);
UTILS_EX std::optional<std::string> which(const std::string& filename);
/// push name to path
UTILS_EX std::string push_path(const std::string& path, const std::string& name);
template<class ...NAME>
std::string push_path(const std::string& path, const std::string& name, NAME...names) {
    return push_path(push_path(path, name), names...);
}
///execute a prog
UTILS_EX int execute(const std::string& prog, const std::string& parameters);

/// exit 
UTILS_EX void process_exit(int err);


template <class T>
std::string write_to_temp(const T& data) {
    const auto name = GempyreUtils::temp_name();
    std::ofstream out(name, std::ios::out | std::ios::binary);
    std::ostreambuf_iterator<typename T::value_type> iter(out);
    std::copy(data.begin(), data.end(), iter);
    return name;
}


template <class T>
std::vector<T> slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max()) {
   std::vector<T> vec;
   std::ifstream stream(file, std::ios::in | std::ios::binary | std::ios::ate);
   if(!stream.is_open()) {
       log(LogLevel::Error, "Cannot open file", qq(file));
       return vec;
   }
   const auto size = std::min(max, static_cast<size_t>(stream.tellg()));
   if(size <= 0) {
       return vec;
   }
   vec.resize(size / sizeof (T), 0);
   stream.seekg(std::ios_base::beg);
   auto ptr = reinterpret_cast<char*>(vec.data());
   stream.read(ptr, static_cast<std::streamsize>(size));
   return vec;
 }

UTILS_EX std::string slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max());

UTILS_EX std::optional<std::string> to_json_string(const std::any& any);
UTILS_EX std::optional<std::any> json_to_any(const std::string& str);

UTILS_EX bool is_available(int port);

UTILS_EX std::string base64_encode(const unsigned char* bytes, size_t sz);
UTILS_EX std::vector<unsigned char> base64_decode(const std::string_view& data);

}



#endif // UTILS_H
