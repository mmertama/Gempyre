
#include <ostream>
#include <atomic>
#include <iostream>
#include <sstream>
#include <unordered_map>

#ifdef WINDOWS_OS
#include <Windows.h>                                
#include <iphlpapi.h>
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

using namespace GempyreUtils;


static std::atomic<GempyreUtils::LogWriter*> g_logWriter{nullptr};

static GempyreUtils::LogLevel g_serverLogLevel{GempyreUtils::LogLevel::
#ifdef UTILS_LOGLEVEL
UTILS_LOGLEVEL
#else
Error
#endif
};


void GempyreUtils::set_log_level(GempyreUtils::LogLevel level) {
    g_serverLogLevel = level;
}


GempyreUtils::LogLevel GempyreUtils::log_level() {
   return g_serverLogLevel;
}

// \033 for ISO, \e is GCC spesific 
#ifndef WINDOWS_OS

#define CLEAR "\033[0m"
#define RED "\033[1;31m"
#define ORANGE "\033[1;48:5:166m"
#define CYAN_TEXT "\033[0;106m\033[1;90m"
#define YELLOW "\033[0;33m"
#define DARK_RED_WITH_UNDERLINE "\033[4;31m"
#define WHITE_BACKGROUND_BLACK_TEXT "\033[0;47m\033[1;90m"

#else

#define CLEAR
#define RED
#define ORANGE
#define CYAN_TEXT
#define YELLOW
#define DARK_RED_WITH_UNDERLINE
#define WHITE_BACKGROUND_BLACK_TEXT

#endif

std::string GempyreUtils::to_str(LogLevel l) {
    const std::unordered_map<LogLevel, std::string> m = {
        {LogLevel::None, "NONE"},
        {LogLevel::Error, "ERROR"},
        {LogLevel::Warning, "WARNING"},
        {LogLevel::Info, "INFO"},
        {LogLevel::Debug, "DEBUG"},
        {LogLevel::Fatal, "FATAL"},
        {LogLevel::Debug_Trace, "TRACE"}
    };

    const std::unordered_map<LogLevel, std::string> ansi_m = {
        {LogLevel::None, "NONE"},
        {LogLevel::Error, RED "ERROR" CLEAR},//Red text
        {LogLevel::Warning, ORANGE "WARNING" CLEAR},//Orange background
        {LogLevel::Info, CYAN_TEXT "INFO" CLEAR},//Cyan background + Black text
        {LogLevel::Debug, YELLOW "DEBUG" CLEAR},//Yellow
        {LogLevel::Fatal, DARK_RED_WITH_UNDERLINE "FATAL" CLEAR},//Dark red with underline
        {LogLevel::Debug_Trace, WHITE_BACKGROUND_BLACK_TEXT "TRACE" CLEAR}//White background + Black text
    };
    return g_logWriter && g_logWriter.load()->has_ansi() ? ansi_m.at(l) : m.at(l);
}

std::string LogWriter::header(LogLevel logLevel) {
    std::stringstream buf;
    buf << '[' << current_time_string() << "] " << to_str(logLevel) << " ";
    return buf.str();
}

#ifdef WINDOWS_OS
class ErrStream : public LogWriter {
    bool do_write(const char* bytes, size_t count) override {
        (void) count;
        OutputDebugStringA(bytes);
        return true;
    }
    bool has_ansi() const override {return false;} 
};
#else
class ErrStream : public LogWriter {
    bool do_write(const char* bytes, size_t count) override {
       (void) count;
       std::cerr << bytes << std::flush;
       return true;
    }
    bool has_ansi() const override {return true;} 
};
#endif


template <size_t SZ>
class LogStream : public std::streambuf {
public:
    LogStream() {
        setp(m_buffer, m_buffer + SZ - 1);
    }
    ~LogStream() = default;
    void setWriter(LogWriter* writer) {
        m_logWriter = writer;
    }
    LogStream (const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;


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
        m_buffer[n] = '\0';
        if(!m_logWriter->do_write(m_buffer, static_cast<size_t>(n))) {
            std::cerr << "Log cannot write " << m_buffer << std::endl;
        }
        pbump(static_cast<int>(-n));
    }
private:
    char m_buffer[SZ + 1] = {'\0'};
    LogWriter* m_logWriter = nullptr;
};


class DevNull : public LogWriter {
    bool do_write(const char* bytes, size_t count) override {
       (void) count;
       (void) bytes;
       return true;
    }
    bool has_ansi() const override {return true;} 
};


FileLogWriter::FileLogWriter(std::string_view path) : m_file(path, std::ios::out | std::ios::app )  {}
bool FileLogWriter::do_write(const char* bytes, size_t count) {
        (void) count;
        if(!m_file.is_open())
            return false;
        m_file << bytes << std::flush; // flush to catch crashes
        return true;
    }

StreamLogWriter::StreamLogWriter(std::ostream& os) : m_os(os)  {}
bool StreamLogWriter::do_write(const char* bytes, size_t count) {
        (void) count;
        if(!m_os.good())
            return false;
        m_os << bytes << std::flush; // flush to catch crashes
        return true;
    }

static ErrStream defaultErrorStream;

std::ostream GempyreUtils::log_stream(LogLevel logLevel) {
    auto strm  = (g_logWriter) ? g_logWriter.load() : &defaultErrorStream;
    static thread_local LogStream<1024> logStreamer;
    logStreamer.setWriter(strm);
    std::ostream(&logStreamer) << strm->header(logLevel);
    return std::ostream(&logStreamer);
}



LogWriter::LogWriter() : m_previousLogWriter(g_logWriter) 
{
    g_logWriter = this;
}

LogWriter::~LogWriter() 
{
    g_logWriter = m_previousLogWriter;
}

/* not needed ?
void LogWriter::set_none() {
    static DevNull dev_null;
    g_logWriter = &dev_null;
}
*/

bool LogWriter::has_ansi() const {
    return false;
}
