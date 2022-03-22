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
#include <tuple>

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

    /// set debuging on/off
    GEMPYRE_EX void setDebug(bool isDebug = true);
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
        [[nodiscard]] const Ui& ui() const { return *m_ui; }
        [[nodiscard]] Ui& ui() { return *m_ui;}

        [[nodiscard]] std::string id() const {return m_id;}
        Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& throttle = 0ms);
        Element& setHTML(const std::string& htmlText);
        Element& setAttribute(const std::string& attr, const std::string& value = "");
        std::optional<Attributes> attributes() const;
        Element& setStyle(const std::string& style, const std::string& value);
        Element& removeStyle(const std::string& style);
        Element& removeAttribute(const std::string& attr);
        [[nodiscard]] std::optional<Values> styles(const std::vector<std::string>& keys) const;
        [[nodiscard]] std::optional<Elements> children() const;
        [[nodiscard]] std::optional<Values> values() const;
        [[nodiscard]] std::optional<std::string> html() const;
        void remove();
        [[nodiscard]] std::optional<std::string> type() const;
        [[nodiscard]] std::optional<Rect> rect() const;
    protected:
        void send(const DataPtr& data);
        void send(const std::string& type, const std::any& data, bool unique = false);
        [[nodiscard]] static const std::string generateId(const std::string& prefix);
        [[nodiscard]] size_t payloadSize() const;
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
        using Handler = std::function<void (const Event& el)>;
    public:
        enum UiFlags : unsigned {
            NoResize = 0x1,
            FullScreen = 0x2,
            Hidden = 0x4,
            Frameless = 0x8,
            Minimized = 0x10,
            OnTop = 0x20,
            ConfirmClose = 0x40,
            TextSelect = 0x80,
            EasyDrag = 0x100,
            Transparent = 0x200
        };
        using Filemap = std::unordered_map<std::string, std::string>;
        using TimerId = int;
        static constexpr unsigned short UseDefaultPort = 0; //zero means default port
        static constexpr auto UseDefaultRoot = "";   //zero means default root
       /* [[nodiscard]]
        static std::string stdParams(int width, int height, const std::string& title);

        /// load a file
        [[deprecated]]
        explicit Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        /// load a file
        [[deprecated]]
        //explicit Ui(const std::string& indexHtml, const std::string& browser, int width, int height, const std::string& title, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        [[deprecated]]
        explicit Ui(const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);


        /// use explicit app as UI
        [[deprecated]]
        //explicit Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        /// use explicit app as UI
        [[deprecated]]
        //explicit Ui(const Filemap& filemap, const std::string& indexHtml, int width, int height, const std::string& title, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        /// use OS browser as UI
        explicit Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        */
        /// Create UI using default ui app or gempyre.conf
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& title = "",  int width = -1, int height = -1, unsigned flags = 0, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

        /// Create UI using given ui app and command line
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser,  const std::string& browser_params, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

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

        ///Starts a perdiodic timer.
        TimerId startPeriodic(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);
        ///Starts a perdiodic timer.
        TimerId startPeriodic(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        ///Starts a single shot timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);
        ///Starts a perdiodic timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        ///Stops a timer.
        bool cancelTimer(TimerId timerId);

        ///Get a (virtual) root element.
        [[nodiscard]] Element root() const;
        ///Get a local file path an URL, can be used with open.
        [[nodiscard]] std::string addressOf(const std::string& filepath) const;
        ///Get elements by class name
        [[nodiscard]] std::optional<Element::Elements> byClass(const std::string& className) const;
        ///Get elements by name
        [[nodiscard]] std::optional<Element::Elements> byName(const std::string& className) const;

        ///Test function to measure round trip time
        [[nodiscard]] std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const;
        ///Access an UI extension
        void extensionCall(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);
        ///Access an UI extension
        [[nodiscard]] std::optional<std::any> extensionGet(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);

        //[[deprecated ("use extensionGet or extensionCall instead")]] std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);


        ///Get a compiled in resource string.
        [[nodiscard]] std::optional<std::vector<uint8_t>> resource(const std::string& url) const;
        ///Add a file data into Gempyre to be accessed via url
        bool addFile(const std::string& url, const std::string& file);
        ///Add file data into map to be added as a map
        static std::optional<std::string> addFile(Filemap& map, const std::string& filename);
        ///Starts an UI write batch, no messages are sent to USER until endBatch
        void beginBatch();
        ///Ends an UI read batch, push all stored messages at once.
        void endBatch();
        ///Set all timers to hold. Can be used to pause UI actions.
        void holdTimers(bool hold) {m_hold = hold;}
        ///Tells if timers are on hold.
        [[nodiscard]] bool isHold() const {return m_hold;}
        ///Get an native UI device pixel ratio.
        [[nodiscard]] std::optional<double> devicePixelRatio() const;
        ///Set application icon, fail silently if backend wont support
        void setApplicationIcon(const uint8_t* data, size_t dataLen, const std::string& type);
        /// resize, fail silently if backend wont support
        void resize(int width, int height);
        /// set title, fail silently if backend wont support
        void setTitle(const std::string& name);
        /// load file as a maps
        static Ui::Filemap toFileMap(const std::vector<std::string>& filenames);
    private:
        enum class State {NOTSTARTED, RUNNING, RETRY, EXIT, CLOSE, RELOAD, PENDING};
        Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port, const std::string& root, const std::unordered_map<std::string, std::string>& parameters);
        void send(const DataPtr& data);
        void send(const Element& el, const std::string& type, const std::any& data, bool unique = false);
        template<class T> std::optional<T> query(const std::string& elId, const std::string& queryString, const std::vector<std::string>& queryParams = {});
        void pendingClose();
        void eventLoop(bool is_main);
        static std::string toStr(const std::atomic<State>&);
        inline void addRequest(std::function<bool()>&&);
        std::tuple<std::string, std::string> guiCmdLine(const std::string& indexHTML, int port, const std::unordered_map<std::string, std::string>& browser_params);
        std::function<void(int)> makeCaller(const std::function<void (TimerId id)>& function);
    private:
        struct InternalEvent {
            std::string element;
            std::string handler;
            std::unordered_map<std::string, std::any> data;
        };
    private:
        std::atomic<State> m_status = State::NOTSTARTED;
        std::unique_ptr<EventQueue<InternalEvent>> m_eventqueue;
        std::unique_ptr<EventMap<std::string, std::any>> m_responsemap;
        std::unique_ptr<Semaphore>  m_sema;
        std::unique_ptr<TimerMgr> m_timers;
        using HandlerMap = std::unordered_map<std::string, Handler>;
        std::unordered_map<std::string, HandlerMap> m_elements;
        std::deque<std::function<bool ()>> m_requestqueue;
        std::deque<std::function<void ()>> m_timerqueue;
        std::function<void ()> m_onUiExit{nullptr};
        std::function<void ()> m_onReload{nullptr};
        std::function<void ()> m_onOpen{nullptr};
        std::function<void (const std::string& element, const std::string& info)> m_onError{nullptr};
        std::unique_ptr<Server> m_server;
        std::function<void ()> m_startup;
        Filemap m_filemap;
        std::mutex m_mutex;
        bool m_hold{false};
        unsigned m_msgId{1};
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
        [[nodiscard]] dataT* data();
        [[nodiscard]] const dataT* data() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] Data::iterator begin() {return data();}
        [[nodiscard]] Data::iterator end() {return data() + size();}
        [[nodiscard]] const Data::const_iterator begin() const {return data();}
        [[nodiscard]] const Data::const_iterator end() const {return data() + size();}
        [[nodiscard]] dataT& operator[](int index) {return (data()[index]);}
        [[nodiscard]] dataT operator[](int index) const {return (data()[index]);}
        [[nodiscard]] dataT* endPtr() {return data() + size();}
        [[nodiscard]] const dataT* endPtr() const {return data() + size();}
        void writeHeader(const std::vector<dataT>& header);
        [[nodiscard]] std::vector<dataT> header() const;
        [[nodiscard]] std::string owner() const;
        [[nodiscard]] DataPtr clone() const;
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
