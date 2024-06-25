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
#include <string_view>
#include <iostream>

/**
  * @file
  * 
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * gempyre_utils.h contains a collection utility functions used internally within Gempyre
  * implementation and test applications. The are in API as they may be useful for any 
  * developer. Gempyre-Python do not wrap these as Python asset have similar functions
  * in internal or pip packages.
  */


/// @cond INTERNAL
#define GEMPYREUTILSDEBUG(x) GempyreUtils::log(Utils::LogLevel::Debug, x, __FILE__, __LINE__)
/// @endcond
/// Release build assert
#define gempyre_utils_assert(b) (b || GempyreUtils::do_fatal("Panic!", nullptr, __FILE__, __LINE__))
/// Release build assert with a message
#define gempyre_utils_assert_x(b, x) (b || GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__))
/// Release build assert with a message and a function executed before forced exit
#define gempyre_utils_assert_x_f(b, x, f) (b || GempyreUtils::do_fatal(x, f, __FILE__, __LINE__))
/// Forced exit with a message
#define gempyre_utils_fatal(x) GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__)
/// Forced exit with a message and a function executed before forced exit
#define gempyre_utils_fatal_f(x, f) GempyreUtils::do_fatal(x, f, __FILE__, __LINE__)
/// RAII helper for pointer
#define gempyre_utils_auto_clean(p, f) std::unique_ptr<std::remove_pointer<decltype(p)>::type, decltype(&f)> _ ## p (p, &f)
/// RAII helper for non-pointer
#define gempyre_utils_auto_close(p, f) GempyreUtils::_Close<std::decay_t<decltype(p)>, decltype(&f)> _ ## p (p, &f)

/// @cond INTERNAL

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


/// @endcond

namespace GempyreUtils {
/**
 * @brief The LogLevel enum
 */
enum class LogLevel : int {
    None,       /// All logs disabled
    Fatal,      /// Execution ends here
    Error,      /// Something is wrong, Default
    Warning,    /// At least developer should be worried
    Info,       /// Something developer should know
    Debug,      /// What is going on
    Debug_Trace /// What is going on, and telling it floods output and impacts performance
    };

/// @brief Parent class for LogWriters
class LogWriter {
public:
    /// @brief Constructor
    LogWriter();
    /// @brief Destructor
    virtual ~LogWriter(); 
    LogWriter(const GempyreUtils::LogWriter&) = delete;
    LogWriter& operator=(const GempyreUtils::LogWriter&) = delete; 
    /// @brief header of class, called before every line, default just returns a timestamp and loglevel string.
    /// @param logLevel 
    /// @return 
    virtual std::string header(LogLevel logLevel);

    /// @brief Implement write to the medium.
    /// @param buffer Buffer's address
    /// @param count  it's size
    /// @return 
    virtual bool do_write(const char* buffer, size_t count) = 0;
    /// @brief  override to return true if this write supports ANSI colors, default just return false
    virtual bool has_ansi() const;
private:
    LogWriter* m_previousLogWriter{nullptr};
};

/// @cond INTERNAL
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
/// @endcond

/// @brief Set current log level.
/// @param level  
UTILS_EX void set_log_level(LogLevel level);

/// @brief  Get current log level.
/// @return current log level.
UTILS_EX LogLevel log_level(); 

/// @brief Log Level to string
/// @param log_level that is converted to string.
/// @return std::string
UTILS_EX std::string to_str(LogLevel log_level);

/// @cond INTERNAL
UTILS_EX std::ostream log_stream(LogLevel logLevel);

template <typename T, typename ...Args>
inline void log_line(LogLevel level, std::ostream& os, const T& e, Args... args) {
    os << e << " ";
    log_line(level, os, args...);
}


UTILS_EX void process_exit(int);

template<typename T>
inline void log_line(LogLevel level, std::ostream& os, const T& e) {
    os << e << std::endl;
    if(level == LogLevel::Fatal)  {
        process_exit(99);
    }
}
/// @endcond

/// @brief Write a log line.
/// @param level log level of this message.
/// @param e param to print. 
/// @param args optional more parameters to print.
template <typename T, typename ...Args>
inline void log(LogLevel level, const T& e, Args... args) {
    if(level <= log_level()) {
        auto os = log_stream(level);
        log_line(level, os, e, args...);
    }
}

/// @cond INTERNAL
template <LogLevel level, typename T, typename ...Args>
inline void write_log(const T& e, Args... args) {
    log(level, e, args...);
}
/// @endcond

/// @brief Write a debug log
template <typename T, typename ...Args>
inline void log_debug(const T& e, Args... args) {
    write_log<LogLevel::Debug, T, Args...>(e, args...);
}


/// @cond INTERNAL
inline bool do_fatal(const std::string_view txt, std::function<void()> f, const char* file, int line) {
    if(f) f();
    log(LogLevel::Fatal, txt, "at", file, "line:", line);
    return false;
}
/// @endcond


#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/// @cond INTERNAL
#define GEM_DEBUG(...) GempyreUtils::log(GempyreUtils::LogLevel::Debug, __PRETTY_FUNCTION__, __VA_ARGS__)
/// @endcond
}




/**
 * @namespace GempyreUtils
 */
namespace GempyreUtils {

/// @cond INTERNAL
//Helper for C memory management
template <typename T, typename A>
class _Close {
public:
    _Close(T ref, A&& f) : t(ref), d(f) {}
    ~_Close() {(void) this->d(this->t);}
    _Close(_Close&& other) = default;
    _Close& operator=(_Close&& other) = default;
    _Close(const _Close& other) = delete;
    _Close& operator=(const _Close& other) = delete;
private:
    T t;
    A d;
};
/// @endcond

/// @brief Option Argument type for parse_args
enum class ArgType{
    NO_ARG, /// Option does not have an argument
    REQ_ARG,    /// Option has an argument 
    OPT_ARG /// Option may have an argument
    };

/// string vector of parameters. Used for parse_params.    
using ParamList = std::vector<std::string>;

/// string - string value pairs of options. Used for parse_params.
using Options = std::multimap<std::string, std::string>;

/// a tuple containing ParamList and Options. Used for parse_params.
using Params = std::tuple<ParamList, Options>;

/// parse arguments
/// @param argc argc from main
/// @param argv argv from main
/// @param args Optional arguments, each tuple of 'long name', 'short name' and type
/// @return tuple of ParamList and Options, where ParamList is vector of positional parameters and Options is map of options and their possible values.
UTILS_EX Params parse_args(int argc, char* argv[], const std::initializer_list<std::tuple<std::string, char, ArgType>>& args);

/// @cond INTERNAL
UTILS_EX void init();
UTILS_EX std::string current_time_string();
UTILS_EX std::string last_error();
/// @endcond

/// @cond INTERNAL
template<typename E>
struct ErrorValue {
    ErrorValue(E&& ee) : e{ee} {}
    ErrorValue(const E& ee) : e{ee} {}
    E e;
    };
/// @endcond

/// @brief Result is used like optional, but contains a fail reasoning
/// @tparam T 
/// @tparam E 
template<typename T, typename E = std::string>
class Result {
    public:
        /// @brief 
        /// @param t 
        Result(const T& t) : val(t) {}
        /// @brief 
        /// @param t 
        Result(T&& t) : val(std::move(t)) {}
        /// @brief 
        /// @param e 
        Result(ErrorValue<E>&& e) : val(std::move(e)) {}
        /// @brief 
        /// @param t 
        /// @return 
        Result& operator= (const T& t) {val = t; return *this;}
        /// @brief 
        /// @param t 
        /// @return 
        Result& operator= (T&& t) {val = std::move(t); return *this;}
        /// @brief 
        /// @param e 
        /// @return 
        static Result<T, E> make_error(const E& e) { return Result{ErrorValue{e}};}
        /// @brief 
        /// @param e 
        /// @return 
        static Result<T, E> make_error(E&& e) { return Result{ErrorValue{e}};}
        /// @brief 
        /// @return 
        bool has_value() const noexcept {return std::get_if<T>(&val);}
        /// @brief 
        /// @return 
        const T& value() const noexcept {return std::get<T>(val);}
        /// @brief 
        /// @return 
        T& value() noexcept {return std::get<T>(val);}
        /// @brief 
        /// @return 
        const E& error() const noexcept {return std::get<ErrorValue<E>>(val);}
        /// @brief 
        /// @return 
        const T& operator->() const noexcept {return value();}
        /// @brief 
        /// @return 
        T& operator->() noexcept {return value();}
        /// @brief 
        operator bool() const noexcept {return has_value();} 
        /// @brief 
        /// @param t 
        void swap(T& t) noexcept {val.swap(t);}                  
    private:
    std::variant<T, ErrorValue<E>> val;
};

/// @brief 
/// @tparam T 
/// @tparam ...A 
/// @param ...a 
/// @return 
template<typename T, typename... A>
Result<T, std::string> make_error(A&&... a) {
    std::stringstream str;
    (std::cout << ... << a); //str << ... << a;
    return Result<T, std::string>::make_error(str.str());
}

/// @brief 
/// @tparam T 
/// @param result 
/// @return 
template<typename T>
Result<T, std::string> make_result(T&& result) {
    return Result<T, std::string>(std::move(result));
}


/**
  * ## String Utils
  */

/// @brief make quoted
/// @param s string
/// @return  quoted string
UTILS_EX std::string qq(const std::string& s);

/// @brief remove newline from end of string
/// @param s string 
/// @return string
UTILS_EX std::string chop(const std::string& s);

/// @brief remove a string from end 
/// @param s 
/// @param chopped 
/// @return string
UTILS_EX std::string chop(const std::string& s, const std::string& chopped);

/// @brief replace a substrings from a string
/// @param str original string
/// @param substring  regular expression
/// @param substitution  replacement
/// @return string
UTILS_EX std::string substitute(const std::string& str, const std::string& substring,  const std::string& substitution);

/// @brief remove 
/// @param str spaces from a string
/// @return string
UTILS_EX std::string remove_spaces(const std::string& str);

/// @cond INTERNAL
template <typename T>
T convert(const std::string& source) {
    std::istringstream ss(source);
    typename std::remove_const<T>::type v{};
    ss.operator>>(v); //overloads have similar conversions MSVC19
    return v;
}

/// @brief  get p right most chars
/// @param str 
/// @param p 
/// @return 
inline
std::string_view right(std::string_view str, size_t p) {
    return str.substr(str.length() - p);
}

template <>
inline std::string convert<std::string>(const std::string& source)
{
    return source;
}
/// @endcond

/// @brief parse string to value
/// @tparam T 
/// @param source 
/// @return value
template <typename T>
std::optional<T> parse(const std::string& source) {
    std::istringstream ss(source);
    T v;
    static_cast<std::istream&>(ss) >> v;   //MSVC said it would be otherwise ambiguous
    return !ss.fail() ? std::make_optional(v) : std::nullopt;
}

/// @brief make lower case string
/// @param str 
/// @return string
template <class T>
std::string to_low(const T& str) {
    std::string n;
    std::transform(str.begin(), str.end(), std::back_inserter(n),
                                    [](auto c){return std::tolower(c);});
    return n;
}

 
/// @brief trim from left
/// @param str 
/// @return trimmed string view
inline std::string_view ltrim(std::string_view str) {
    const auto begin = std::find_if(str.begin(), str.end(), [](auto ch) {
        return (ch > ' ');
    });
   return str.substr(static_cast<std::string_view::size_type>(
    std::distance(str.begin(), begin)));
}

/// @brief trim from right
/// @param str 
/// @return trimmed string view
inline std::string_view rtrim(std::string_view str) {
    const auto end = std::find_if(str.rbegin(), str.rend(), [](auto ch) {
        return (ch > ' ');
    });
    return str.substr(0, static_cast<std::string_view::size_type>(
        std::distance(end, str.rend())));
}

/// @brief trim from left and right
/// @param str 
/// @return trimmed string view
inline std::string_view trim(std::string_view str) {
    return ltrim(rtrim(str));
}



 /// @brief Hex presentation of the value
 /// @param ival 
 /// @return string
 template< typename T >
 std::string to_hex(T ival) {
   std::stringstream stream;
   stream << std::setfill('0') << std::setw(sizeof(T) * 2)
          << std::hex << ival;
   return stream.str();
 }


/// @brief Get a levenshtein distance of strings
/// @param s1 
/// @param s2 
/// @return their distance
UTILS_EX int levenshtein_distance(std::string_view s1, std::string_view s2);

/**
 * ## Container Utils
 */

/// @cond INTERNAL
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
/// @endcond

/// @brief Split sting to container
/// @tparam Container defaults to std::vector<std::string>
/// @param str 
/// @param splitChar 
/// @return container
template <class Container = std::vector<std::string>>
Container split(const std::string& str, const char splitChar = ' ') {
    Container con;
    std::istringstream iss(str);
    for(std::string token; iss.good() && std::getline(iss, token, splitChar);) {
        con.insert(con.end(), convert<typename Container::value_type>(token));
    }
    return con;
}

/// @cond INTERNAL
template <typename IT>
inline constexpr std::string_view make_string_view(IT begin, IT end)
{
    return   (begin == end) ? std::string_view{nullptr} : std::string_view{&*begin, std::distance(begin, end)};
}
/// @endcond

/// @brief Get keys from map
/// @tparam T map type
/// @tparam K key type, defaults to key type
/// @param map 
/// @return vector
template <class T, typename K = typename T::key_type>
std::vector<K> keys(const T& map) {
    std::vector<K> ks; ks.resize(map.size());
    std::transform(map.begin(), map.end(), ks.begin(), [](const auto& p) {return p.first;});
    return ks;
}

/// @brief Join container values, try 1st if compiler can deduct types
/// @tparam IT container
/// @tparam In default container value type
/// @tparam Out default to container type
/// @param begin begin iterator
/// @param end  end iterator
/// @param joinChar optional glue string
/// @param f optional transform function
/// @return string
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

/// @brief Join container values, try 1st if compiler can deduct types
/// @tparam IT container
/// @tparam In default container value type
/// @tparam Out default to container type
/// @param t container
/// @param joinChar optional glue string
/// @param f optional transform function
/// @return string
template <class T, typename In=typename T::value_type, typename Out=typename T::value_type>
std::string join(const T& t,
                 const std::string joinChar = "",
                 const std::function<Out (const In&)>& f = [](const In& v)->Out{return v;}) {
    return join(t.begin(), t.end(), joinChar, f);
}

/// @brief Join container values, try 1st if compiler can deduct types
/// @tparam IT container
/// @tparam In default container value type
/// @tparam Out default to container type
/// @param begin begin iterator
/// @param end  end iterator
/// @param joinChar optional glue string
/// @param f optional transform function
/// @return string 
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

/// @cond INTERNAL
template <class T>
T merge(const T& b1, const T& b2) {
       T bytes(b1.size() + b2.size());
       auto begin = bytes.begin();
       std::copy(b1.begin(), b1.end(), begin);
       std::advance(begin, std::distance(b1.begin(), b1.end()));
       std::copy(b2.begin(), b2.end(), begin);
       return bytes;
   }

// I just wonder why copy instead of move hence not public
template <class T, typename ...Arg>
T merge(const T& b1, const T& b2, Arg ...args) {
    T bytes(b1.size() + b2.size());
    auto begin = bytes.begin();
    std::copy(b1.begin(), b1.end(), begin);
    std::advance(begin, std::distance(b1.begin(), b1.end()));
    std::copy(b2.begin(), b2.end(), begin);
    return merge(bytes, args...);
}
/// @endcond

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


 /*
 *  Misc Utils
 */

/// @cond INTERNAL

class expiror;
[[nodiscard]]
UTILS_EX std::shared_ptr<expiror> wait_expire(std::chrono::seconds s, const std::function<void ()>& onExpire);
class expiror {
public:
    ~expiror() {m_f.wait();}
private:
    expiror() = default;
    expiror(std::future<void>&& f) : m_f(std::move(f)) {}
    std::future<void> m_f{};
    friend std::shared_ptr<GempyreUtils::expiror> GempyreUtils::wait_expire(std::chrono::seconds s, const std::function<void ()>& onExpire);
};

/// @endcond

/// @brief URL hexify string
/// @param src source
/// @param pat regex pattern that are separated in hexified sequences
/// @return  string
UTILS_EX  std::string hexify(const std::string& src, const std::string pat);

/// @brief un hexify hexified string
/// @param src 
/// @return string
UTILS_EX  std::string unhexify(const std::string& src);

/// @brief OS id
enum class OS {OtherOs, MacOs, WinOs, LinuxOs, AndroidOs, RaspberryOs};

/// Get
UTILS_EX OS current_os();

/// @cond INTERNAL
UTILS_EX std::string html_file_launch_cmd();
/// @endcond


/// @brief address type
enum  AddressType : unsigned {Ipv4 = 0x1, Ipv6 = 0x2};
/// @brief  Try to resolve own ipaddresses
/// @param addressType 
/// @return list of addresses
UTILS_EX std::vector<std::string> ip_addresses(unsigned addressType);


/**
  * ## File Utils
  */


/// Get symbolic link source 
UTILS_EX std::string get_link(const std::string& fname);
/// Is dir
UTILS_EX bool is_dir(const std::string& fname);
/// Current source dir
UTILS_EX std::string working_dir();
/// Current source dir
UTILS_EX std::string home_dir();
/// Current root dir - 'C:\' or '/'
UTILS_EX std::string root_dir();
/// Absolute path
UTILS_EX std::string abs_path(const std::string& rpath);
/// @brief Remove elements from path
/// @param filename 
/// @param steps - number of elements removed, default 1
/// @return - remaining path
UTILS_EX std::string path_pop(const std::string& filename, int steps = 1);
/// Directory entries
UTILS_EX std::vector<std::string> entries(const std::string& dirname);

/// @cond INTERNAL
[[deprecated("use entries")]]
inline std::vector<std::string> directory(const std::string& dirname) {return entries(dirname);}
/// @endcond

/// @brief Read stdout from the process - wait process to end
/// @param processName 
/// @param params process parameters
/// @return optional parameters
UTILS_EX std::optional<std::string> read_process(const std::string& processName, const std::vector<std::string>& params);
/// Base name
UTILS_EX std::string base_name(const std::string& filename); 
/// Name and extension
UTILS_EX std::tuple<std::string, std::string> split_name(const std::string& filename);
/// Generate unique name (prefer std::filesystem if available)
UTILS_EX std::string temp_name();
/// Machine host name
UTILS_EX std::string host_name();
/// Read environment value
UTILS_EX std::optional<std::string> system_env(const std::string& env);
/// Is entry hidden
UTILS_EX bool is_hidden_entry(const std::string& filename);
/// is executable
UTILS_EX bool is_executable(const std::string& filename);
/// File size
UTILS_EX SSIZE_T file_size(const std::string& filename);
/// Rename a file
UTILS_EX bool rename(const std::string& of, const std::string& nf);
/// Delete file
UTILS_EX void remove_file(const std::string& filename);
/// Test if file with name exits
UTILS_EX bool file_exists(const std::string& filename);
/// Try to find a executable from PATH
UTILS_EX std::optional<std::string> which(const std::string& filename);
/// push name to path
UTILS_EX std::string push_path(const std::string& path, const std::string& name);
/// Construct a path from parameters
template<class ...NAME>
std::string push_path(const std::string& path, const std::string& name, NAME...names) {
    return push_path(push_path(path, name), names...);
}
/// Execute a program
UTILS_EX int execute(const std::string& prog, const std::string& parameters);


/// @brief Write data to temp file
/// @tparam T 
/// @param data 
/// @return filename where data was written to.
template <class T>
std::string write_to_temp(const T& data) {
    const auto name = GempyreUtils::temp_name();
    std::ofstream out(name, std::ios::out | std::ios::binary);
    std::ostreambuf_iterator<typename T::value_type> iter(out);
    std::copy(data.begin(), data.end(), iter);
    return name;
}

/// @brief Read a file data in one read.
/// @tparam T type of value packed in the returned vector.
/// @param file to read.
/// @param max maximum amount of data to read.
/// @return Vector of data read
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

/// @brief Read a file in one read 
/// @return string containing file
UTILS_EX std::string slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max());

/// @brief Convert any type to json string, if possible.
/// @param any - assumed to be a type convertible to json: int, string, boolean, null, double, or std::vector, std::unordered_map or std::map containing other values.
/// @return json string.
UTILS_EX Result<std::string> to_json_string(const std::any& any);
/// @brief Concert json string to any type
/// @param str 
/// @return 
UTILS_EX Result<std::any> json_to_any(const std::string& str);

/// Check if port is free.
UTILS_EX bool is_available(int port);

/// Base64 encode.
UTILS_EX std::string base64_encode(const unsigned char* bytes, size_t sz);
/// Base64 encode 
UTILS_EX std::string base64_encode(const std::vector<uint8_t> & vec);
/// Base64 decode.
UTILS_EX std::vector<uint8_t> base64_decode(const std::string_view& data);

}



#endif // UTILS_H
