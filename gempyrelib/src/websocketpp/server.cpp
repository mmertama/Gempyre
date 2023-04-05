#include "server.h"
#include "wspp_server.h"

using namespace Gempyre;


WSPP_Server::WSPP_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase) : Server{port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, queryIdBase} {
           }


WSPP_Server::~WSPP_Server() {
}

bool WSPP_Server::isJoinable() const {}
bool WSPP_Server::isRunning() const {}  
bool WSPP_Server::isConnected() const {}
bool WSPP_Server::retryStart() {}
void WSPP_Server::close(bool wait) {}
bool WSPP_Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {}
bool WSPP_Server::send(const Gempyre::Data& data) {}
bool WSPP_Server::beginBatch() {}
bool WSPP_Server::endBatch() {}

std::unique_ptr<Server> Gempyre::create_server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int querIdBase) {
                return std::unique_ptr<Server>(new WSPP_Server(port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, querIdBase));
           }

