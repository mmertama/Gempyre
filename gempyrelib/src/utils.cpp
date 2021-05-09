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

#include <stdio.h>
#include <variant>



#include "gempyre_utils.h"

//without <filesystem> support
#include <stdlib.h>

namespace GempyreUtils {
template<typename T>
static std::string lastError(T err) {
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
using namespace GempyreUtils;

#ifndef WINDOWS_OS
template <size_t SZ>
class SysLogStream : public std::streambuf {
public:
    SysLogStream() : m_os(this) {
        setp(m_buffer, m_buffer + SZ - 1);
    }
    ~SysLogStream() override {
        ::closelog();
    }
    std::ostream& stream(GempyreUtils::LogLevel logLevel) {
        static int priorities[] = {LOG_EMERG, LOG_ERR, LOG_INFO, LOG_DEBUG};
        m_prio = priorities[static_cast<int>(logLevel)];
        return m_os;
    }
private:
    int_type overflow(int_type ch) override {
        if(ch != traits_type::eof()){
            *pptr() = static_cast<char>(ch);
            pbump(1);
            write();
        }
        return ch;
    }
    int sync() override {
        write();
        return 1;
    }
    void write() {
        std::ptrdiff_t n = pptr() - pbase();
#ifdef COMPILER_CLANG
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    #pragma clang diagnostic ignored "-Wformat-security"
#endif
#ifdef COMPILER_GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
        char ntBuf[SZ];
        std::memcpy(ntBuf, m_buffer, static_cast<size_t>(n));
        ntBuf[n] = '\0';
        ::syslog(m_prio, ntBuf);
#ifdef COMPILER_CLANG
    #pragma clang diagnostic pop
#endif
#ifdef COMPILER_GCC
    #pragma GCC diagnostic pop
#endif
        pbump(static_cast<int>(-n));
    }
private:
    char m_buffer[SZ];
    int m_prio = LOG_DEBUG;
    int PADDING;
    std::ostream m_os;
};


static std::unique_ptr<SysLogStream<1024>> gSysLogStream;
#endif

static GempyreUtils::LogLevel g_serverLogLevel = GempyreUtils::LogLevel::
#ifdef UTILS_LOGLEVEL
UTILS_LOGLVEL
#else
Error
#endif
;


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
        GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Cannot initialize socket, Windows WSAStartup failed: code", code, "Last error:",  lastError());
    }
#endif
}

std::string GempyreUtils::toStr(LogLevel l) {
    const std::unordered_map<LogLevel, std::string> m = {
        {LogLevel::None, "NONE"},
        {LogLevel::Error, "ERROR"},
        {LogLevel::Warning, "WARNING"},
        {LogLevel::Info, "INFO"},
        {LogLevel::Debug, "DEBUG"},
        {LogLevel::Fatal, "FATAL"},
        {LogLevel::Debug_Trace, "TRACE"}
    };
    return m.at(l);
}

std::string GempyreUtils::qq(const std::string& s) {
   std::stringstream ss;
   ss << std::quoted(s);
   return ss.str();
}

std::string GempyreUtils::chop(const std::string& s) {
    auto str = s;
    str.erase(str.find_last_not_of("\t\n\v\f\r ") + 1);
    return str;
}

std::string GempyreUtils::chop(const std::string& s, const std::string& chopped) {
    auto str = s;
    str.erase(str.find_last_not_of(chopped) + 1);
    return str;
}

void GempyreUtils::setLogLevel(GempyreUtils::LogLevel level, bool useSysLog) {

    g_serverLogLevel = level;

#ifndef  WINDOWS_OS
    if(useSysLog)
        gSysLogStream.reset(new SysLogStream<1024>);
#else
     (void) useSysLog;
#endif // ! WINDOWS_OS
}

static std::mutex global_stream_mutex;
DebugStream GempyreUtils::logStream(LogLevel logLevel) {
#ifdef WINDOWS_OS
    (void) logLevel;
#endif
    auto& strm =
#ifndef WINDOWS_OS
		gSysLogStream ? gSysLogStream->stream(logLevel) :
#endif
        std::cerr;
    return DebugStream(&global_stream_mutex,&strm);
}

GempyreUtils::LogLevel GempyreUtils::logLevel() {
    return g_serverLogLevel;
}

bool GempyreUtils::useSysLog() {
#ifndef WINDOWS_OS
    return static_cast<bool>(gSysLogStream);
#else
	return false;
#endif
}

std::variant<std::tuple<std::multimap<std::string, std::string>, std::vector<std::string>>, int> GempyreUtils::parseArgs(int argc, char* argv[], const std::initializer_list<std::tuple<std::string, char, ArgType>>& args) {
#ifndef WINDOWS_OS
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
        }
        ++argi;
    }
    longOptions[args.size()] = {nullptr, 0, nullptr, 0};

    std::multimap<std::string, std::string> options;

    for(;;) {
        auto opt = getopt_long(argc, argv, plist.c_str(),
                            longOptions, nullptr);
        if(opt < 0)
            break;

        if(opt == '?') {
            return ParsedParameters(optind);
        }

        const auto index = std::find_if(longOptions, longOptions + args.size(), [opt](const ::option& o){return o.val == opt;});

        options.emplace(longOptions[std::distance(longOptions, index)].name, optarg ? optarg : "");
    }

    std::vector<std::string> params;
    while(optind < argc) {
        params.push_back(argv[optind++]);
    }
#else
    std::vector<std::string> quoted;
    std::transform(argv + 1, argv + argc, std::back_inserter(quoted), [](const auto cstr) {return qq(std::string(cstr));});
    int numArgs;
    const auto commandLine = join(quoted.begin(), quoted.end(), " ");
    std::wstring argString(commandLine.begin(), commandLine.end());

    auto wlist = CommandLineToArgvW(argString.c_str(), &numArgs);
    std::unique_ptr<LPWSTR, decltype(&LocalFree)> wlistPtr(wlist, LocalFree);
    std::vector<std::string> plist;
    for(auto ii = 0; ii < numArgs; ii++) {
        const std::wstring w = wlist[ii];
        plist.push_back(std::string(w.begin(), w.end()));
    }

    Options options;
    ParamList params;
    for(auto i = 0U; i < static_cast<unsigned>(numArgs); i++) {
        const auto arg = plist[i];
        if(arg[0] == '-') {
            if(arg.length() < 2)
                return ParsedParameters{static_cast<int>(i)};
            decltype(args.end()) it;
            bool longOpt = false;
            auto assing = arg.end();
            if(arg[1] == '-') {
                if(arg.length() < 3)
                    return ParsedParameters{static_cast<int>(i)};
                longOpt = true;
                const auto key = arg.substr(2);
                assing = std::find(arg.begin(), arg.end(), '=');
                it = std::find_if(args.begin(), args.end(), [&key](const auto& a){return std::get<0>(a) == key;});
            } else {
                const auto key = arg.substr(1, 1);
                it = std::find_if(args.begin(), args.end(), [&key](const auto& a){return std::get<1>(a) == key[0];});
            }
            if(it != args.end()) {
                switch(std::get<ArgType>(*it)) {
                case ArgType::NO_ARG:
                    options.emplace(std::get<std::string>(*it), "true");
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
                    } else return ParsedParameters{static_cast<int>(i)};
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
                        val = "true";
                    }
                     options.emplace(std::get<std::string>(*it), val);
                    } break;
                }
            }
        } else {
            params.push_back(arg);
        }

    }
#endif
    return ParsedParameters(std::make_tuple(options, params));
}

std::string GempyreUtils::absPath(const std::string& rpath) {
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

std::string GempyreUtils::baseName(const std::string& filename) {
    const auto dname = pathPop(filename);
    return dname.empty() ? filename :
        filename.substr(dname.length() + 1);
}

std::string GempyreUtils::pathPop(const std::string& filename, int steps) {
    if (steps <= 0)
        return filename;
    else {
        const auto p = filename.find_last_of('/');
        return pathPop(p != std::string::npos ? filename.substr(0, p) : "", steps - 1);
    }
}

#ifndef WINDOWS_OS
std::string GempyreUtils::readProcess(const std::string& processName) {
    auto fd = ::popen(processName.c_str(), "r");
    gempyre_utils_assert_x(fd, "cannot create pipe");
    gempyre_utils_auto_close(fd, ::pclose);
    std::string out;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fd) != nullptr) {
           out += buffer;
    }
    return out;
}
#endif

#ifdef WINDOWS_OS
#define USE_TEMPNAM
#endif

std::string GempyreUtils::tempName() {
#ifndef USE_TEMPNAM
//in MT this may NOT be any more safe than the std::tmpnam
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
#else
    const auto name = std::tmpnam(nullptr); //tmpnam is not encouraged to use, but some systems mkstemp may not be supported
#endif
    return name;
}

void GempyreUtils::removeFile(const std::string& name) {
    ::remove(name.c_str());
}


std::string GempyreUtils::substitute(const std::string& str, const std::string& substring, const std::string& subsitution) {
    try {
    return std::regex_replace(str, std::regex(substring) , subsitution);
    } catch (std::regex_error& e) {
         log(LogLevel::Error, "reg exp", substring, e.code());
         gempyre_utils_assert_x(false, "regexp");
    }
    return std::string();
}

#ifdef UNIX_OS //fix if needed
std::string GempyreUtils::appPath() {
    return getLink("/proc/self/exe");  //obviously UNIX only
}
#endif


bool GempyreUtils::fileExists(const std::string& filename) {
#ifndef WINDOWS_OS
    return ::access(filename.c_str(), F_OK) != -1;
#else
    return (0xFFFFFFFF != GetFileAttributes(filename.c_str()));
#endif
}


std::vector<std::tuple<std::string, bool, std::string>> GempyreUtils::directory(const std::string& dirname) {

    const auto dname = dirname.back() != '/' ? dirname + '/' : dirname;
    std::vector<std::tuple<std::string, bool, std::string>> entries;
#ifndef WINDOWS_OS
    auto dir = ::opendir(dname.c_str());
    if(!dir)
        return entries;
    while(auto dirEntry = readdir(dir)) {
        if(dirEntry->d_type == DT_LNK) {
            const auto fullname = dname + dirEntry->d_name;
            const auto linked = GempyreUtils::getLink(fullname);
            const auto fulllink = dname + linked;
            entries.push_back({dirEntry->d_name, GempyreUtils::isDir(fulllink), fulllink});
        } else
            entries.push_back({dirEntry->d_name, dirEntry->d_type == DT_DIR, ""});
    }
#else
        const auto searchPath = dirname + "/*.*";
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &fd);
        if(hFind != INVALID_HANDLE_VALUE) {
            do {
                const auto isDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                entries.push_back({fd.cFileName, isDir, ""});
            } while(::FindNextFile(hFind, &fd));
            ::FindClose(hFind);
        }
#endif
        return entries;
}    


#ifndef WINDOWS_OS
bool GempyreUtils::isDir(const std::string& path) {
   struct stat statbuf;
   if (::stat(path.c_str(), &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
#endif


std::string GempyreUtils::getLink(const std::string& lname) {
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

std::string GempyreUtils::systemEnv(const std::string& env) {
    if(env.length() > 0) {
        auto str = std::getenv(env.c_str());
        if(str) {
            return str;
        }
    }
    return std::string();
}

std::string GempyreUtils::hostName() {
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

bool GempyreUtils::isHiddenEntry(const std::string& filename) {
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

#ifdef UNIX_OS
long GempyreUtils::timeStamp(const std::string& filename) {
    struct stat attr;
    if(::stat(filename.c_str(), &attr) == -1) {
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

        const size_t bufsz = 1024 * 1024;
        char buffer[bufsz];

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

#ifndef WINDOWS_OS
std::string GempyreUtils::workingDir() {
    size_t sz = 128;
    for(;;) {
        std::vector<char> buffer(sz);
        if(nullptr != ::getcwd(buffer.data(), sz)) {
            return std::string(buffer.data(), ::strlen(buffer.data()));
        } else sz *= 2;
    }
}
#else
std::string GempyreUtils::workingDir() {
    std::vector<char> buffer(GetCurrentDirectory(0, nullptr));
    GetCurrentDirectory(buffer.size(), buffer.data());
    return std::string(buffer.data(), ::strlen(buffer.data()));
}
#endif

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
       const auto byte = i->str()[0];
       out += '%';
       out += hex_chars[ ( byte & 0xF0 ) >> 4 ];
       out += hex_chars[ ( byte & 0x0F ) >> 0 ];
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

bool GempyreUtils::isExecutable(const std::string& filename) {
    if(!fileExists(filename)) {
        return false;
    }
    struct stat sb;
    return (stat(filename.c_str(), &sb) == 0 && sb.st_mode & S_IXUSR);
}

SSIZE_T GempyreUtils::fileSize(const std::string& filename) {
    std::ifstream stream(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if(!stream.is_open()) {
        log(LogLevel::Error, "Cannot open file", qq(filename));
        return -1;
    }
    return stream.tellg();
}

std::string GempyreUtils::which(const std::string& filename) {
    const auto pe = systemEnv("PATH");
    const auto path = split<std::vector<std::string>>(pe,
#ifdef WINDOWS_OS
    ';');
#else
    ':');
#endif
    for(const auto& d : path) {
        for(const auto& [name, isdir, link] : directory(d)) {
            (void) isdir;
            (void) link;
            const auto longName = d + "/" + name;
            if(!isExecutable(longName))
                continue;
            const auto e = name.find_last_of('.');
#ifdef WINDOWS_OS
            if(e == std::string::npos)
                continue;
            const auto n = toLow(name);
            const auto ext = n.substr(e + 1);
            if(ext != "exe" || ext != "bat" || ext != "cmd")
                continue;
            const auto basename = n.substr(0, e - 1);
            std::string lname = toLow(filename);
            if(lname == basename || lname == n)
                return longName;
#else
            const auto basename = name.substr(0, e - 1);
            if(filename == name || filename == basename)
                return longName;
#endif
        }

    }
    return "";
}

std::shared_ptr<GempyreUtils::expiror> GempyreUtils::waitExpire(std::chrono::seconds s, const std::function<void ()>& onExpire) {
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

GempyreUtils::OS GempyreUtils::currentOS() {
#if defined(MAC_OS)
    return OS::MacOs;
#elif defined(WINDOWS_OS)
    return OS::WinOs;
#elif defined(UNIX_OS)
    return OS::LinuxOs;
#elif defined(ANDROID_OS)
    return OS::AndroidOs;
#else
    return OS::OtherOs;
#endif
}

std::string GempyreUtils::osBrowser() {
    return
    #if defined(UNIX_OS) //maybe works only on Debian derivatives
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

std::string GempyreUtils::currentTimeString() {
    const auto result = std::time(nullptr);
    char timebuf[64];
    strftime(timebuf, sizeof timebuf, "%c", std::localtime(&result));
    return GempyreUtils::chop(timebuf);
}

std::string GempyreUtils::lastError() {
#ifdef WINDOWS_OS
    DWORD dw = GetLastError();
    return lastError(dw);
#else
    const int err = errno;
    return lastError(err);
#endif
}

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


bool GempyreUtils::isAvailable(int port) {
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0 ) {
        return false;
    }
    struct sockaddr_in serv_addr = {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
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

std::vector<std::string> GempyreUtils::ipAddresses(int addressType) {
    std::vector<std::string> addresses;
#ifndef WINDOWS_OS
    struct ifaddrs *ifaddr;
    if (::getifaddrs(&ifaddr) < 0)
        return addresses;
    /* Walk through linked list, maintaining head pointer so we can free list later */
    for (auto *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;
        const auto family = ifa->ifa_addr->sa_family;
        if ((family == AF_INET && (addressType & AddressType::Ipv4)) ||
                (family == AF_INET6 && (addressType & AddressType::Ipv6))) {
            char host[1025];
            const auto s = ::getnameinfo(ifa->ifa_addr, (family == AF_INET) ?
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
    const auto family = addressType == AddressType::Ipv4 
    ? AF_INET : (addressType == AddressType::Ipv6 ? AF_INET6 :  AF_UNSPEC);  
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
            log(LogLevel::Error, "ipAddresses error:", lastError(err));
            return addresses;
        }
    }
    for(auto adapter = adapters; adapter; adapter = adapter->Next) {
        if (IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)     // Skip loopback adapters
            continue;
        for(auto* address = adapter->FirstUnicastAddress; address; address = address->Next) {
            const auto family = address->Address.lpSockaddr->sa_family;
            if(family == AF_INET) {
                auto ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
                char str_buffer[INET_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET, &(ipv4->sin_addr), str_buffer, INET_ADDRSTRLEN);
                addresses.push_back(std::string(str_buffer));
                }
            else if(family == AF_INET6) {
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

