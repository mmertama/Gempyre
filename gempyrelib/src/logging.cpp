
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

std::string GempyreUtils::to_str(LogLevel l) {
#ifdef WINDOWS_OS
    const std::unordered_map<LogLevel, std::string> m = {
        {LogLevel::None, "NONE"},
        {LogLevel::Error, "ERROR"},
        {LogLevel::Warning, "WARNING"},
        {LogLevel::Info, "INFO"},
        {LogLevel::Debug, "DEBUG"},
        {LogLevel::Fatal, "FATAL"},
        {LogLevel::Debug_Trace, "TRACE"}
    };
#else
        const std::unordered_map<LogLevel, std::string> m = {
        {LogLevel::None, "NONE"},
        {LogLevel::Error, "\e[1;31mERROR\e[0m"},//Red text
        {LogLevel::Warning, "\e[1;48:5:166mWARNING\e[0m"},//Orange background
        {LogLevel::Info, "\e[0;106m\e[1;90mINFO\e[0m"},//Cyan background + Black text
        {LogLevel::Debug, "\e[0;103m\e[1;90mDEBUG\e[0m"},//Yello background + Black text
        {LogLevel::Fatal, "\e[4;31mFATAL\e[0m"},//Dark red with underline
        {LogLevel::Debug_Trace, "\e[0;103m\e[1;90mTRACE\e[0m"}//Yello background + Black text
    };
#endif
    return m.at(l);
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
};
#else
class ErrStream : public LogWriter {
    bool do_write(const char* bytes, size_t count) override {
       (void) count;
       std::cerr << bytes;
       return true;
    }
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
    char m_buffer[SZ + 1];
    LogWriter* m_logWriter;
};


FileLogWriter::FileLogWriter(const std::string& path) : m_file(path, std::ios::out | std::ios::app )  {}
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

static std::atomic<GempyreUtils::LogWriter*> g_logWriter{nullptr};

void GempyreUtils::set_log_writer(LogWriter* writer) {
    g_logWriter = writer;
}

static ErrStream defaultErrorStream;

std::ostream GempyreUtils::log_stream(LogLevel logLevel) {
    auto strm  = (g_logWriter) ? g_logWriter.load() : &defaultErrorStream;
    static thread_local LogStream<1024> logStreamer;
    logStreamer.setWriter(strm);
    std::ostream(&logStreamer) << strm->header(logLevel);
    return std::ostream(&logStreamer);
}



