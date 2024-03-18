#include <iostream>
#include <condition_variable>
#include <thread>
#include <sstream>
#include <future>
#include <cassert>

#include "semaphore.h"

#include "gempyre.h"
#include "server.h"
#include "gempyre_utils.h"
#include "eventqueue.h"

#include "base64.h"
#include "gempyre.js.h"

#include "idlist.h"
#include "timer.h"

#include "core.h"

#include "gempyre_internal.h"

using namespace std::chrono_literals;
using namespace Gempyre;


#ifdef ANDROID_OS
extern int androidLoadUi(const std::string&);
#endif

#define CHECK_FATAL(x) if(ec) {error(ec, merge(x, " at ", __LINE__)); return;}  std::cout << x << " - ok" << std::endl;

void Gempyre::set_debug(bool is_debug) {
    GempyreUtils::set_log_level(is_debug ? GempyreUtils::LogLevel::Debug : GempyreUtils::LogLevel::Error);
}


#ifndef ANDROID_OS
void Gempyre::setJNIENV(void*, void*) {
    GempyreUtils::log(GempyreUtils::LogLevel::Fatal, "setJNIENV should not be called within current OS");
}
#endif

#define STR(x) #x
#define TOSTRING(x) STR(x)


Ui::Filemap Ui::to_file_map(const std::vector<std::string>& filenames) {
    Ui::Filemap map;
    for(const auto& filename : filenames) {
        const auto bytes = GempyreUtils::slurp<Base64::Byte>(filename);
        const auto encoded = Base64::encode(bytes);
        const auto name = GempyreUtils::base_name(filename);
        map.emplace('/' + name, encoded);
    }
    return map;
}

std::tuple<int, int, int> Gempyre::version() {
    static_assert(TOSTRING(GEMPYRE_PROJECT_VERSION)[0], "GEMPYRE_PROJECT_VERSION not set");
    const auto c = GempyreUtils::split<std::vector<std::string>>(TOSTRING(GEMPYRE_PROJECT_VERSION), '.');
    return {GempyreUtils::convert<int>(c[0]), GempyreUtils::convert<int>(c[1]), GempyreUtils::convert<int>(c[2])};
}





/// Create UI using default ui app or gempyre.conf
Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       unsigned short port,
       const std::string& root) : Ui(filemap, indexHtml, port, root,
       {{GempyreUtils::log_level() >= GempyreUtils::LogLevel::Debug ? BROWSER_PARAMS_KEY : "","" }},
       WindowType::BROWSER){}


Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       const std::string& browser,
       const std::string& browser_params,
       unsigned short port,
       const std::string& root) : Ui(filemap, indexHtml, port, root,
        {
            {!browser.empty() ? BROWSER_KEY : "", browser},
            {!browser_params.empty() ? BROWSER_PARAMS_KEY : "", browser_params}
        },
        WindowType::AUTO) {}


static
std::unordered_map<std::string, std::string> merge(const std::unordered_map<std::string, std::string>& a, const std::unordered_map<std::string, std::string>& b) {
    std::unordered_map<std::string, std::string> result{a};
    for(const auto& kv : b) {
        result.emplace(kv);
    }
    return result;
}

Ui::Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& title,  int width, int height, unsigned flags,
            const std::unordered_map<std::string, std::string>& ui_params, unsigned short port, const std::string& root) : 
            Ui{filemap, indexHtml, port, root, merge({
            {!title.empty() ? TITLE_KEY : "", GempyreUtils::qq(title)},
            {flags != 0 ? FLAGS_KEY : "", std::to_string(flags)},
            {width > 0 ? WIDTH_KEY : "", std::to_string(width)},
            {height > 0 ? HEIGHT_KEY : "", std::to_string(height)},
            {GempyreUtils::log_level() >= GempyreUtils::LogLevel::Debug ? BROWSER_PARAMS_KEY : "","" }},ui_params),
            WindowType::PYTHON} {
            }


Ui::Ui(const Filemap& filemap,
       const std::string& indexHtml,
       unsigned short port,
       const std::string& root,
       const std::unordered_map<std::string, std::string>& parameters,
       WindowType windowType) :
    m_ui{std::make_unique<GempyreInternal>(
        this,
        filemap,
        indexHtml,
        port,
        root,
        parameters,
        windowType)} {
    GempyreUtils::init();
    // automatically try to set app icon if favicon is available
    const auto icon = resource("/favicon.ico");
    if(icon)
        set_application_icon(icon->data(), icon->size(), "ico");
}

// destoructor is slow due server thread close is slow (join) - to be fixed (?) when uwebsocket will be ditched
Ui::~Ui() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ui Destructor", m_ui ? m_ui->state_str() : "N/A");
    if(m_ui) {
        if(*m_ui == State::SUSPEND) {
            m_ui->do_exit();
        }
        else if(*m_ui == State::RUNNING) {
            exit();
        }
        else if (*m_ui != State::NOTSTARTED && *m_ui != State::EXIT) {
            GempyreUtils::log(GempyreUtils::LogLevel::Error, "Strange state", m_ui->state_str());
        }
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Ui Destroyed"); 
}


void Ui::close() {
    m_ui->add_request([this]() {
        return m_ui->send_to(Server::TargetSocket::All, {{"type", "close_request"}});
    });
}

void Ui::exit() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - start", m_ui->state_str());
    if (*m_ui == State::RUNNING) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - request", m_ui->state_str());
        if(! m_ui->is_running()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no run", m_ui->state_str());
            m_ui->set(State::EXIT);
            return;
        }
        if(! m_ui->is_connected()) {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - no connect", m_ui->state_str());
            m_ui->close_server();
            m_ui->set(State::EXIT);
            return;
        }
        m_ui->add_request([this]() {
            GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - send", m_ui->state_str());
            if(! m_ui->send_to(Server::TargetSocket::All, {{"type", "exit_request"}})) {
                //on fail we force
                GempyreUtils::log(GempyreUtils::LogLevel::Warning, "exit - send force", m_ui->state_str());
                m_ui->close_server(); //at this point we can close server (it may already be close)
                return false;
            }
            return true;
        });
        m_ui->flush_timers(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop", m_ui->state_str());
        m_ui->eventLoop(false);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "exit - wait in eventloop done, back in mainloop", m_ui->state_str());
        }

    else if(*m_ui == State::CLOSE) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Status change -> EXIT");
        m_ui->set(State::EXIT);  //there will be no one      
    }
    else {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "on exit switch", m_ui->state_str());
    }
    m_ui->signal_pending();
}


void Ui::begin_batch() {
    m_ui->add_request([this]() {
        return m_ui->begin_batch();
    });
}

void Ui::end_batch() {
    m_ui->add_request([this]() {
        return m_ui->end_batch();
    });
}


Ui::TimerId Ui::start_periodic(const std::chrono::milliseconds &ms, const std::function<void (TimerId)> &timerFunc) {
    assert(timerFunc);
    auto caller = m_ui->makeCaller(timerFunc);
    const int id = m_ui->append_timer(ms, false, std::move(caller));
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start Periodic", ms.count(), id);
    return id;
}

Ui::TimerId Ui::start_periodic(const std::chrono::milliseconds &ms, const std::function<void ()> &timerFunc) {
    return start_periodic(ms, [timerFunc](TimerId) {
        return timerFunc();
    });
}

Ui::TimerId Ui::after(const std::chrono::milliseconds &ms, const std::function<void (TimerId)> &timerFunc) {
    auto caller = m_ui->makeCaller(timerFunc);
    const int id = m_ui->append_timer(ms, true, std::move(caller));
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Start After", ms.count(), id);
    return id;
}

Ui::TimerId Ui::after(const std::chrono::milliseconds &ms, const std::function<void ()> &timerFunc) {
    return after(ms, [timerFunc](TimerId) {
        return timerFunc();
    });
}


bool Ui::cancel_timer(TimerId id) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Stop Timer", id);
    return m_ui->remove_timer(id);
}

Gempyre::Ui::ExitFunction Ui::on_exit(const ExitFunction& onUiExitFunction) {
    return m_ui->set_on_exit(onUiExitFunction);
}

Gempyre::Ui::ReloadFunction Ui::on_reload(const ReloadFunction& onReloadFunction) {
    return m_ui->set_on_reload(onReloadFunction);
}

Gempyre::Ui::OpenFunction Ui::on_open(const OpenFunction& onOpenFunction) {
    return m_ui->set_on_open(onOpenFunction);
}

Gempyre::Ui::ErrorFunction Ui::on_error(const ErrorFunction& onErrorFunction) {
    return m_ui->set_on_error(onErrorFunction);
}

void Ui::run() {
    m_ui->start_server();
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "run, Status change --> RUNNING");
    m_ui->set(State::RUNNING);
    m_ui->eventLoop(true);
    if(*m_ui != State::SUSPEND)
        m_ui->do_exit();
}


void Ui::set_logging(bool logging) {
    m_ui->send(root(), "logging", logging ? "true" : "false");
}

void Ui::eval(const std::string& eval) {
    m_ui->send(root(), "eval", eval);
}

void Ui::debug(const std::string& msg) {
    m_ui->send(root(), "debug", msg);
}

void Ui::alert(const std::string& msg) {
    m_ui->send(root(), "alert", msg);
}

void Ui::open(const std::string& url, const std::string& name) {
    m_ui->send(root(), "open", json {{"url", url}, {"view", name}});
}

bool Ui::available(const std::string& id) const {
    const auto is_available = m_ui->query<std::string>(id, "exists");
    return is_available && is_available.value() == "true";
}

std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> Ui::ping() const {
    const auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const clock_t begin_time = ::clock();
    const auto pong = m_ui->query<std::string>(std::string(), "ping");
    if(pong.has_value() && !pong->empty()) {
        // full loop
        const auto full = double(::clock() - begin_time) / (CLOCKS_PER_SEC / 1000000.0);
        // timestamp from the response
        const auto pong_time = pong.value();
        const auto half = GempyreUtils::convert<decltype(milliseconds_since_epoch)>(pong_time) - milliseconds_since_epoch;
        return std::make_pair(
                   std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(full)),
                   std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double, std::ratio<1, 1000000>>(half)));
    } else {
        GEM_DEBUG("Bad ping pong");
    }
    return std::nullopt;
}

Element Ui::root() const {
    return Element(*const_cast<Ui*>(this), "");
}


std::string Ui::address_of(const std::string& filepath) const {
    gempyre_utils_assert_x(m_ui->is_connected(), "Not connected");
    return std::string(SERVER_ADDRESS) + ":" + std::to_string(m_ui->port()) +
           "?file=" + GempyreUtils::hexify(GempyreUtils::abs_path(filepath), R"([^a-zA-Z0-9-,.,_~])");
}

std::optional<Element::Elements> Ui::by_class(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = m_ui->query<std::vector<std::string>>(className, "classes");
    if(!childIds.has_value()) {
        return std::nullopt;
    }
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return *m_ui == State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

std::optional<Element::Elements> Ui::by_name(const std::string& className) const {
    Element::Elements childArray;
    const auto childIds = m_ui->query<std::vector<std::string>>(className, "names");
    if(!childIds.has_value()) {
        return std::nullopt;
    }
    for(const auto& cid : *childIds) {
        childArray.push_back(Element(*const_cast<Ui*>(this), cid));
    }
    return *m_ui == State::RUNNING ? std::make_optional(childArray) : std::nullopt;
}

void Ui::extension_call(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) {
    const auto json = GempyreUtils::to_json_string(parameters);
    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");
    m_ui->add_request([this, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_ui->send_to(Server::TargetSocket::Extension, {
                                  {"type", "extension"},
                                  {"extension_call", callId},
                                  {"extension_id", ""},
                                  {"extension_parameters", json.value()}});
    });
}

/*
 * TODO: remove me
std::optional<std::any> Ui::extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) {
    return extensionGet(callId, parameters);
}
*/

std::optional<std::any> Ui::extension_get(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters)  {
    if(*m_ui != State::RUNNING) {
        return std::nullopt;
    }
    const auto queryId = m_ui->query_id();

    const auto json = GempyreUtils::to_json_string(parameters);

    gempyre_utils_assert_x(json.has_value(), "Invalid parameter");

    m_ui->add_request([this, queryId, callId, json]() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension:", json.value());
        return m_ui->send_to(Server::TargetSocket::Extension, {
            {"type", "extension"},
            {"extension_id", queryId},
            {"extension_call", callId},
            {"extension_parameters", json.value()}});
    });

    while(m_ui->is_running()) {   //start waiting the response
        m_ui->eventLoop(false);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "extension - wait in eventloop done, back in mainloop", m_ui->state_str());
        if(*m_ui != State::RUNNING) {
            m_ui->signal_pending();
            break; //we are gone
        }

        const auto response = m_ui->take_response(queryId);
        if(response)
            return response;
    }
    return std::nullopt;
}
std::optional<std::vector<uint8_t>> Ui::resource(const std::string& url) const {
    const auto file = m_ui->file(url);
    if(!file) {
        return std::nullopt;
    }
    const auto data = Base64::decode(*file);
    return std::make_optional(data);
}

bool Ui::add_file(const std::string& url, const std::string& file_name) {
    if(!GempyreUtils::file_exists(file_name)) {
        return false;
    }
    const auto file = m_ui->file(url);
    if(file) { // expect not found
        return false;
    }
    m_ui->add_file(url, file_name);
    return true;
}

bool Ui::add_data(const std::string& url, const std::vector<uint8_t>& data) {
    if(data.empty())
        return false;
    m_ui->add_data(url, data);    
    return true;    
}

std::optional<double> Ui::device_pixel_ratio() const {
    const auto value = m_ui->query<std::string>("", "devicePixelRatio");
    return value.has_value() && *m_ui == State::RUNNING ? GempyreUtils::parse<double>(value.value()) : std::nullopt;
}

void Ui::set_application_icon(const uint8_t *data, size_t dataLen, const std::string& type) {
    extension_call("setAppIcon", {{"image_data", Base64::encode(data, dataLen)}, {"type", type}});
}

void Ui::resize(int width, int height) {
    extension_call("resize", {{"width", width}, {"height", height}});
}

void Ui::set_title(const std::string& name) {
    extension_call("setTitle", {{"title", name}});
}

/*
std::string Ui::stdParams(int width, int height, const std::string& title) {
    std::stringstream ss;
    ss << " --gempyre-width=" << width << " --gempyre-height=" << height << " --gempyre-title=\"" << title << "\""; // circle with spaces
    return ss.str();
}
*/
std::optional<std::string> Ui::add_file(Gempyre::Ui::Filemap& map, const std::string& file) {
    if(!GempyreUtils::file_exists(file)) {
        return std::nullopt;
    }
    auto url = GempyreUtils::substitute(file, R"([\/\\])", "_");
    if(map.find(url) != map.end()) {
        return std::nullopt;
    }

    url.insert(url.begin(), '/');

    const auto data = GempyreUtils::slurp<Base64::Byte>(file);
    const auto string = Base64::encode(data);
    map.insert_or_assign(url, std::move(string));
    return url;
}

void Ui::set_timer_on_hold(bool on_hold) {
    m_ui->set_hold(on_hold);
    }

bool Ui::is_timer_on_hold() const {
    return m_ui->is_hold();
    }


void Ui::suspend() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "suspend state is " + std::string(m_ui->state_str()));
    gempyre_utils_assert(m_ui->loop() == 1);

    if(*m_ui != State::RUNNING) { // maybe for ealier error
        GempyreUtils::log(GempyreUtils::LogLevel::Warning, "suspend state is " + std::string(m_ui->state_str()));
        return;
    }

    gempyre_utils_assert(!is_timer_on_hold());
    set_timer_on_hold(true);
    m_ui->set(State::SUSPEND);
    m_ui->clear_handlers();
    m_ui->flush_timers(false);
    m_ui->clear();
    m_ui->signal_pending();
    std::this_thread::sleep_for(200ms); // uws has strange corking problem
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "suspend exit is " + std::string(m_ui->state_str()));
 
}

void Ui::resume() {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "resume state is " + std::string(m_ui->state_str()));
    gempyre_utils_assert(m_ui->loop() == 0);
    if(*m_ui == State::SUSPEND) {
        m_ui->set(State::RUNNING);
        set_timer_on_hold(false);
        m_ui->eventLoop(true);
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "resumed exit " + std::string(m_ui->state_str()));  
    } else {
        gempyre_utils_assert(*m_ui != State::EXIT);  
        exit();
    }
}

void Ui::flush() {
    m_ui->shoot_requests();
}

 bool Ui::ui_available() const {
    return m_ui->is_connected() && m_ui->is_running();
 }

