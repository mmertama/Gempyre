#ifndef WINDOWS_OS

#include <getopt.h>
#include <unistd.h>
#include <syslog.h>
#include <dirent.h>
#include <libgen.h>

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#else
#ifndef _WINSOCKAPI_                              
    #include <winsock2.h>
#endif   
#include <Windows.h>                                
#include <iphlpapi.h>
#include <Ws2tcpip.h>
#include <shlwapi.h>

#endif

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <cstring>
#include <iostream>
#include <regex>
#include <ios>
#include <iomanip>
#include <thread>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdio>
#include <variant>
#include <cassert>

#if 0 //def MAC_OS
#include <mach-o/dyld.h>
#endif


#include "gempyre_utils.h"
#include "base64.h"

//without <filesystem> support
#include <cstdlib>

namespace GempyreUtils {
template<typename T>
static std::string last_error(T err) {
#ifdef WINDOWS_OS
    const DWORD size = 256;
    char buffer[size];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
       nullptr, err, 0,buffer, size, NULL );
    return std::string(buffer);
#else
    return std::string(::strerror(err));
#endif
}
}

using namespace std::chrono_literals;


void GempyreUtils::init() {
#ifdef WINDOWS_OS
	if(!LoadLibraryA("ntdll.dll")) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Cannot preload", "ntdll.dll");
	}
	if(!LoadLibraryA("kernel32.dll")) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Cannot preload", "kernel32.dll");
	}
	if(!LoadLibraryA("advapi32.dll")) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Cannot preload", "advapi32.dll");
	}
    WSADATA wsa;
    const auto version = MAKEWORD(2, 2);
    const int code = WSAStartup(version, &wsa);
    if(0 != code) {
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Cannot initialize socket, Windows WSAStartup failed: code", code, "Last error:",  last_error());
    }
#endif
}


std::string GempyreUtils::qq(const std::string& s) {
   std::stringstream ss;
   ss << std::quoted(s);
   return ss.str();
}

std::string GempyreUtils::chop(const std::string& s) {
    auto str = s;
    str.erase(str.find_last_not_of('\0') + 1);
    str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
    return str;
}

std::string GempyreUtils::chop(const std::string& s, const std::string& chopped) {
    auto str = s;
    str.erase(str.find_last_not_of(chopped) + 1);
    return str;
}


GempyreUtils::Params GempyreUtils::parse_args(int argc, char* argv[], const std::initializer_list<std::tuple<std::string, char, ArgType>>& args) {
#ifndef WINDOWS_OS
    /*
     * The variable optind is the index of the next element to be processed in argv.
     * The system initializes this value to 1. The caller can reset it to 1 to restart scanning of the same argv,
     *  or when scanning a new argument vector.
    */
    optind = 1;
    optarg = nullptr;
    opterr = 0;
    auto longOptionsPtr = std::make_unique<::option[]>(args.size() + 1);
    ::option* longOptions = longOptionsPtr.get();
    int argi = 0;
    std::string plist = "";
    for(const auto& t: args) {
        longOptions[argi].name = std::get<0>(t).c_str();
        const auto shortChar = std::get<1>(t);
        plist += shortChar;
        longOptions[argi].val = shortChar;
        longOptions[argi].flag = nullptr;
        const auto atype = std::get<2>(t);
        switch (atype) {
        case ArgType::NO_ARG:
            longOptions[argi].has_arg = no_argument;
            break;
        case ArgType::REQ_ARG:
            longOptions[argi].has_arg = required_argument;
            plist += ':';
            break;
        case ArgType::OPT_ARG:
            longOptions[argi].has_arg = optional_argument;
            plist += "::";
            break;
        default:
            log(LogLevel::Fatal, "Bad argument", static_cast<int>(atype));
        }
        ++argi;
    }
    longOptions[args.size()] = {nullptr, 0, nullptr, 0};

    std::multimap<std::string, std::string> options;

    for(;;) {
        const auto opt = getopt_long(argc, argv, plist.c_str(),
                            longOptions, nullptr);
        if(opt < 0)
            break;

        if(opt == '?') {
            log(LogLevel::Warning, "Unknown argument");
            continue; // this is unknow argument, we just ignore it
        }

        if(opt == ':') {
            log(LogLevel::Error, "Argument value is missing");
            continue;
        }

        const auto index = std::find_if(longOptions, longOptions + args.size(), [opt](const ::option& o){return o.val == opt;});

        const std::string opt_value{optarg ? trimmed(optarg) : std::string{}};
        options.emplace(longOptions[std::distance(longOptions, index)].name, opt_value);
    }

    std::vector<std::string> params;
    while(optind < argc) {
        params.push_back(argv[optind++]);
    }
#else

    std::vector<std::string> plist;
    for(auto ii = 1; ii < argc; ++ii) {
        plist.push_back(argv[ii]);
    }

    Options options;
    ParamList params;
    for(auto i = 0U; i < plist.size(); ++i) {
        const auto arg = plist[i];
        if(arg[0] == '-') {
            if(arg.length() < 2) {
                log(LogLevel::Error, "Invalid argument");
                continue;
            }
            decltype(args.end()) it;
            bool longOpt = false;
            auto assing = arg.end();
            if(arg[1] == '-') {
                if(arg.length() < 3) {
                    log(LogLevel::Error, "Invalid argument");
                    continue;
            }
            longOpt = true;
            assing = std::find(arg.begin(), arg.end(), '=');
            const auto key = assing == arg.end() ?
                        arg.substr(2) : arg.substr(2, std::distance(arg.begin(), assing) - 2); // AA=BB -> get AA
            it = std::find_if(args.begin(), args.end(), [&key](const auto& a) {
                return std::get<0>(a) == key;}
            );
        } else {
            const auto key = arg.substr(1, 1);
            it = std::find_if(args.begin(), args.end(), [&key](const auto& a){return std::get<1>(a) == key[0];});
        }
        if(it != args.end()) {
            switch(std::get<ArgType>(*it)) {
            case ArgType::NO_ARG:
                options.emplace(std::get<std::string>(*it), "");
                break;
            case ArgType::REQ_ARG: {
                std::string val;
                if(!longOpt && !arg.substr(2).empty()) {
                    val = arg.substr(2);
                } else if(assing != arg.end()) {
                    val = arg.substr(static_cast<unsigned>(std::distance(arg.begin(), assing) + 1));
                } else if(i + 1 < plist.size()) {
                    val = plist[i + 1];
                    ++i;
                } else  {
                    log(LogLevel::Error, "Invalid argument");
                    continue;
                    }
                options.emplace(std::get<std::string>(*it), val);
                } break;
            case ArgType::OPT_ARG: {
                std::string val;
                if(!longOpt && !arg.substr(2).empty()) {
                    val = arg.substr(2);
                } else if(assing != arg.end()) {
                    val = arg.substr(static_cast<unsigned>(std::distance(arg.begin(), assing) + 1));
                }
                else {
                    val = "";
                }
                 options.emplace(std::get<std::string>(*it), val);
                }
                break;
            }
        }
    } else {
        params.push_back(arg);
    }

    }
#endif
    return std::make_tuple(params, options);
}

std::string GempyreUtils::abs_path(const std::string& rpath) {
#ifndef WINDOWS_OS
    char* pathPtr = ::realpath(rpath.c_str(), nullptr);
    if(!pathPtr)
        return std::string();
    std::string path(pathPtr);
    free(pathPtr);
    return path;
#else
    constexpr auto bufSz = 4096; //this could be done properly...
    TCHAR  buffer[bufSz]=TEXT("");
    return 0 != GetFullPathName(rpath.c_str(),
                     bufSz,
                     buffer,
                     nullptr) ? std::string(buffer) : "";
#endif
}

std::tuple<std::string, std::string> GempyreUtils::split_name(const std::string& filename) {
    const auto name = base_name(filename);
    const auto index = name.find_last_of('.');
    return std::make_tuple(name.substr(0, index), name.substr(index + 1));
}


std::string GempyreUtils::base_name(const std::string& filename) {
    const auto dname = path_pop(filename);
    return dname.empty() ? filename :
        filename.substr(dname.length() + 1);
}

std::string GempyreUtils::path_pop(const std::string& filename, int steps) {
    if (steps <= 0)
        return filename;
    else {
         const auto p = filename.find_last_of(
    #ifndef WINDOWS_OS
       '/');
    #else
       '\\');
    #endif
        return path_pop(p != std::string::npos ? filename.substr(0, p) : "", steps - 1);
    }
}

std::optional<std::string> GempyreUtils::read_process(const std::string& processName, const std::vector<std::string>& params) {
    const auto param_line = join(params, " ");
    auto fd =
 #ifndef WINDOWS_OS
            ::popen
 #else
            ::_popen
 #endif
            ((processName + " " + param_line).c_str(), "r");

    if(!fd)
        return std::nullopt;
#ifndef WINDOWS_OS
    gempyre_utils_auto_close(fd,::pclose);
#else
   gempyre_utils_auto_close(fd,::_pclose);
#endif
    std::string out;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fd) != nullptr) {
           out += buffer;
    }
    return out;
}



std::string GempyreUtils::temp_name() {
#ifdef WINDOWS_OS
    TCHAR path_buf[MAX_PATH];
    const auto len = GetTempPathA(MAX_PATH, path_buf);
    path_buf[len] = '\0';
    TCHAR name_buf[MAX_PATH];
    const auto err = GetTempFileNameA(path_buf, TEXT("ecu"), 0, name_buf);
    assert(err); (void) err;
    const std::string name{name_buf};  
#else
#ifdef MAC_OS
    const auto tmp = std::getenv("TMPDIR");
    assert(tmp); // should alway be there;
    assert(tmp[std::strlen(tmp) - 1] == '/');
    constexpr char ext[] = "ecutils_XXXXXX";
    char name[1024] = {0};
    std::strcat(name, tmp);
    std::strcat(name, ext);
#else
    constexpr char tpl[] = "/tmp/ecutils_XXXXXX";
    char name[sizeof tpl];
    std::strcpy(name, tpl);
#endif
    const auto fd = ::mkstemp(name);
    ::close(fd);
#endif
    return name;
}

void GempyreUtils::remove_file(const std::string& name) {
    ::remove(name.c_str());
}


std::string GempyreUtils::substitute(const std::string& str, const std::string& substring, const std::string& subsitution) {
    try {
    return std::regex_replace(str, std::regex(substring) , subsitution);
    } catch (std::regex_error& e) {
         log(LogLevel::Error, "reg exp", substring, static_cast<unsigned>(e.code()));
         gempyre_utils_assert_x(false, "regexp");
    }
    return std::string();
}

#if 0
std::string GempyreUtils::appPath() {
#ifdef WINDOWS_OS
    char buffer[MAX_PATH];
    const auto ok =  GetModuleFileNameA(
        nullptr,
        buffer,
        MAX_PATH);
    return ok ? std::string{buffer} : std::string{};
}
#elif MAC_OS
    char buffer[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    const auto err = NSGetExecutablePath(buffer, &bufsize);
    return !err ? std::string{buffer};
#elif UNIX_OS
    return getLink("/proc/self/exe");
#endif
}
#endif



bool GempyreUtils::file_exists(const std::string& filename) {
#ifndef WINDOWS_OS
    return ::access(filename.c_str(), F_OK) != -1;
#else
    return (0xFFFFFFFF != GetFileAttributes(filename.c_str()));
#endif
}


std::vector<std::string> GempyreUtils::directory(const std::string& dirname) {
    std::vector<std::string> entries;
    if(dirname.empty())
        return entries;
    const auto dname = dirname.back() != '/' ? dirname + '/' : dirname;
#ifndef WINDOWS_OS
    auto dir = ::opendir(dname.c_str());
    if(!dir)
        return entries;
    while(auto dirEntry = ::readdir(dir)) {
        if(std::strcmp(dirEntry->d_name, ".") != 0 && std::strcmp(dirEntry->d_name, "..") != 0)
            entries.push_back({dirEntry->d_name});
    }
    ::closedir(dir);
#else
        const auto searchPath = dirname + "/*.*";
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
        if(hFind != INVALID_HANDLE_VALUE) {
            do {
                entries.push_back({fd.cFileName});
            } while(::FindNextFile(hFind, &fd));
            ::FindClose(hFind);
        }
#endif
        return entries;
}    


#ifndef WINDOWS_OS
bool GempyreUtils::is_dir(const std::string& path) {
   struct stat statbuf;
   if (::stat(path.c_str(), &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
#else
bool GempyreUtils::is_dir(const std::string& path) {
    return PathIsDirectory(path.c_str());
}
#endif


std::string GempyreUtils::get_link(const std::string& lname) {
 #ifndef WINDOWS_OS
    size_t sz = 128;
    for(;;) {
        std::vector<char> buffer(sz);
        const auto nchars = ::readlink (lname.c_str(), buffer.data(), sz);
        if(nchars <= 0)
            return std::string();
        if(static_cast<size_t>(nchars) < sz) {
            return std::string(buffer.data(), static_cast<size_t>(nchars));
        } else sz *= 2;
    }
#else
    return lname;
#endif
}

std::optional<std::string> GempyreUtils::system_env(const std::string& env) {
    if(env.length() > 0) {
        auto str = std::getenv(env.c_str());
        if(str) {
            return str;
        }
    }
    return std::nullopt;
}

std::string GempyreUtils::host_name() {
#ifndef WINDOWS_OS
    char buf[256] = {0};
    if(0 == ::gethostname(buf, sizeof (buf))) {
        return std::string(buf);
    }
#else
    constexpr auto INFO_BUFFER_SIZE = 32767;
    TCHAR  infoBuf[INFO_BUFFER_SIZE] = {0};
    DWORD  bufCharCount = INFO_BUFFER_SIZE;
    if(GetComputerName(infoBuf, &bufCharCount ))
        return std::string(infoBuf);
#endif
    return std::string();
}

bool GempyreUtils::is_hidden_entry(const std::string& filename) {
#ifndef WINDOWS_OS
    char s[4096] = {0};
    filename.copy(s, filename.length());
    const std::string name = ::basename(s);
    return name.length() > 0 && name[0] == '.';
#else
    const auto attr = GetFileAttributes(filename.c_str());
    return 0 != ((FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) & attr);
#endif
}

#if 0
long GempyreUtils::time_stamp(const std::string& filename) {
#if WINDOWS_OS
    struct _stat buf;
    if(::_stat(filename.c_str(), &attr) == -1) {
#else
    struct stat attr;
    if(::stat(filename.c_str(), &attr) == -1) {
#endif
        log(LogLevel::Error, "time stamp request failed", filename,  ::strerror(errno));
        return 0;
    }
    return (static_cast<long>(attr.st_ctim.tv_sec) * 1000L) + attr.st_ctim.tv_nsec / 1000L; //ms
}
#endif

bool GempyreUtils::rename(const std::string& of, const std::string& nf) {
    if(std::rename(of.c_str(), nf.c_str())) {
        std::ifstream stream; //fallback
        stream.open(of, std::ios::in | std::ios::binary);
        if(!stream.is_open())
            return false;

        std::ofstream ostream;
        ostream.open(nf, std::ios::out | std::ios::binary);
        if(!ostream.is_open())
            return false;

        
        const size_t bufsz = 1024 * 16;
        auto bufferData = std::make_unique<char[]>(bufsz);
        auto buffer = bufferData.get();

        for (;;) {
            stream.read(buffer, bufsz);
            const auto read = stream.gcount();
            if(read <= 0)
                break;
            ostream.write(buffer, read);
        }
        stream.close();
        ostream.close();
    }
    return true;
}

std::string GempyreUtils::working_dir() {
#ifndef WINDOWS_OS
    size_t sz = 128;
    for(;;) {
        std::vector<char> buffer(sz);
        if(nullptr != ::getcwd(buffer.data(), sz)) {
            return std::string(buffer.data(), ::strlen(buffer.data()));
        } else sz *= 2;
    }
#else
    std::vector<char> buffer(GetCurrentDirectory(0, nullptr));
    GetCurrentDirectory(static_cast<DWORD>(buffer.size()), buffer.data());
    return std::string(buffer.data(), ::strlen(buffer.data()));
#endif
}

std::string GempyreUtils::slurp(const std::string& file, size_t max) {
    std::ifstream stream(file, std::ios::in | std::ios::ate);
    if(!stream.is_open()) {
        log(LogLevel::Error, "Cannot open file", qq(file));
        return "";
    }

    const auto size = std::min(max, static_cast<size_t>(stream.tellg()));
        if(size <= 0) {
        return "";
    }

    std::string str(size, '\0');

    stream.seekg(std::ios_base::beg);
    auto ptr = str.data();
    stream.read(ptr, static_cast<std::streamsize>(size));
    return str;
}

std::string GempyreUtils::hexify(const std::string& src, const std::string pat) {
    std::regex rx(pat);
    auto words_begin = std::sregex_iterator(src.begin(), src.end(),  rx);
    auto words_end = std::sregex_iterator();
    std::string out;
    size_t pos = 0;
    char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (auto i = words_begin; i != words_end; ++i) {
       const auto p = static_cast<decltype(pos)>(i->position());
       const auto l = p - pos;
       if(l > 0)
           out += src.substr(pos, p - pos);
       gempyre_utils_assert_x(i->str().length() == 1, "Bad pattern, should only match a single char");
       const auto f_byte = i->str()[0];
       out += '%';
       out += hex_chars[ ( f_byte & 0xF0 ) >> 4 ];
       out += hex_chars[ ( f_byte & 0x0F ) >> 0 ];
       pos = p + 1;
    }
    if(pos <  src.length())
        out += src.substr(pos, src.length() - pos);
    return out;
}


std::string GempyreUtils::unhexify(const std::string& src) {
    std::string out;
    for(auto it = src.begin() ; it != src.end(); ++it) {
        if(*it == '%') {
            char rep[] = {*(++it), *(++it), '\0'};
            const auto h = std::stoi(rep, nullptr, 16);
            out += static_cast<char>(h);
        } else {
            out += *it;
        }
    }
    return out;
}

#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif

bool GempyreUtils::is_executable(const std::string& filename) {
    if(!file_exists(filename)) {
        return false;
    }
    struct stat sb;
    return (stat(filename.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR);
}

SSIZE_T GempyreUtils::file_size(const std::string& filename) {
    std::ifstream stream(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if(!stream.is_open()) {
        log(LogLevel::Error, "Cannot open file", qq(filename));
        return -1;
    }
    return static_cast<SSIZE_T>(stream.tellg());
}

std::optional<std::string> GempyreUtils::which(const std::string& filename) {
    const auto pe = system_env("PATH");
    if(!pe)
        return std::nullopt;
    const auto path = split<std::vector<std::string>>(*pe,
#ifdef WINDOWS_OS
    ';');
#else
    ':');
#endif
    for(const auto& d : path) {
        if(d.empty())
            continue;
        const auto dir = d.back() == '\\' || d.back() == '/' ? d.substr(0, d.size() - 1) : d;
        for(const auto& name  : directory(dir)) {
#ifdef WINDOWS_OS
             const auto longName = dir + '\\' + name;
#else
            const auto longName = dir + '/' + name;
#endif
            if(!is_executable(longName))
                continue;
            const auto e = name.find_last_of('.');
#ifdef WINDOWS_OS
            if(e == std::string::npos)
                continue;
            const auto n = to_low(name);
            const auto ext = n.substr(e + 1);
            if(ext != "exe" && ext != "bat" && ext != "cmd")
                continue;
            const auto base_name = n.substr(0, e);
            std::string lname = to_low(filename);
            if(lname == base_name || lname == n)
                return longName;
#else
            const auto base_name = name.substr(0, e - 1);
            if(filename == name || filename == base_name)
                return longName;
#endif
        }

    }
    return std::nullopt;
}

std::shared_ptr<GempyreUtils::expiror> GempyreUtils::wait_expire(std::chrono::seconds s, const std::function<void ()>& onExpire) {
    auto token = std::shared_ptr<expiror>(new expiror);
    std::weak_ptr<expiror> ref = token;
    token->m_f = std::async([ref, s, onExpire](){
        auto count = s.count() * 10;
        while(count > 0 && !ref.expired()) {
            std::this_thread::sleep_for(100ms);
            --count;
        }
        if(!ref.expired()) {
            onExpire();
        }
    });
    return token;
}

GempyreUtils::OS GempyreUtils::current_os() {
#if defined(MAC_OS)
    return OS::MacOs;
#elif defined(WINDOWS_OS)
    return OS::WinOs;
#elif defined(UNIX_OS) && !defined(RASPBERRY_OS)
    return OS::LinuxOs;
#elif defined(ANDROID_OS)
    return OS::AndroidOs;
#elif defined(RASPBERRY_OS)
    return OS::RaspberryOs;
#else
    return OS::OtherOs;
#endif
}

std::string GempyreUtils::html_file_launch_cmd() {
    return
    #if defined(UNIX_OS) || defined(RASPBERRY_OS) //maybe works only on Debian derivatives
        "x-www-browser"
    #elif defined(MAC_OS)
        "open"
    #elif defined(WINDOWS_OS)
        "start /max"
    #else
        ""
    #endif
            ;
}

template<typename T>
static std::string printTime(std::chrono::time_point<T> time) {
    using namespace std;
    using namespace std::chrono;

    const auto curr_time = T::to_time_t(time);

    char buf[100];
    struct tm result;
#ifndef WINDOWS_OS
    if(localtime_r(&curr_time, &result))
        strftime(buf, sizeof(buf),"%Y-%m-%d %H:%M:%S", &result);
#else
    if(0 == localtime_s(&result, &curr_time))
    strftime(buf, sizeof(buf),"%Y-%m-%d %H:%M:%S", &result);
#endif

    typename T::duration since_epoch = time.time_since_epoch();
    const auto s = duration_cast<std::chrono::seconds>(since_epoch);
    since_epoch -= s;
    const auto milli = duration_cast<std::chrono::milliseconds>(since_epoch);

    strcat(buf, ".");
    strcat(buf, std::to_string(milli.count()).c_str());
    return std::string(buf);
}

std::string GempyreUtils::current_time_string() {
    return printTime(std::chrono::system_clock::now());
}

std::string GempyreUtils::last_error() {
#ifdef WINDOWS_OS
    DWORD dw = GetLastError();
    return last_error(dw);
#else
    const int err = errno;
    return last_error(err);
#endif
}

/*
#ifdef UNIX_OS //fix if needed
bool GempyreUtils::setPriority(int priority) {
      sched_param sch;
      int thisPolicy;
      auto err = pthread_getschedparam(pthread_self(), &thisPolicy, &sch);
      if(err) {
          log(LogLevel::Debug, "setPriority error:", ::strerror(errno));
          return false;
      }
      if(thisPolicy == SCHED_OTHER) {
          thisPolicy = SCHED_FIFO;
          log(LogLevel::Debug, "policy change:", SCHED_OTHER, thisPolicy);
      }
      sch.sched_priority = priority;
      err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch);
      if(err){
          log(LogLevel::Debug, "setPriority error:", ::strerror(errno));
          return false;
      }
      return true;
}

std::pair<int, int> GempyreUtils::getPriorityLevels() {
    return {sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO)};
}
#endif
*/

bool GempyreUtils::is_available(int port) {
    const auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WINDOWS_OS
    if(sockfd == INVALID_SOCKET ) {
        return false;
    } 
#else
    if(sockfd < 0) {
        return false;
    }
#endif
    struct sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(static_cast<unsigned short>(port));
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        if( errno == EADDRINUSE )
           return false;
        }
#ifndef WINDOWS_OS    
    if (close (sockfd) < 0 ) {
        return false;
    }
#else
 if (closesocket (sockfd) < 0 ) {
        return false;
    }
#endif
    return true;
}

std::vector<std::string> GempyreUtils::ip_addresses(int addressType) {
    std::vector<std::string> addresses;
#ifndef WINDOWS_OS
    struct ifaddrs *ifaddr;
    if (::getifaddrs(&ifaddr) < 0)
        return addresses;
    /* Walk through linked list, maintaining head pointer so we can free list later */
    for (auto *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;
        const auto sa_family = ifa->ifa_addr->sa_family;
        if ((sa_family == AF_INET && (addressType & AddressType::Ipv4)) ||
                (sa_family == AF_INET6 && (addressType & AddressType::Ipv6))) {
            char host[1025];
            const auto s = ::getnameinfo(ifa->ifa_addr, (sa_family == AF_INET) ?
                                           sizeof(struct sockaddr_in) :
                                           sizeof(struct sockaddr_in6),
                               host, sizeof(host),
                               NULL, 0, NI_NUMERICHOST);
           if(s == 0 && std::strcmp(host, "127.0.0.1") && std::strcmp(host, "::1"))
              addresses.push_back(host);
        }
    }
    ::freeifaddrs(ifaddr);
    return addresses;

#else
     const auto a_family = addressType == AddressType::Ipv4 
    ? AF_INET : (addressType == AddressType::Ipv6 ? AF_INET6 :  AF_UNSPEC);
    (void) a_family;
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
    ULONG adapterBufferSize = 15 * 1024;
    auto buf_ptr = std::make_unique<uint8_t[]>(adapterBufferSize);
    IP_ADAPTER_ADDRESSES* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf_ptr.get());
    while(true)  {
        const auto err = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &adapterBufferSize);
        if(err == ERROR_SUCCESS )
            break;
        else if(err == ERROR_BUFFER_OVERFLOW) {
            buf_ptr = std::make_unique<uint8_t[]>(adapterBufferSize);
            adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf_ptr.get());
        }
        else {
            log(LogLevel::Error, "ipAddresses error:", last_error(err));
            return addresses;
        }
    }
    for(auto adapter = adapters; adapter; adapter = adapter->Next) {
        if (IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)     // Skip loopback adapters
            continue;
        for(auto* address = adapter->FirstUnicastAddress; address; address = address->Next) {
            const auto sa_family = address->Address.lpSockaddr->sa_family;
            if(sa_family == AF_INET) {
                auto ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
                char str_buffer[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &(ipv4->sin_addr), str_buffer, INET_ADDRSTRLEN);
                addresses.push_back(std::string(str_buffer));
                }
            else if(sa_family == AF_INET6) {
                SOCKADDR_IN6* ipv6 = reinterpret_cast<SOCKADDR_IN6*>(address->Address.lpSockaddr);
                char str_buffer[INET6_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET6, &(ipv6->sin6_addr), str_buffer, INET6_ADDRSTRLEN);
                // Detect and skip non-external addresses,https://stackoverflow.com/questions/122208/get-the-ip-address-of-local-computer
                auto is_link_local = false;
                auto is_special_use = false;
                const auto ipv6_str = std::string(str_buffer);
                if (0 == ipv6_str.find("fe")) {
                    char c = ipv6_str[2];
                    if (c == '8' || c == '9' || c == 'a' || c == 'b')
                        is_link_local = true;
            
                } else if (0 == ipv6_str.find("2001:0:"))
                    is_special_use = true;
                if (!(is_link_local || is_special_use))
                    addresses.push_back(ipv6_str);
        
            }
        }
    }           
    return addresses;
#endif
}

std::string GempyreUtils::base64_encode(const unsigned char* bytes, size_t sz) {
    return Base64::encode(bytes, sz);
}

std::vector<unsigned char> GempyreUtils::base64_decode(const std::string_view& data) {
    return Base64::decode(data);
}

std::string GempyreUtils::push_path(const std::string& path, const std::string& name) {
#ifdef OS_WIN
    return path + '\\' + name;
 #else
    return path + '/' + name;
#endif
}

int GempyreUtils::execute(const std::string& executable, const std::string& parameters) {
#if defined(WINDOWS_OS)
    if(executable.empty())
        return system(parameters.c_str()); // for compatibility with osbrowser
    else {
        /*
        STARTUPINFO si = {};
        si.cb = sizeof (STARTUPINFO);
        PROCESS_INFORMATION pi = {};
        auto cmd = executable + " " + parameters;
        auto plist = const_cast<char*>(parameters.c_str());
        const auto ok = CreateProcess(executable.c_str(),   // No module name (use command line)
                plist,        // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                FALSE,          // Set handle inheritance to FALSE
                CREATE_NEW_PROCESS_GROUP,              //new process
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory
                &si,            // Pointer to STARTUPINFO structure
                &pi);           // Pointer
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        return ok ? 0 : 1;*/
        const auto hi = reinterpret_cast<INT_PTR>(::ShellExecuteA(NULL, NULL, executable.c_str(), parameters.c_str(), NULL, SW_SHOWNORMAL));
        return static_cast<int>((hi > 32 || hi < 0) ? 0 : hi); //If the function succeeds, it returns a value greater than 32. If the function fails, it returns an error value that indicates the cause of the failure.
    }
#else
    return std::system((executable + " " + parameters + " &").c_str());
#endif
}

std::string GempyreUtils::trimmed(const std::string& s) {
    return substitute(s, R"(\s+)", std::string{});
}

int GempyreUtils::levenshtein_distance(std::string_view s1, std::string_view s2) {
    const auto l1 = s1.length();
    const auto l2 = s2.length();

    auto dist = std::vector<std::vector<unsigned>>(l2 + 1, std::vector<unsigned>(l1 + 1));

    for(auto i = 0U; i <= l1 ; i++) {
       dist[0][i] = i;
    }

    for(auto j = 0U; j <= l2; j++) {
       dist[j][0] = j;
    }

    for (auto j = 1U; j <= l1; j++) {
       for(auto i = 1U; i <= l2 ;i++) {
          const auto track = (s2[i-1] == s1[j-1]) ? 0U : 1U;
          const auto t = std::min((dist[i - 1][j] + 1), (dist[i][j - 1] + 1));
          dist[i][j] = std::min(t, (dist[i - 1][j - 1] + track));
       }
    }
    return static_cast<int>(dist[l2][l1]);
}


std::string GempyreUtils::home_dir() {
#ifdef WINDOWS_OS
   constexpr int max_data = 512;
   TCHAR path_buf[max_data];
   const auto sz = GetEnvironmentVariable("USERDATA", path_buf, max_data);
   if(sz)
       return std::string(path_buf, sz);
#else
    const auto home = getenv("HOME");
    if(home)
        return std::string{home};
#endif
    return {};
}


std::string GempyreUtils::root_dir() {
#ifdef WINDOWS_OS
    return std::string{"C:\\"};
#else
    return std::string{"/"};
#endif
}

void GempyreUtils::process_exit(int exitCode) {
    set_log_level(GempyreUtils::LogLevel::None); // There is something gone and logs do dangling
#ifdef WINDOWS_OS
    PostQuitMessage(exitCode); // try to flush buffers
    Sleep(20); //20ms
#endif
    std::exit(exitCode);
}