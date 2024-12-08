#ifdef __GNUC__ // cmake suppression not work well with mingw
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "lws_server.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "gempyre_utils.h"
#include <nlohmann/json.hpp>
#include <cstring> //memcpy

#ifdef WINDOWS_OS
#ifndef __GNUC__
#pragma comment(lib, "Ws2_32.lib")
#endif
#endif

using namespace Gempyre;


static inline
unsigned get_error_code(void* in, size_t len) {
     auto reason_buffer = reinterpret_cast<unsigned char*>(in);
     // The close code is a 2-byte value at the start of the reason buffer
     return len >= 2 ? (
          static_cast<unsigned>(reason_buffer[0] << 8) |
          static_cast<unsigned>(reason_buffer[1])) : 1005U;        
}


class Gempyre::SendBuffer {
     public:
          SendBuffer() : m_pos{m_data.end()} {
          }
          SendBuffer(const SendBuffer&) = delete;
          SendBuffer& operator=(const SendBuffer&) = delete;
          void apply(std::string_view data/*, std::string_view mime*/) {
               if (m_data.size() < LWS_PRE + data.size())
                    m_data.resize(LWS_PRE + data.size());
               m_pos = m_data.begin() + LWS_PRE;
               assert(std::distance(m_data.begin(), m_pos) == LWS_PRE);
               GempyreUtils::log(GempyreUtils::LogLevel::Debug, "SendBuffer", m_data.size(), data.size(), LWS_PRE);
               assert((m_data.size() - LWS_PRE) >= data.size());     
               std::copy(data.begin(), data.end(), m_pos); 
          }

          bool empty() const {
               return m_pos == m_data.end();
          }

          uint8_t* data() {
               auto pos = reinterpret_cast<uint8_t*>(&(*m_pos));
               return pos;
          }

          bool end() const {
               return empty();
          }

          void commit(int size) {
               assert(size >= 0);
               m_pos += size;
               if (empty()) {
                    clear();
               }
               assert(m_pos <= m_data.end());
          }

          size_t size() const {
               return static_cast<size_t>(std::distance(static_cast<std::string::const_iterator>(m_pos), m_data.end()));
          }

          void clear() {
               m_data.clear();
               m_pos = m_data.end();
          }

     private:
          std::string m_data = {};
          std::string::iterator m_pos = {};     
};

std::optional<std::string_view> LWS_Server::match(std::string_view prefix, std::string_view param) const {
     if(param.empty()) {
          return std::nullopt;
     }
     for (auto i = 0U; i < prefix.size(); ++i) {
          if (prefix[i] == ':') {
               return param.substr(i + 1);
          }
          if (i >= param.size() || prefix[i] != param[i])
               return std::nullopt;
     }
     return std::nullopt;
}

 std::string LWS_Server::parse_query(std::string_view query) const {
     std::string fullPath;
     if(!query.empty()) {
          const auto queries = GempyreUtils::split<std::vector<std::string>>(query, '&');
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

bool LWS_Server::write_http_header(lws* wsi, std::string_view mime_type, size_t size) {
     unsigned char buffer[LWS_PRE + 2048];
     unsigned char* p = &buffer[LWS_PRE];
     unsigned char* start = p;
     unsigned char *end = &buffer[sizeof(buffer)];
     if (lws_add_http_common_headers(wsi, HTTP_STATUS_OK, mime_type.data(), size, &p, end)) {
          return false;
     }
     if (lws_finalize_write_http_header(wsi, start, &p, end)) {
          return false;
     }
     return true;
}

bool LWS_Server::get_http(lws* wsi, std::string_view get_param) {
     assert(m_send_buffers.find(wsi) != m_send_buffers.end());
    // assert(m_send_buffers.at(wsi)->empty());
#ifdef PULL_MODE       
     const auto id = match("/data/:id", get_param);
     if (id) {
          const auto it = m_pulled.find(*id);
          if(it == m_pulled.end()) {
               return false;
          }
          const auto mime_type = std::get<DataType>(it->second) == DataType::Json ? "application/json" : "application/octet-stream";
          m_send_buffer->apply(std::move(it->second), std::string(mime_type));
          m_pulled.erase(it);
          return true;
     }
#endif
     auto serverData = m_onGet(get_param); // is it would be just an url     
     if(serverData.has_value()) {
          GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get:", serverData->size());
          const auto mime_type = Server::fileToMime(get_param);
          if (!write_http_header(wsi, mime_type, serverData->size()))
               return false;
          m_send_buffers.at(wsi)->apply(*serverData);
          return true;
     }
     // this probably could be replaced with lws_serve_http_file, tbd if works ok TODO
     GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "server get does an file query");
     auto fullPath = m_onGet(get_param); // is it would be just an url
     if(fullPath && GempyreUtils::file_exists(*fullPath)) {
          auto mime_type = Server::fileToMime(*fullPath);
          auto data = GempyreUtils::slurp(*fullPath);
          if(!data.empty()) {
               if (!write_http_header(wsi, mime_type, data.size()))
                    return false;
               m_send_buffers.at(wsi)->apply(data);
               return true;
          } else {
               GempyreUtils::log(GempyreUtils::LogLevel::Error, "path:", *fullPath, "Cannot read"); 
          }
     } else {
          GempyreUtils::log(GempyreUtils::LogLevel::Warning, "path:", fullPath ? *fullPath : ("N/A, param: " + std::string{get_param}), "Not found");
     }   
        return false;
     }

void LWS_Server::append_socket(lws* wsi) {
     auto ws = std::make_unique<LWS_Socket>(wsi);
     assert(wsi);
     assert(ws);
     m_broadcaster->append(ws.get());
     m_sockets.emplace(wsi, std::move(ws));
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "LWS_CALLBACK_ESTABLISHED");
}

bool LWS_Server::remove_socket(lws* wsi, unsigned code) {
     auto it = m_sockets.find(wsi);
     m_broadcaster->remove(it->second.get());
     m_sockets.erase(it);
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "LWS_CALLBACK_CLOSED");

     if(code != 1001 && code != 1006 && code != 1005) {  //browser window closed
          if(code == 1000 || (/*code != 1005 &&*/ code >= 1002 && code <= 1015)  || (code >= 3000 && code <= 3999) || (code >= 4000 && code <= 4999)) {
               GempyreUtils::log(GempyreUtils::LogLevel::Error, "WS", "closed on error", code);
          } else if(code != 0) {
               GempyreUtils::log(GempyreUtils::LogLevel::Debug, "WS", "Non closing error", code);
               return true;
          }
     }

     m_onClose(CloseStatus::CLOSE, static_cast<int>(code));
     return false;
}


bool LWS_Server::received(lws* wsi, std::string_view msg) {
     switch(messageHandler(msg)) {
          case MessageReply::DoNothing:
               if(m_do_close) {
                    return false; // close connection
               }
               break;
          case MessageReply::AddUiSocket:
               m_uiready = true;
               assert(wsi);
               assert(m_sockets.find(wsi) != m_sockets.end());
               m_broadcaster->setType(m_sockets[wsi].get(), TargetSocket::Ui);
               assert(m_onOpen);
               m_onOpen();
               GempyreUtils::log(GempyreUtils::LogLevel::Debug, "onOpen Called");
               break;
          case MessageReply::AddExtensionSocket:
               assert(wsi);
               assert(m_sockets.find(wsi) != m_sockets.end());
               m_broadcaster->setType(m_sockets[wsi].get(), TargetSocket::Extension);
               break;
          }
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "LWS_CALLBACK_RECEIVE", msg);
     return true;
}

size_t LWS_Server::on_write(lws* wsi) {
     auto ws = m_sockets[wsi].get();
     if(ws->empty())
          return 0;
     const auto& [type, ptr, sz] = ws->front();
     const auto m = lws_write(wsi, const_cast<unsigned char*>(ptr), sz, type); //+ 1 is for null
     if (m < static_cast<int>(sz)) {
          lwsl_err("sending message failed: %d\n", m);
          return 0;
     }
     ws->shift();
     if (!ws->empty())
          lws_callback_on_writable(wsi); 
     return sz;
}


int LWS_Server::ws_callback(lws* wsi, lws_callback_reasons reason, void* /*user*/, void *in, size_t len) {
     auto self = static_cast<LWS_Server*>(lws_context_user(lws_get_context(wsi)));
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "wsCallback", reason);
     switch (reason) {
     case LWS_CALLBACK_CLOSED:
          if(!self->remove_socket(wsi, get_error_code(in, len)))
               return -1;
          break;     
     case LWS_CALLBACK_ESTABLISHED:
          self->append_socket(wsi);
          break;
     case LWS_CALLBACK_SERVER_WRITEABLE:
          self->on_write(wsi);
          break;
     case LWS_CALLBACK_RECEIVE: 
          if (!self->received(wsi, std::string_view{static_cast<char*>(in), len}))
               return -1; // exit
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
static auto lwsToken(std::string_view sv) {
     return reinterpret_cast<const unsigned char*>(sv.data()); // not \0 terminated
}
*/

int LWS_Server::on_http(lws *wsi, void* in) {

     const std::string_view params{static_cast<const char*>(in)};
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "http-get", params);
     if (m_send_buffers.find(wsi) == m_send_buffers.end()) {
          m_send_buffers.emplace(wsi, std::make_unique<SendBuffer>());
     }
     if(get_http(wsi, params)) {
          lws_callback_on_writable(wsi); 
     } else {
          lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, nullptr);
     }
     return 0;     
}

int LWS_Server::on_http_write(lws *wsi) {
     auto& buffer = m_send_buffers.at(wsi);
     const auto protocol = buffer->end() ? LWS_WRITE_HTTP_FINAL : LWS_WRITE_HTTP;
     if (LWS_WRITE_HTTP_FINAL != protocol) {
          auto ptr = buffer->data();
          const auto sz = buffer->size();
          const auto written = lws_write(wsi, ptr, sz, protocol);
          if (written != static_cast<int>(sz)) {
               GempyreUtils::log(GempyreUtils::LogLevel::Error, "http-get write failed", written);
               return 1;
          }
          buffer->commit(written);
          lws_callback_on_writable(wsi);
     } else {
          if (lws_http_transaction_completed(wsi))
               return -1;
          buffer->clear();          
     }
     return 0;
}


int LWS_Server::http_callback(lws *wsi, enum lws_callback_reasons reason, void* /*user*/, void* in, size_t /*len*/)
{
     auto self = static_cast<LWS_Server*>(lws_context_user(lws_get_context(wsi)));
     
     switch (reason) {
     case LWS_CALLBACK_HTTP:
          return self->on_http(wsi, in);
     case LWS_CALLBACK_HTTP_WRITEABLE:
          return self->on_http_write(wsi);     
     default:
          break;
     }
     return 0;
}


static void set_lws_log_level() {
     int logs = LLL_USER | LLL_NOTICE
			/* for LLL_ verbosity above NOTICE to be built into lws,
			 * lws must have been configured and built with
			 * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
			/* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
			/* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
#ifdef LWS_DEBUG         
			 | LLL_WARN | LLL_DEBUG  
#endif
#ifndef SUPRESS_WS_ERRORS
     | LLL_ERR
#endif
     ;
     lws_set_log_level(logs, [](int level, const char* line) {
          GempyreUtils::LogLevel lvl = GempyreUtils::LogLevel::Debug;
          switch(level) {
               case LLL_ERR: lvl = GempyreUtils::LogLevel::Error; break;
               case LLL_NOTICE:
               case LLL_WARN: lvl = GempyreUtils::LogLevel::Warning; break;
               case LLL_INFO: lvl = GempyreUtils::LogLevel::Info; break;
               default:
                    break;
          }
          GempyreUtils::log(lvl, "libwebsocket:", line);
     });          
}

#if 0
static lws_retry_bo retry{
	0,	     /* base delay in ms */
	0,        /* entries in table */
	0,        /* max retries to conceal */
	3,        /* idle before PING issued */
	10,       /* idle before hangup conn */
	0		/* % additional random jitter */
};
#endif

LWS_Server::LWS_Server(unsigned int port,
     const std::string& rootFolder,
     Server::OpenFunction&& onOpen,
     Server::MessageFunction&& onMessage,
     Server::CloseFunction&& onClose,
     Server::GetFunction&& onGet,
     Server::ListenFunction&& onListen,
     int queryIdBase,
     Server::ResendRequest&& resendRequest) :
Server{port, rootFolder, std::move(onOpen), std::move(onMessage), std::move(onClose), std::move(onGet), std::move(onListen), queryIdBase},
m_broadcaster{std::make_unique<LWS_Broadcaster>([resendRequest](LWS_Socket*, LWS_Socket::SendStatus) {
     resendRequest();
})} {
     m_broadcaster->set_loop(&m_loop);
     std::atomic_bool thread_started = false;

     m_loop = std::thread([this, &thread_started] {
          
          lws_context_creation_info info{};

          const lws_protocols gempyre_ws { 
               "gempyre",
               LWS_Server::ws_callback,
               0,
               8192,
               0,
               nullptr,
               0
          };

          const lws_protocols gempyre_http {
               "http-only",                  // name
               LWS_Server::http_callback,     // cb
               0,                            // per_session_data_size
               0,                            // rx_buffer_size
               0,                            // id
               nullptr,                      // user
               0                             // tx_packet_size
          };

          const lws_protocols protocols[] = {
               gempyre_ws,
               gempyre_http,
               LWS_PROTOCOL_LIST_TERM
          };

          const auto cwd = GempyreUtils::working_dir();

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


          info.port = static_cast<int>(m_port);
          info.protocols = protocols;
          info.mounts = &mount;
          info.error_document_404 = "/404.html";
          info.user = this;
          //info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

          set_lws_log_level();
          
          auto context = lws_create_context(&info);
          if(!context) {
               GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Init failed");
               thread_started = true;
               return;
          }

          m_loop.set_context(context);

     
          /*auto *vhost = lws_create_vhost(m_context, &info);
          if (!vhost) {
               GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "Generating vhost failed");
               lws_context_destroy(m_context);
               return;
          }*/

          m_running = true;
          thread_started = true;

          if(!m_onListen(m_port)) {
               m_running = false;
          }

          while (m_running && lws_service(context, 0) >= 0) {
               m_loop.execute();    
          }

          m_running = false;
          lws_context_destroy(context);
          });

     while (!thread_started) {
        std::this_thread::yield();  // Yield to avoid hogging the CPU
    }
}

void LWS_Loop::defer(std::function<void ()>&& f) {
     std::lock_guard<std::mutex> guard(m_mutex);
     m_deferred.push_back(std::move(f));
     wakeup();
}

void LWS_Loop::execute() {
     decltype(m_deferred) copy_of_deferred;
     do {
          std::lock_guard g{m_mutex};
          copy_of_deferred = m_deferred;
          m_deferred.clear();
     } while (false);
     for (auto&& f : copy_of_deferred) {
          f();
     }
}

bool LWS_Loop::valid() const {
     return m_fut.joinable();
}

void LWS_Loop::join() {
     m_fut.join();
}

LWS_Loop& LWS_Loop::operator=(std::thread&& fut) {
     m_fut = std::move(fut);
     return *this;
}

void LWS_Loop::set_context(lws_context* context) {
     m_context = context;
}

void LWS_Loop::wakeup() {
     lws_cancel_service(m_context); 
}

LWS_Server::~LWS_Server() {
     close(true);
     assert(!m_broadcaster ||  m_broadcaster->empty());
}

bool LWS_Server::isJoinable() const {
     return m_loop.valid();
}

bool LWS_Server::isUiReady() const {
    return m_uiready && isRunning();
}

bool LWS_Server::isRunning() const {
     return m_running && isJoinable();
}  

bool LWS_Server::isConnected() const {
     return isRunning() && !m_broadcaster->empty();
}
     
bool LWS_Server::retryStart() {
     std::abort();
}


void LWS_Server::close(bool wait) {
     m_broadcaster->close();
     m_do_close = true;
     m_running = false;
     m_loop.wakeup();
     if (wait && isJoinable()) {
          m_loop.join();
         // std::this_thread::sleep_for(100ms);
     }
     GempyreUtils::log(GempyreUtils::LogLevel::Debug,
      "closed", m_loop.valid());

}

/*
bool LWS_Server::send(TargetSocket target, Server::Value&& value) {
     assert(isConnected());*/
     //GempyreUtils::log(GempyreUtils::LogLevel::Debug, "send!", k, v);
     /*
          // https://libwebsockets.org/lws-api-doc-master/html/md_README.coding.html
     Only send data when socket writeable
     You should only send data on a websocket connection from the user callback LWS_CALLBACK_SERVER_WRITEABLE (
     or LWS_CALLBACK_CLIENT_WRITEABLE for clients).

     If you want to send something, do not just send it but request a callback when the socket is writeable using
     lws_callback_on_writable(context, wsi) for a specific wsi,
     or lws_callback_on_writable_all_protocol(protocol) for all connections using that protocol to get a callback when next writeable.

     Usually you will get called back immediately next time around the service loop,
     but if your peer is slow or temporarily inactive the callback will be delayed accordingly.
     Generating what to write and sending it should be done in the ...WRITEABLE callback.

*/

     // Here we add data to queue and, if it is not a batch, kick writeable request
/*     return true;
     }
*/
/*
bool LWS_Server::send(const Gempyre::Data& data)
bool LWS_Server::beginBatch()
bool LWS_Server::endBatch()
*/

BroadcasterBase& LWS_Server::broadcaster() {
    return *m_broadcaster;
}

LWS_Socket::SendStatus LWS_Server::send_bin(LWS_Socket* s, std::string_view bin) {
     const auto status = s->append(bin, LWS_Socket::BIN);
     return status;
}

LWS_Socket::SendStatus LWS_Server::send_text(LWS_Socket* s, std::string_view text) {
     const auto status = s->append(text, LWS_Socket::TEXT);
     return status;
}

bool LWS_Server::has_backpressure(LWS_Socket* s, size_t /*len*/) {
     return s->is_full();
}

void LWS_Socket::close() {
     GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Socket close request");
     lws_set_timeout(m_ws,  PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
}

std::unique_ptr<Server> Gempyre::create_server(unsigned int port,
           const std::string& rootFolder,
           Server::OpenFunction&& onOpen,
           Server::MessageFunction&& onMessage,
           Server::CloseFunction&& onClose,
           Server::GetFunction&& onGet,
           Server::ListenFunction&& onListen,
           int querIdBase,
           Server::ResendRequest&& request) {
                return std::unique_ptr<Server>(new LWS_Server(port, rootFolder, std::move(onOpen), std::move(onMessage), std::move(onClose), std::move(onGet), std::move(onListen), querIdBase, std::move(request)));
           }

