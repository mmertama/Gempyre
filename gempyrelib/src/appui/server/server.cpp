#include "server.h"
#include "gempyre_utils.h"

using namespace Gempyre;

constexpr unsigned short DEFAULT_PORT  = 30000;
constexpr unsigned short PORT_ATTEMPTS = 50;

static
unsigned wishAport(unsigned port, unsigned max) {
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

Server::Server(
    unsigned port,
    const std::string& root,
    OpenFunction&& onOpen,
    MessageFunction&& onMessage,
    CloseFunction&& onClose,
    GetFunction&& onGet,
    ListenFunction&& onListen,
    int queryIdBase) :
    m_port{port == 0 ? wishAport(DEFAULT_PORT, PORT_ATTEMPTS) : port},
    m_rootFolder{root},
    m_queryId{queryIdBase},
    m_onOpen{onOpen},
    m_onMessage{onMessage},
    m_onClose{onClose},
    m_onGet{onGet},
    m_onListen{onListen}
    {}

    std::string Server::fileToMime(std::string_view filename) {
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

    std::string Server::notFoundPage(std::string_view url, std::string_view info) {
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

Server::MessageReply Server::messageHandler(std::string_view message) {
        auto object = json::parse(message);
        const auto f = object.find("type");
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "ServerMsg", f != object.end() ? *f : "N/A");
        if(f != object.end()) {
            if(*f == "keepalive") {
                return MessageReply::DoNothing;
            }
            if(*f == "ui_ready") {
                m_onMessage(std::move(object));
                return MessageReply::AddUiSocket;
            }
            if(*f == "extension_ready") {
                m_onMessage(std::move(object));
                return MessageReply::AddExtensionSocket;
            }
            if(*f == "extension") {
                const auto log = object.find("level");
                const auto msg = object.find("msg");
                if(*log == "log")
                    GempyreUtils::log(GempyreUtils::LogLevel::Info, "Ext", *msg);
                else if(*log == "info")
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ext", *msg);
                else if(*log == "warn")
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "Ext", *msg);
                else if(*log == "error" || log->empty())
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "Ext", *msg);
                return  MessageReply::DoNothing;
            }
            if(*f == "log") {
                const auto log = object.find("level");
                const auto msg = object.find("msg");
                if(*log == "log")
                    GempyreUtils::log(GempyreUtils::LogLevel::Info, "JS", *msg);
                else if(*log == "info")
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS", *msg);
                else if(*log == "warn")
                    GempyreUtils::log(GempyreUtils::LogLevel::Warning, "JS", *msg);
                else if(*log == "" || *log == "error") {
                    GempyreUtils::log(GempyreUtils::LogLevel::Error, "JS", *msg);
                    const auto trace = object.find("trace");
                    if(trace != object.end()) {
                        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "JS-TRACE", *trace);
                    }
                }
                return MessageReply::DoNothing;
            }
        }
        m_onMessage(std::move(object));
        return MessageReply::DoNothing;
    }

bool Server::beginBatch() {
    m_batch = std::make_unique<Batch>();
    return true;
}

bool Server::endBatch() {
    if(m_batch) {
        const auto targets = {TargetSocket::Ui, TargetSocket::Extension};
        for(const auto target : targets) {
            auto str = m_batch->dump(target);
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
            if(!send(TargetSocket::Ui, std::move(str), false))
                return false;
#ifdef PULL_MODE                
        } else {
            const auto pull = addPulled(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add batch pull", str.size(), pull);
            if(!m_broadcaster->send(target, obj.dump()))
                return false;
        }
#endif      
        }  
        m_batch.reset();
    }
    return true;
}

#ifdef PULL_MODE   
    int Server::addPulled(DataType type, std::string_view data) {
        ++m_pulledId;
        m_pulled.emplace(std::to_string(m_pulledId), std::pair<DataType, std::string> {type, std::string(data)});
        return m_pulledId;  
    }
#endif


bool Server::send(TargetSocket target, Server::Value&& value, bool batchable) {
    if(batchable && m_batch) {
        m_batch->push_back(target, std::move(value));
    } else {
            auto str = value.dump();
#ifdef PULL_MODE        
        if(str.size() < WS_MAX_LEN) {
#endif            
    if (!broadcaster().send_text(target, std::move(str)))
        return false;
#ifdef PULL_MODE               
    This is not working - but keep here as a reference if pull mode want to be re-enabled 
        } else {
            const auto pull = ed(DataType::Json, str);
            const json obj = {{"type", "pull_json"}, {"id", pull}};
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add text pull", str.size(), pull);
            if(!broadcaster().send(obj.dump(), is_ext))
                   return false;
        }
#endif
    }
    return true;
}

bool Server::send(Gempyre::DataPtr&& ptr, bool droppable) {
#ifdef PULL_MODE    
    if(len < WS_MAX_LEN) {
#endif        
        if (!broadcaster().send_bin(std::move(ptr), droppable))
            return false;
#ifdef PULL_MODE            
    } else {
        const auto pull = addPulled(DataType::Bin, {data, len});
        const json obj = {{"type", "pull_binary"}, {"id", pull}};
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "add bin pull", len, pull);
        if(!m_broadcaster->send(obj.dump()))
            return false;
    }
#endif    
    return true;
}

void Server::flush() {
    broadcaster().flush();
}
