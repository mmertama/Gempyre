#include "gempyre_utils.h"
#include "server.h"

#include <cstring> //memcpy

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
extern "C" {
     #include "lws_server.h"
}
#pragma GCC diagnostic pop

using namespace Gempyre;

class Gempyre::SendBuffer {
     public:
          SendBuffer() : m_mime{}, m_data{} {
          }
          SendBuffer(const SendBuffer&) = delete;
          SendBuffer& operator=(const SendBuffer&) = delete;
          void apply(std::string&& data, std::string&& mime) {
               assert(empty());
               m_mime = std::move(mime);
               m_data = std::move(data);
               m_pos = m_data.begin(); 
          }

          bool empty() const {
               return m_mime.empty() && m_data.empty();
          }

          uint8_t* copy_to(uint8_t* ptr, uint8_t* end) {
               const auto distance_to_end = std::distance(m_pos, m_data.end());
               const auto delta = std::min((end - ptr), distance_to_end);
               auto pos = &(*m_pos);
               std::memcpy(ptr, pos, delta);
               m_pos += delta;
               return ptr + delta;
          }

          bool end() const {
               GempyreUtils::log(GempyreUtils::LogLevel::Debug, "to read", (int64_t) (m_data.end() - m_pos));
               return m_pos == m_data.end();
          }

          const std::string& mime() const {
               return m_mime;
          }

          const auto size() const {
               return m_data.size();
          }

          void clear() {
               m_mime.clear();
               m_data.clear();
          }

     private:
          std::string::iterator m_pos = {};
          std::string m_mime  = {};
          std::string m_data = {};     
};

 std::string LWS_Server::parseQuery(const std::string_view query) const {
     std::string fullPath;
     if(!query.empty()) {
          const auto queries = GempyreUtils::split<std::vector<std::string>>(std::string(query), '&');
          for(const auto& q : queries) {
               const auto query_list = GempyreUtils::split<std::vector<std::string>>(q, '=');
               if(query_list.size() == 2) {
                    if(query_list[0] == "file") {
                         fullPath = GempyreUtils::unhexify(query_list[1]);
                    }
               }
          }
     }
     if(fullPath.empty()) {
          fullPath = m_rootFolder;
          fullPath.append(query);
     }
     return fullPath;
}

bool LWS_Server::get(const std::string_view get_param) const {
     assert(m_send_buffer->empty());
     auto serverData = m_onGet(get_param); // is it would be just an url
     if(serverData.has_value()) {
          GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get:", serverData->size());
          const auto mime_type = Server::fileToMime(get_param);
          m_send_buffer->apply(std::move(*serverData), std::string(mime_type));
          return true;
     }
     // this propably could be replaced with lws_serve_http_file, tbd if works ok TODO
     GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get does an file query");
     const auto fullPath = parseQuery(get_param);
     if(GempyreUtils::file_exists(fullPath)) {
          auto mime_type = Server::fileToMime(fullPath);
          auto data = GempyreUtils::slurp(fullPath);
          if(!data.empty()) {
               m_send_buffer->apply(std::move(data), std::string(mime_type));
               return true;
          } else {
               GempyreUtils::log(GempyreUtils::LogLevel::Error, "path:", fullPath, "Cannot read"); 
          }
     } else {
          GempyreUtils::log(GempyreUtils::LogLevel::Error, "path:", fullPath, "Not found");
     }   
        return false;
     }

int LWS_Server::wsCallback(lws *wsi, lws_callback_reasons reason, void *user, void *in, size_t len)
{
     auto self = static_cast<LWS_Server*>(lws_context_user(lws_get_context(wsi)));
     switch (reason)
     {
     case LWS_CALLBACK_ESTABLISHED:
          self->m_connected = true;
          break;
     
     default:
          break;
     }
	return 0;
}

/*
// before u ask: constexpr is not reinterpret_cast
static auto lwsToken(const char* c_str) {
     return reinterpret_cast<const unsigned char*>(c_str);
}

// before u ask: constexpr is not reinterpret_cast
static auto lwsToken(const std::string_view sv) {
     return reinterpret_cast<const unsigned char*>(sv.data()); // not \0 terminated
}
*/


int LWS_Server::httpCallback(lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
     auto self = static_cast<LWS_Server*>(lws_context_user(lws_get_context(wsi)));
     uint8_t buf[LWS_PRE + 2048];
     uint8_t* start = &buf[LWS_PRE];
     uint8_t* ptr = start;
	uint8_t* end = &buf[sizeof(buf) - 1];
     switch (reason) {
     case LWS_CALLBACK_HTTP: {
          const std::string_view get_params{static_cast<const char*>(in)};
          GempyreUtils::log(GempyreUtils::LogLevel::Debug, "http-get", get_params);
          if(self->get(get_params)) {
               const auto mime_type = self->m_send_buffer->mime();
               const auto size = self->m_send_buffer->size();
               if (lws_add_http_common_headers(wsi, HTTP_STATUS_OK, mime_type.c_str(), size, &ptr, end))
			     return 1;
               if (lws_finalize_write_http_header(wsi, start, &ptr, end))
                    return 1;
               lws_callback_on_writable(wsi); 
          } else {
               lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, nullptr);
          }
     } break;
     case LWS_CALLBACK_HTTP_WRITEABLE: {
          ptr = self->m_send_buffer->copy_to(ptr, end);
          const auto protocol = self->m_send_buffer->end() ? LWS_WRITE_HTTP_FINAL : LWS_WRITE_HTTP;
          if (lws_write(wsi, start, lws_ptr_diff_size_t(ptr, start), protocol) != lws_ptr_diff(ptr, start)) {
               GempyreUtils::log(GempyreUtils::LogLevel::Error, "http-get write failed");
          }
          if (protocol == LWS_WRITE_HTTP_FINAL) {
               if (lws_http_transaction_completed(wsi))
                    return -1;
               self->m_send_buffer->clear();
		} else
			lws_callback_on_writable(wsi);
     } break;
     default:
          break;
     }
     return 0;
}


static void set_lws_log_level() {
     int logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
			 | LLL_DEBUG ;
     lws_set_log_level(logs, NULL);          
}

static lws_retry_bo retry{
	0,	     /* base delay in ms */
	0,        /* entries in table */
	0,        /* max retries to conceal */
	3,        /* idle before PING issued */
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
           int queryIdBase) :
            Server{port, rootFolder, onOpen, onMessage, onClose, onGet, onListen, queryIdBase},
            m_send_buffer{std::make_unique<SendBuffer>()} {
               assert(m_send_buffer && m_send_buffer->empty());
               m_loop = std::async(std::launch::async, [this] {

                    lws_context_creation_info info{};

                    const lws_protocols gempyre_ws { 
                         "gempyre",
                         LWS_Server::wsCallback,
                         0,
                         128,
                         0,
                         nullptr,
                         0
                    };

                    const lws_protocols gempyre_http {
                         "http-only",                  // name
                         LWS_Server::httpCallback,     // cb
                         0,                            // per_session_data_size
                         0,                            // rx_buffer_size
                         0,                            // id
                         nullptr,                        // user
                         0                             // tx_packet_size
                    };

                    const lws_protocols protocols[] = {
                         gempyre_http,
                         gempyre_ws,
                         LWS_PROTOCOL_LIST_TERM
                    };

                    const auto cwd = GempyreUtils::working_dir();lws_protocol_init_vhost: 

                    const struct lws_http_mount mount = {
	               /* .mount_next */		     nullptr,		          /* linked-list "next" */
	               /* .mountpoint */		     "/",		               /* mountpoint URL */
	               /* .origin */			     cwd.c_str(),             /* serve from dir */
	               /* .def */			     m_rootFolder.c_str(),	     /* default filename */
	               /* .protocol */		     "http-only",
	               /* .cgienv */			     nullptr,
	               /* .extra_mimetypes */		nullptr,
	               /* .interpret */		     nullptr,
	               /* .cgi_timeout */		     0,
	               /* .cache_max_age */		0,
	               /* .auth_mask */		     0,
	               /* .cache_reusable */		0,
	               /* .cache_revalidate */		0,
	               /* .cache_intermediaries */	0,
	               /* .origin_protocol */		LWSMPRO_FILE,	          /* files in a dir */
	               /* .mountpoint_len */		1,		               /* char count */
	               /* .basic_auth_login_file */	nullptr,
                    };


                    info.port = m_port;
                    info.protocols = protocols;
	               info.mounts = &mount;
	               info.error_document_404 = "/404.html";
                    info.user = this;
	               //info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

                    set_lws_log_level();
                    
                    auto context = lws_create_context(&info);
                    if(!context) {
                         GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Init failed");
                         return;
                    }

                    
                    /*auto *vhost = lws_create_vhost(m_context, &info);
                    if (!vhost) {
                         GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Generatoring vhost failded");
                         lws_context_destroy(m_context);
                         return;
                    }*/

                    m_running = true;
                    if(!m_onListen(m_port)) {
                          m_running = false;
                    }

                    while (m_running && lws_service(context, 0) >= 0) {
                    }

                    m_running = false;
                    lws_context_destroy(context);
                });
           }


LWS_Server::~LWS_Server() {
}

bool LWS_Server::isJoinable() const {
     return m_loop.valid();
     }
bool LWS_Server::isRunning() const {
     assert(isJoinable());
     return m_running;
     }  
bool LWS_Server::isConnected() const {
     assert(isRunning());
     return m_connected;
     }
bool LWS_Server::retryStart() {std::abort();}
void LWS_Server::close(bool wait) {std::abort();}

bool LWS_Server::send(const std::unordered_map<std::string, std::string>& object, const std::any& values) {
     assert(isConnected());
     for(const auto& [k, v] : object) {
          GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send!", k, v);
     }
     //TODO lws_write
     //TODO break - IDs to identidy target: extension, ui or both
     //TODO break - any --> json
     return true;
     }

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

