#ifndef GEMPYRE_H
#define GEMPYRE_H

#include <iterator>
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
  * gempyre.h
  * =====
  * Gempyre GUI Framework
  * -------------
  *
  * gempyre.h contains core functionality, everything that is needed for basic application using Gempyre framework.
  */

using namespace std::chrono_literals;

#ifdef WINDOWS_EXPORT
#define GEMPYRE_EX __declspec( dllexport )
#else
#define GEMPYRE_EX
#endif


namespace Gempyre {

    class Ui;
    class Server;
    class Semaphore;
    class TimerMgr;
    template <class T> class EventQueue;
    template <class T> class IdList;
    template <class K, class T> class EventMap;

    enum class DebugLevel{Quiet, Fatal, Error, Warning, Info, Debug, Debug_Trace};

    /// set debuging level and target, defauls to std::cout
    GEMPYRE_EX void setDebug(DebugLevel level = DebugLevel::Debug, bool useLog = false);
    /// Internal for Android
    GEMPYRE_EX void setJNIENV(void* env, void* obj);
    /// Return current version
    GEMPYRE_EX std::tuple<int, int, int> version();

    class Data;
    using DataPtr = std::shared_ptr<Data>;

    struct Event;

    class GEMPYRE_EX Element {
    public:
        using Attributes = std::unordered_map<std::string, std::string>;
        using Values = std::unordered_map<std::string, std::string>;
        using Elements = std::vector<Element>;
        struct Rect {
            int x;
            int y;
            int width;
            int height;
        };
    public:
        /// Copy constructor
        Element(const Element& other) = default;
        /// Move constructor
        Element(Element&& other) = default;
        Element& operator=(const Element& other) {m_ui = other.m_ui; m_id = other.m_id; return *this;}
        Element& operator=(Element&& other) {m_ui = other.m_ui; m_id = std::move(other.m_id); return *this;}

        Element(Ui& ui, const std::string& id);
        Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent);
        Element(Ui& ui, const std::string& htmlElement, const Element& parent);

        virtual ~Element() = default;
        const Ui& ui() const { return *m_ui; }
        Ui& ui() { return *m_ui;}

        std::string id() const {return m_id;}
        Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& throttle = 0ms);
        Element& setHTML(const std::string& htmlText);
        Element& setAttribute(const std::string& attr, const std::string& value = "");
        std::optional<Attributes> attributes() const;
        Element& setStyle(const std::string& style, const std::string& value);
        Element& removeStyle(const std::string& style);
        Element& removeAttribute(const std::string& attr);
        std::optional<Values> styles(const std::vector<std::string>& keys) const;
        std::optional<Elements> children() const;
        std::optional<Values> values() const;
        std::optional<std::string> html() const;
        void remove();
        std::optional<std::string> type() const;
        std::optional<Rect> rect() const;
    protected:
        void send(const DataPtr& data);
        void send(const std::string& type, const std::any& data);
        static const std::string generateId(const std::string& prefix);
        size_t payloadSize() const;
    protected:
        Ui* m_ui;
        std::string m_id;
        friend class Ui;
    };
    struct Event {
        Element element;
        std::unordered_map<std::string, std::string> properties;
    };


    class GEMPYRE_EX Ui {
        struct Event {
            Element element;
            const std::unordered_map<std::string, std::any> properties;
        };
        using Handler = std::function<void(const Event& el)>;
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

        ///Application gracefully finish the event loop and exits.
        void exit();
        ///Requires Client window to close (that cause the application to close).
        void close();

        ///The callback is called before before the eventloop exit.
        Ui& onExit(std::function<void ()> onExitFunction);
        ///The callback is called on UI reload.
        Ui& onReload(std::function<void ()> onReleadFunction);
        ///The callback is called on UI open.
        Ui& onOpen(std::function<void ()> onOpenFunction);
        ///The callback is called on error.
        Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction);
        ///Starts the event loop.
        void run();

        ///Set broser to verbose mode
        void setLogging(bool logging);
        ///Executes eval string in UI context.
        void eval(const std::string& eval);
        ///Send a debug message on UI.
        void debug(const std::string& msg);
        ///Show an alert window.
        void alert(const std::string& msg);
        ///Opens an url in the UI view
        void open(const std::string& url, const std::string& name = "");

        ///Starts a timer.
        TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc);
        ///Starts a timer.
        TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc);
        ///Stops a timer.
        bool stopTimer(TimerId timerId);

        ///Get a (virtual) root element.
        Element root() const;
        ///Get a local file path an URL, can be used with open.
        std::string addressOf(const std::string& filepath) const;
        ///Get elements by class name
        std::optional<Element::Elements> byClass(const std::string& className) const;
        ///Get elements by name
        std::optional<Element::Elements> byName(const std::string& className) const;

        ///Test function to measure round trip time
        std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const;
        ///Access an UI extension
        std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);
        ///Get a compiled in resource string.
        std::optional<std::vector<uint8_t>> resource(const std::string& url) const;
        ///Add a file data into Gempyre to be accessed via url
        bool addFile(const std::string& url, const std::string& file);
        ///Starts an UI write batch, no messages are sent to USER until endBatch
        void beginBatch();
        ///Ends an UI read batch, push all stored messages at once.
        void endBatch();
        ///Set all timers to hold. Can be used to pause UI actions.
        void holdTimers(bool hold) {m_hold = hold;}
        ///Tells if timers are on hold.
        bool isHold() const {return m_hold;}
        ///Get an native UI device pixel ratio.
        std::optional<double> devicePixelRatio() const;
    private:
        enum class State {NOTSTARTED, RUNNING, RETRY, EXIT, CLOSE, RELOAD, PENDING};
        void send(const DataPtr& data);
        void send(const Element& el, const std::string& type, const std::any& data);
        template<class T> std::optional<T> query(const std::string& elId, const std::string& queryString, const std::vector<std::string>& queryParams = {});
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
        using HandlerMap = std::unordered_map<std::string, Handler>;
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
        bool m_hold = false;
        friend class Element;
        friend class Server;
    };

    class GEMPYRE_EX Data {
    public:
        template <class T> class iteratorT {
        public:
            using iterator_category = std::forward_iterator_tag; //could be upgraded, but I assume there is no need
            using value_type = T;
            using difference_type = void;
            using pointer = T*;
            using reference = T&;
            iteratorT(pointer data = nullptr) : m_data(data) {}
            iteratorT(const iteratorT& other) = default;
            iteratorT& operator=(const iteratorT& other) = default;
            bool operator==(const iteratorT& other) const  {return m_data == other.m_data;}
            bool operator!=(const iteratorT& other) const  {return m_data != other.m_data;}
            reference operator*() {return *m_data;}
            reference operator*() const {return *m_data;}
            pointer operator->() {return m_data;}
            value_type operator++() {++m_data ; return *m_data;}
            value_type operator++(int) {auto temp(m_data); ++m_data; return *temp;}
        private:
            pointer m_data;
        };
        using dataT = uint32_t;
        using iterator = iteratorT<dataT>;
        using const_iterator = iteratorT<const dataT>;
    public:
        dataT* data();
        const dataT* data() const;
        size_t size() const;
        Data::iterator begin() {return data();}
        Data::iterator end() {return data() + size();}
        const Data::const_iterator begin() const {return data();}
        const Data::const_iterator end() const {return data() + size();}
        dataT& operator[](int index) {return (data()[index]);}
        dataT operator[](int index) const {return (data()[index]);}
        dataT* endPtr() {return data() + size();}
        const dataT* endPtr() const {return data() + size();}
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
}

#endif // GEMPYRE_H
