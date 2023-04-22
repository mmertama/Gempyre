#ifndef GEMPYRE_H
#define GEMPYRE_H

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <any>
#include <optional>
#include <vector>
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
    class GempyreInternal;
    enum class CloseStatus;
    enum class WindowType;
    template <class T> class EventQueue;
    template <class T> class IdList;
    template <class K, class T> class EventMap;

    /// set debuging on/off
    GEMPYRE_EX void set_debug(bool isDebug = true);
   
    /// Internal for Android
    GEMPYRE_EX void setJNIENV(void* env, void* obj);
    /// Return current version
    GEMPYRE_EX std::tuple<int, int, int> version();

    class Data;
    using DataPtr = std::shared_ptr<Data>;
    using dataT = uint32_t;

    struct Event;

    class GEMPYRE_EX Element {
    public:
        using Attributes = std::unordered_map<std::string, std::string>;
        using Values = std::unordered_map<std::string, std::string>;
        using Elements = std::vector<Element>;
        using SubscribeFunction = std::function<void(const Event&)>;
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

        virtual ~Element();
        [[nodiscard]] const Ui& ui() const { return *m_ui; }
        [[nodiscard]] Ui& ui() { return *m_ui;}

        [[nodiscard]] std::string id() const {return m_id;}
        Element& subscribe(const std::string& name, const SubscribeFunction& handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& throttle = 0ms);
        Element& set_html(const std::string& htmlText);
        Element& set_attribute(const std::string& attr, const std::string& value = "");
        std::optional<Attributes> attributes() const;
        Element& set_style(const std::string& style, const std::string& value);
        Element& remove_attribute(const std::string& attr);
        [[nodiscard]] std::optional<Values> styles(const std::vector<std::string>& keys) const;
        [[nodiscard]] std::optional<Elements> children() const;
        [[nodiscard]] std::optional<Values> values() const;
        [[nodiscard]] std::optional<std::string> html() const;
        void remove();
        [[nodiscard]] std::optional<std::string> type() const;
        [[nodiscard]] std::optional<Rect> rect() const;
    protected:    
        const GempyreInternal& ref() const;
        GempyreInternal& ref();
        static const std::string generateId(const std::string& prefix);    
    protected:
        Ui* m_ui;
        std::string m_id;
    private:   
        friend class GempyreInternal;
    };

    struct Event {
        Element element;
        std::unordered_map<std::string, std::string> properties;
    };


    class GEMPYRE_EX Ui {
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
        /// Create UI using default ui app or gempyre.conf
        Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);
        //Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& title = "",  int width = -1, int height = -1, unsigned flags = 0, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);


        /// Create UI using given ui app and command line
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser,  const std::string& browser_params, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

        /// Create window UI
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& title,  int width, int height,
            const std::unordered_map<std::string, std::string>& ui_params = {}, unsigned flags = 0, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);


        ~Ui();
        Ui(const Ui& other) = delete;
        Ui(Ui&& other) = delete;

        ///Application gracefully finish the event loop and exits.
        void exit();
        ///Requires Client window to close (that cause the application to close).
        [[deprecated("Prefer exit")]] void close();

        using ExitFunction = std::function<void ()>;
        using ReloadFunction = std::function<void ()>;
        using OpenFunction = std::function<void ()>;
        using ErrorFunction = std::function<void (const std::string& element, const std::string& info)>;


        ///The callback is called before before the eventloop exit.
        ExitFunction on_exit(const ExitFunction& onExitFunction);
        ///The callback is called on UI reload.
        ReloadFunction on_reload(const ReloadFunction& onReleadFunction);
        ///The callback is called on UI open.
        OpenFunction on_open(const OpenFunction& onOpenFunction);
        ///The callback is called on error.
        ErrorFunction on_error(const ErrorFunction& onErrorFunction);
        ///Starts the event loop.
        void run();

        ///Set broser to verbose mode
        void set_logging(bool logging);
        ///Executes eval string in UI context.
        void eval(const std::string& eval);
        ///Send a debug message on UI.
        void debug(const std::string& msg);
        ///Show an alert window.
        void alert(const std::string& msg);
        ///Opens an url in the UI view
        void open(const std::string& url, const std::string& name = "");

        ///Starts a perdiodic timer.
        TimerId start_periodic(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);
        ///Starts a perdiodic timer.
        TimerId start_periodic(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        ///Starts a single shot timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);

        ///Starts a perdiodic timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        ///Stops a timer.
        bool cancel_timer(TimerId timerId);


        ///Get a (virtual) root element.
        [[nodiscard]] Element root() const;
        ///Get a local file path an URL, can be used with open.
        [[nodiscard]] std::string address_of(const std::string& filepath) const;
        ///Get elements by class name
        [[nodiscard]] std::optional<Element::Elements> by_class(const std::string& className) const;
        ///Get elements by name
        [[nodiscard]] std::optional<Element::Elements> by_name(const std::string& className) const;
        ///Test function to measure round trip time
        [[nodiscard]] std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const;
        ///Access an UI extension
        void extension_call(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);
        ///Access an UI extension
        [[nodiscard]] std::optional<std::any> extension_get(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);

        ///Get a compiled in resource string.
        [[nodiscard]] std::optional<std::vector<uint8_t>> resource(const std::string& url) const;
        ///Add a file data into Gempyre to be accessed via url
        bool add_file(const std::string& url, const std::string& file);
        ///Add file data into map to be added as a map
        static std::optional<std::string> add_file(Filemap& map, const std::string& filename);
        ///Starts an UI write batch, no messages are sent to USER until endBatch
        void begin_batch();
        ///Ends an UI read batch, push all stored messages at once.
        void end_batch();
        ///Set all timers to hold. Can be used to pause UI actions.
        void set_timer_on_hold(bool on_hold);
        ///Tells if timers are on hold.
        [[nodiscard]] bool is_timer_on_hold() const;
        ///Get an native UI device pixel ratio.
        [[nodiscard]] std::optional<double> device_pixel_ratio() const;
        ///Set application icon, fail silently if backend wont support
        void set_application_icon(const uint8_t* data, size_t dataLen, const std::string& type);
        /// resize, fail silently if backend wont support
        void resize(int width, int height);
        /// set title, fail silently if backend wont support
        void set_title(const std::string& name);
        /// load file as a maps
        static Ui::Filemap to_file_map(const std::vector<std::string>& filenames);

        // for testing
        void resume();
        // for testing
        void suspend();
        
    private:
        Ui(const Filemap& filemap, const std::string& indexHtml,
            unsigned short port, const std::string& root,
            const std::unordered_map<std::string, std::string>& parameters, WindowType windowType);
        const GempyreInternal& ref() const;
        GempyreInternal& ref();
    private:
        friend class Element;
        std::unique_ptr<GempyreInternal> m_ui;
    };
}

#endif // GEMPYRE_H
