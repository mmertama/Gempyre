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