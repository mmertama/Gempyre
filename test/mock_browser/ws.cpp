#include "ws.hpp"
#include "libwebsockets.h"

#include<cassert>
#include <set>

constexpr int BUFFER_SIZE{(1024 * 16) - 256};
constexpr int RETRIES{10};

static constexpr uint32_t backoff_ms[] = { 1000, 2000, 3000, 4000, 5000 };

static const lws_retry_bo_t retry = {
    backoff_ms,
    LWS_ARRAY_SIZE(backoff_ms),
    LWS_ARRAY_SIZE(backoff_ms),
    3,  // force PINGs after secs idle
    10, // hangup after secs idle
    20  //jitter
};



#define LWS_PROTOCOL_LIST_TERM { NULL, NULL, 0, 0, 0, NULL, 0 }

struct Connection {
    std::string address;
    int port;
    std::string protocol;
    int interrupted;
    unsigned char msg[BUFFER_SIZE];
    int msg_size;
    websocket::App* extension;
    std::set<lws*>* m_connections; // if not ptr, not a POD, offset_of wont work
    uint16_t retries = 10;
    lws_sorted_usec_list_t sul;
    lws_context* context;
 //   std::function<void (const websocket::Extension::Info&)>* connectedCall;
    Connection(const std::string& address, int port, const std::string& protocol, websocket::App* extension) :
        address{address},
        port{port},
        protocol{protocol},
        interrupted{0},
        msg{'\0'},
        msg_size{0},
        extension{extension},
        m_connections{new std::set<lws*>},
        retries{RETRIES},
        sul{},
        context{nullptr}
    //    connectedCall{nullptr}
    {}
    ~Connection(){
        delete m_connections;
    }
};


static void
connect_client(lws_sorted_usec_list_t *sul) {
    lwsl_user("connect_client");

    Connection* c = lws_container_of(sul, struct Connection, sul);
    assert(c);
    struct lws_client_connect_info i;

    memset(&i, 0, sizeof(i));

    i.context = c->context;
    i.port = c->port;
    i.address = c->address.c_str()/* "127.0.0.1"*/;
    i.path = "/gempyre";
    i.host = i.address;
    i.origin = i.address;
    i.ssl_connection = 0;
    i.protocol = c->protocol.c_str();
    i.local_protocol_name = c->protocol.c_str();
    i.userdata = c;

    if (!lws_client_connect_via_info(&i)) {
        if (lws_retry_sul_schedule(c->context, 0, sul, &retry,
                               connect_client, &c->retries)) {
            lwsl_err("%s: connection attempts exhausted\n", __func__);
            c->interrupted = 1;
        }
    }
}

static void
send_message(Connection* c, const std::string& str) {
    std::fill_n(c->msg, LWS_PRE + str.size() + 1U, '\0');
    std::copy(str.begin(), str.end(), c->msg + LWS_PRE);
    c->msg_size = static_cast<int>(str.size()) + 1;
}

static int
callback(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len)
{
    auto context = lws_get_context(wsi);
    auto mco = static_cast<Connection*> (lws_context_user(context));
    int m = 0;
    int pj;
    switch (reason) {

    case LWS_CALLBACK_PROTOCOL_INIT:
        lws_sul_schedule(context, 0, &mco->sul, connect_client, 1);
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        if (lws_retry_sul_schedule(mco->context, 0, &mco->sul, &retry,
                               connect_client, &mco->retries)) {
            lwsl_err("CLIENT_CONNECTION_ERROR: %s\n", in ? (char *)in :
                "(null)");
            mco->interrupted = 1;
        }
        break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        lws_callback_on_writable(wsi);
        lwsl_user("%s: established connection, wsi = %p\n",
                __func__, wsi);
        mco->m_connections->emplace(wsi);
        send_message(mco, mco->extension->on_status(websocket::Status::Established));
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        lwsl_user("%s: CLOSED\n", __func__);
        mco->interrupted = 1;
        mco->m_connections->erase(wsi);
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if(mco->msg_size == 0)
            return 0;
        lwsl_user("%s: WRITEABLE\n", __func__);
        m = lws_write(wsi, mco->msg + LWS_PRE, mco->msg_size, LWS_WRITE_TEXT); //+ 1 is for null
        if (m < mco->msg_size) {
            lwsl_err("sending message failed: %d\n", m);
            return 0;
        }
        mco->msg_size = 0;
        break;

    case LWS_CALLBACK_TIMER:
        // Let the main loop know we want to send another message to the
        // server
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        pj =  mco->extension->received(static_cast<const char*>(in));
        if(pj < 0) {
            lwsl_notice("Unexpexted message, %d : %s", pj, static_cast<const char*>(in));
        } else if (pj > 0) {
            mco->interrupted = 1;
        }
        break;
    case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        lwsl_notice("server initiated connection close: len = %lu, "
                "in = %s\n", (unsigned long)len, (char*)in);
        mco->interrupted = 1;
        return 0;

    default:
        break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
        { "lws wrapper", callback, 0 , BUFFER_SIZE, 0, NULL, BUFFER_SIZE },
        LWS_PROTOCOL_LIST_TERM
};


int websocket::start_ws(const std::string& address, int port, const std::string& protocol, websocket::App& ext)
{
    int logs =
            LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    logs |= LLL_INFO | LLL_DEBUG;
    lws_set_log_level(logs, websocket::debug_print);

    lwsl_user("start_ws");

    assert(protocol == "gempyre"); // only supported now

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */

    info.options = 0;
    info.port = CONTEXT_PORT_NO_LISTEN; /* we do not run any server */
    info.protocols = protocols;

    auto mco = std::unique_ptr<Connection>(new Connection{address,
                                                          port,
                                                          protocol,
                                                          &ext}
                                           );
    info.user = mco.get();

    mco->address = address;
    mco->port = port;
    mco->protocol = protocol;

    info.fd_limit_per_thread = (unsigned int)(1 + 1 + 1); // 1 client

    mco->context  = lws_create_context(&info);
    if (!mco->context) {
        lwsl_err("lws init failed\n");
        return 1;
    }

    ext.exit = [&mco]() {
        mco->interrupted = 1;
        lws_cancel_service(mco->context);
    };

    ext.send_message = [&mco](const std::string& msg) {
         if(msg.size() > BUFFER_SIZE)
             return false;
         send_message(mco.get(), msg);
         for(const auto& wsi : *mco->m_connections)
            lws_callback_on_writable(wsi);
         return !mco->m_connections->empty();
    };

    websocket::debug_print(LLL_NOTICE, "Startup!");

    int n = 0;
    while (n >= 0 && mco->interrupted == 0)
        n = lws_service(mco->context, 0);

    ext.exit = nullptr;

    lwsl_notice("%s: exiting service loop. n = %d, interrupted = %d\n",
            __func__, n, mco->interrupted);

    lws_context_destroy(mco->context);

    return 0;
}
