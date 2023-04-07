#include "server.h"
#include "gempyre_utils.h"

using namespace Gempyre;

constexpr unsigned short DEFAULT_PORT  = 30000;
constexpr unsigned short PORT_ATTEMPTS = 50;

 unsigned Server::wishAport(unsigned port, unsigned max) {
    auto end = port + max;
    while(!GempyreUtils::is_available(static_cast<unsigned short>(port))) {
        ++port;
        if(port == end) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "wish a port", GempyreUtils::last_error());
            return 0;
        }
    }
    return port;
}

unsigned Server::portAttempts() {
    return PORT_ATTEMPTS;
}

Server::Server(
    unsigned port,
    const std::string& root,
    const OpenFunction& onOpen,
    const MessageFunction& onMessage,
    const CloseFunction& onClose,
    const GetFunction& onGet,
    const ListenFunction& onListen,
    int queryIdBase) :
    m_port(port == 0 ? wishAport(DEFAULT_PORT, PORT_ATTEMPTS) : port),
    m_rootFolder(root),
     m_queryId{queryIdBase},
    m_onOpen(onOpen),
    m_onMessage(onMessage),
    m_onClose(onClose),
    m_onGet(onGet),
    m_onListen(onListen)
    {}

    std::string Server::fileToMime(const std::string_view& filename) {
        const auto index = filename.find_last_of('.');
        if(index == std::string::npos) {
            return "";
        }
        const std::string_view ext(&filename[index], filename.length() - index);
        static const std::unordered_map<std::string_view, std::string_view> mimes = {
            {".html", "text/html;charset=utf-8"},
            {".css", "text/css;charset=utf-8"},
            {".js", "text/javascript;charset=utf-8"},
            {".txt", "text/txt;charset=utf-8"},
            {".ico", "image/x-icon"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".gif", "image/gif"},
            {".svg", "image/svg+xml"}
        };
        const auto it = mimes.find(ext);
        const auto mimeType = it == mimes.end() ? " application/octet-stream" : std::string(it->second);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Mime type:", filename, mimeType);
        return mimeType;
    }

    std::string Server::notFoundPage(const std::string_view& url, const std::string_view& info) {
        return R"(<html>
           <header>
               <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate, max-age=0 "/>
               <meta http-equiv="Pragma" content="no-cache" />
               <meta http-equiv="Expires" content="0" />
               <meta charset="UTF-8">
               <style>
               #styled {
                   color:red;
                   font-size:32px
               }
               </style>
           </header>
      <body><h1>Ooops</h1><h3 class="styled">404 Data Not Found </h3><h5>)" + std::string(url) + "</h5><i>" + std::string(info) + "</i></body></html>";
}