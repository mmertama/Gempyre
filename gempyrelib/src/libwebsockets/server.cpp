#include "gempyre_utils.h"
#include "server.h"
#include "lws_server.h"

#include<libwebsockets.hxx>

/*#include<websocketpp/config/core.hpp>
#include<websocketpp/config/minimal_server.hpp>


struct GempyreServerConfig : public websocketpp::config::minimal_server {

};
*/

using namespace Gempyre;

static void set_lws_log_level() {
     int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			/* | LLL_DEBUG */;
     lws_set_log_level(logs, NULL);          
}

const lws_protocols gempyre_ws { 
     "gempyre",
     ws_cb,
     sizeof(struct per_session_data__minimal),
     128,
     0,
     nullptr,
     0
};

const const lws_protocols gempyre_http {
     "http",   // name
     http_cb,  // cb
     0,        // per_session_data_size
     0,        // rx_buffer_size
     0,        // id
     nullptr,  // user
     0         // tx_packet_size
}

const lws_protocols protocols[] = {
	gempyre_http,
	gempyre_ws,
	LWS_PROTOCOL_LIST_TERM
};

static lws_retry_bo retry{
	0,	     /* base delay in ms */
	0,        /* entries in table */
	0         /* max retries to conceal */
	3         /* idle before PING issued */
	10,       /* idle before hangup conn */
	0		/* % additional random jitter */
};

LWS_Server::LWS_Server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int queryIdBase) : Server{port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, queryIdBase} {
                 m_loop = std::async(std::launch::async, [this] {
                    lws_context_creation_info info{};

                    const struct lws_http_mount mount = {
	               /* .mount_next */		     nullptr,		/* linked-list "next" */
	               /* .mountpoint */		     "/",		/* mountpoint URL */
	               /* .origin */			     "./mount-origin",  /* serve from dir */
	               /* .def */			     rootFolder.c_str(),	/* default filename */
	               /* .protocol */		     nullptr,
	               /* .cgienv */			     nullptr,
	               /* .extra_mimetypes */		nullptr,
	               /* .interpret */		     nullptr,
	               /* .cgi_timeout */		     0,
	               /* .cache_max_age */		0,
	               /* .auth_mask */		     0,
	               /* .cache_reusable */		0,
	               /* .cache_revalidate */		0,
	               /* .cache_intermediaries */	0,
	               /* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
	               /* .mountpoint_len */		1,		/* char count */
	               /* .basic_auth_login_file */	nullptr,
                    };

                    info.port = port;
	               info.mounts = &mount;
	               info.error_document_404 = "/404.html";
	               info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;


                    set_lws_log_level();
                    
                    m_context = lws_create_context(&info);
                    if(!m_context) {
                         GempyreUtils::log(GempyreUtils::LogError, "Init failed");
                         return;
                    }

                    m_running = true;

                    while (m_running && ws_service(context, 0) >= 0) {
                    }

                    m_running = false;
                    lws_context_default_loop_run_destroy(cx);
                });
           }


LWS_Server::~LWS_Server() {
}

bool LWS_Server::isJoinable() const {return m_loop.valid();}
bool LWS_Server::isRunning() const {return m_running;}  
bool LWS_Server::isConnected() const {std::abort();}
bool LWS_Server::retryStart() {std::abort();}
void LWS_Server::close(bool wait) {std::abort();}
bool LWS_Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {std::abort();}
bool LWS_Server::send(const Gempyre::Data& data) {std::abort();}
bool LWS_Server::beginBatch() {std::abort();}
bool LWS_Server::endBatch() {std::abort();}

std::unique_ptr<Server> Gempyre::create_server(unsigned int port,
           const std::string& rootFolder,
           const Server::OpenFunction& onOpen,
           const Server::MessageFunction& onMessage,
           const Server::CloseFunction& onClose,
           const Server::GetFunction& onGet,
           const Server::ListenFunction& onListen,
           int querIdBase) {
                return std::unique_ptr<Server>(new LWS_Server(port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, querIdBase));
           }

