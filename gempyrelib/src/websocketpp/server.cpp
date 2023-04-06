#include "server.h"
#include "wspp_server.h"
#include <future>

#include<websocketpp/config/core.hpp>
#include<websocketpp/config/minimal_server.hpp>

using namespace Gempyre;

struct GempyreServerConfig : public websocketpp::config::minimal_server {

};


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

bool WSPP_Server::isJoinable() const {std::abort();}
bool WSPP_Server::isRunning() const {std::abort();}  
bool WSPP_Server::isConnected() const {std::abort();}
bool WSPP_Server::retryStart() {std::abort();}
void WSPP_Server::close(bool wait) {std::abort();}
bool WSPP_Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {std::abort();}
bool WSPP_Server::send(const Gempyre::Data& data) {std::abort();}
bool WSPP_Server::beginBatch() {std::abort();}
bool WSPP_Server::endBatch() {std::abort();}

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

