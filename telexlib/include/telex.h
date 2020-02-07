#ifndef TELEX_H
#define TELEX_H

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <deque>
#include <chrono>
#include <atomic>
#include <any>
#include <optional>
#include <vector>
#include <variant>
#include <mutex>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * Telex
  * =====
  * GUI Framework
  * -------------
  *
  * @toc
  *
  */

/** @scope
  * Telex contains three header files
  * * `telex.h`
  * * `telex_utils.h`
  * * `telex_graphics.h`
  * @scopeend
 */

using namespace std::chrono_literals;

#ifdef WINDOWS_EXPORT
#define TELEX_EX __declspec( dllexport )
#else
#define TELEX_EX
#endif

/**
 * @namespace Telex
 * Common namespace for Telex implementation.
 */
namespace Telex {

    class Ui;
    class Server;
    class Semaphore;
    class TimerMgr;
    template <class T> class EventQueue;
    template <class T> class IdList;
    template <class K, class T> class EventMap;

    /**
     * @function setDebug
     * Enable debug outputs
     */
    TELEX_EX void setDebug();

    class Data;
    using DataPtr = std::shared_ptr<Data>;
    /**
     * @class Data
     * Generic container for data
     */
    class TELEX_EX Data {
    public:
        using dataT = uint32_t;
        dataT* data();
        const dataT* data() const;
        size_t size() const;
        dataT* begin() {return data();}
        dataT* end() {return data() + size();}
        const dataT* begin() const {return data();}
        const dataT* end() const {return data() + size();}
        dataT& operator[](int index) {return (data()[index]);}
        dataT operator[](int index) const {return (data()[index]);}
        void writeHeader(const std::vector<dataT>& header);
        std::vector<dataT> header() const;
        std::string owner() const;
        DataPtr clone() const;
        virtual ~Data() = default;
    protected:
        Data(size_t sz, dataT type, const std::string& owner, const std::vector<dataT>& header);
    private:
        std::tuple<const char*, size_t> payload() const;
        std::vector<dataT> m_data;
        friend class Element;
        friend class Ui;
    };



    /**
     * @class Element
     * Element is any UI element
     */
    class TELEX_EX Element {
    public:
        using Attributes = std::unordered_map<std::string, std::string>;
        using Values = std::unordered_map<std::string, std::string>;
        using Elements = std::vector<Element>;
        struct Event {
            std::shared_ptr<Element> element;
            const std::unordered_map<std::string, std::any> properties;
        };
        using Handler = std::function<void(const Event& el)>;
        struct Rect {int x; int y; int width; int height;};
    public:
        Element(const Element& other) = default;
        Element(Element&& other) = default;
        Element& operator=(const Element& other) {m_ui = other.m_ui; m_id = other.m_id; return *this;}
        Element& operator=(Element&& other) {m_ui = other.m_ui; m_id = std::move(other.m_id); return *this;}
        /**
         * @function Element
         * @param ui
         * @param id
         * Creates instance that refers to existing element
         */
        Element(Ui& ui, const std::string& id);
        /**
         * @function Element
         * @param ui
         * @param id
         * @param htmlElement
         * @param parent
         * Creates a new elements as given HTML type and parent
         */
        Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent);
        virtual ~Element() = default;
        /**
         * @function ui
         * @return current Ui
         */
        const Ui& ui() const {
            return *m_ui;
        }
        /**
         * @function ui
         * @return
         */
        Ui& ui() {
            return *m_ui;
        }
        /**
         * @function id
         * @return HTML identifer of this element
         */
        std::string id() const {return m_id;}
        /**
         * @function subscribe
         * @param name
         * @param handler
         * @param throttle
         * set function to listen ui element
         * @return
         */
        Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& = 0ms);
        /**
         * @function setHTML
         * @param name
         * @param handler
         * @return
         */
        Element& setHTML(const std::string& htmlText);
        /**
         * @function setAttribute
         * @param attr
         * @param values
         * @return
         * Set a given attribute a given name
         */
        Element& setAttribute(const std::string& attr, const std::string& values);
        /**
         * @function attributes
         * @return
         */
        std::optional<Attributes> attributes() const;
        /**
         * @function children
         * @return
         * All attributes this element has
         */
        std::optional<Elements> children() const;
        /**
         * @function values
         * @return
         * Children ot this element (non-recursive)
         */
        std::optional<Values> values() const;
        /**
         * @function html
         * @return
         * For input types returns all the values this element has
         */
        std::optional<std::string> html() const;
        /**
         * @function remove
         * Remove this element from page
         */
        void remove();
        /**
         * @function type
         * @return a HTML tag in lower case, empty is not found, nullopt if page is not ready.
         *
         */
        std::optional<std::string> type() const;
        /**
         * @function rect
         * @return bounding rect of element
         */
        std::optional<Rect> rect() const;
    protected:
        void send(const DataPtr& data);
        void send(const std::string& type, const std::any& data);
    protected:
        Ui* m_ui;
        std::string m_id;
        friend class Ui;
    };

    /**
     * @class Ui
     */
    class TELEX_EX Ui {
    public:
        using Filemap = std::unordered_map<std::string, std::string>;
        using TimerId = int;
        static constexpr unsigned short UseDefaultPort = 0; //zero means default port
        static constexpr char UseDefaultRoot[] = "";   //zero means default root
        explicit Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        explicit Ui(const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        explicit Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        explicit Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        ~Ui();
        Ui(const Ui& other) = delete;
        Ui(Ui&& other) = delete;
        /**
         * @function exit
         * Exits the eventloop and make application to close
         */
        void exit();
        /**
         * @function close
         * Asks Client window to close (which then signals application to close)
         */
        void close();
        /**
         * @function onUiExit
         * @param onExitFunction
         * @return
         * Callback just before exit
         */
        Ui& onUiExit(std::function<void ()> onExitFunction = nullptr);
        /**
         * @function onReload
         * @param onReleadFunction
         * @return
         * Callback when browser window reload occurs.
         */
        Ui& onReload(std::function<void ()> onReleadFunction = nullptr);
        /**
         * @function onOpen
         * @param onOpenFunction
         * @return
         * Callback when browser UI is running.
         */
        Ui& onOpen(std::function<void ()> onOpenFunction = nullptr);
        /**
         * @function onError
         * @param onErrorFunction
         * @return
         * Browser reports an issue
         */
        Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction = nullptr);
        /**
         * @function run
         * Starts eventloop
         */
        void run();
        /**
         * @function setLogging
         * @param logging
         * Enforces Browser being verbose when serving Telex
         */
        void setLogging(bool logging);
        /**
         * @function eval
         * @param eval
         * Execute code on browser (calls JS eval)
         */
        void eval(const std::string& eval);
        /**
         * @function debug
         * @param msg
         * Echoes a message
         */
        void debug(const std::string& msg);
        /**
         * @function alert
         * @param msg
         * Shown a browser's alert dialogue
         */
        void alert(const std::string& msg);
        /**
         * @function open
         * @param url
         * @param name
         * Opens an extrenal URL on browser tab
         */
        void open(const std::string& url, const std::string& name = "");
        /**
         * @function startTimer
         * @param ms
         * @param singleShot
         * @param timerFunc
         * @return
         */
        TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc);
        /**
         * @function startTimer
         * @param ms
         * @param singleShot
         * @param timerFunc
         * @return
         * Starts a timer that is called after given amount of milliseconds
         */
        TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc);
        /**
         * @function stopTimer
         * @return
         */
        bool stopTimer(TimerId);
        /**
         * @function root
         * @return
         * Pseudo element that represents root of the element structure
         */
        Element root() const;
        /**
         * @function addressOf
         * @param filepath
         * @return
         * Translates given path to address that Telex can read when provided as a link.
         */
        std::string addressOf(const std::string& filepath) const;
        /**
         * @function byClass
         * @param className
         * @return
         * Returns all elements match to given class
         */
        std::optional<Element::Elements> byClass(const std::string& className) const;
        /**
         * @function byName
         * @param className
         * @return
         * Returns all elements match to given name
         */
        std::optional<Element::Elements> byName(const std::string& className) const;
        /**
         * @function ping
         * @return
         * Just a ping
         */
        std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const;
        /**
         * @function extension
         * @param callId
         * @param parameters
         * @return
         * Low level access to extension services that browser may implement. The return parameter and return value are JSON kind
         * of structures or types and depends on given callId.
         * Note if return value contain a string it is very straighforward
         *
         * for example
         *```
         * const auto out = ui.extension("openFile", {{"caption", "hexview - open"}});
         * const std::string filename = std::any_cast<std::string>(*out);
         *```
         * but if is an array of string values must be converted accordint to structure
         *```
         * const auto out = ui.extension("openFile", {{"caption", "hexview - open"}});
         * const auto anyvec = std::any_cast<std::vector<std::any>(*out);
         * std::vector<std::string> vec;
         * std::transform(anyvec.begin(), anyvec.end, std::back_inserter(vec), [](const auto& a){return std::any_cast<std::string>(a)});
         *```
         */
        std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);

        /**
         * @brief resource
         * @param url
         * @return
         * Get data stored as a resource
         */
        std::optional<std::vector<uint8_t>> resource(const std::string& url) const;
        /**
         * @brief addFile
         * @param url
         * @param file
         * @return
         * Adds a gile as a resources
         */
        bool addFile(const std::string& url, const std::string& file);
        /**
         * @brief beginBatch
         * Starts
         */
        /**
         * @brief beginBatch
         * Starts buffering non-DataPtr message locally
         */
        void beginBatch();
        /**
         * @brief endBatch
         * Sends locally buffered message to UI
         */
        void endBatch();
    private:
        enum class State {NOTSTARTED, RUNNING, RETRY, EXIT, CLOSE, RELOAD, PENDING};
        void send(const DataPtr& data);
        void send(const Element& el, const std::string& type, const std::any& data);
        template<class T> std::optional<T> query(const std::string& elId, const std::string& queryString);
        void pendingClose();
        void eventLoop();
        static std::string toStr(const std::atomic<State>&);
        inline void addRequest(std::function<bool()>&&);
    private:
        std::atomic<State> m_status = State::NOTSTARTED;
        std::unique_ptr<EventQueue<std::tuple<std::string, std::string, std::unordered_map<std::string, std::any>>>> m_eventqueue;
        std::unique_ptr<EventMap<std::string, std::any>> m_responsemap;
        std::unique_ptr<Semaphore>  m_sema;
        std::unique_ptr<TimerMgr> m_timers;
        using HandlerMap = std::unordered_map<std::string, Element::Handler>;
        std::unordered_map<std::string, HandlerMap> m_elements;
        std::deque<std::function<bool ()>> m_requestqueue;
        std::deque<std::function<void ()>> m_timerqueue;
        std::function<void ()> m_onUiExit = nullptr;
        std::function<void ()> m_onReload = nullptr;
        std::function<void ()> m_onOpen = nullptr;
        std::function<void (const std::string& element, const std::string& info)> m_onError = nullptr;
        std::unique_ptr<Server> m_server;
        std::function<void ()> m_startup;
        Filemap m_filemap;
        std::mutex m_mutex;
        friend class Element;
        friend class Server;
    };
}

#endif // TELEX_H
