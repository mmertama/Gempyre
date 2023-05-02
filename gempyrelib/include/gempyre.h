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
  * @file 
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

/// @cond INTERNAL
#ifdef WINDOWS_EXPORT
#define GEMPYRE_EX __declspec( dllexport )
#else
#define GEMPYRE_EX
#endif
/// @endcond


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

    /// set debugging on/off
    GEMPYRE_EX void set_debug(bool isDebug = true);
   
    /// Internal for Android
    GEMPYRE_EX void setJNIENV(void* env, void* obj);
    /// Return current version
    GEMPYRE_EX std::tuple<int, int, int> version();

    class Data;

     /// @cond INTERNAL
    using DataPtr = std::shared_ptr<Data>;
    using dataT = uint32_t;
    /// @endcond

    struct Event;

    /// @brief Represents all HTML elements on UI
    class GEMPYRE_EX Element {
    public:
        /// @brief Attribute key, value pairs.
        using Attributes = std::unordered_map<std::string, std::string>;
        /// @brief Value key, value pairs.
        using Values = std::unordered_map<std::string, std::string>;
        /// @brief Vector of Elements.
        using Elements = std::vector<Element>;
        /// @brief Callback function for event subscriptions. @see Element::subscribe.
        using SubscribeFunction = std::function<void(const Event&)>;

        /// @brief Rect
        struct Rect {
            /// @brief rectangle x coordinate.
            int x;
            /// @brief rectangle y coordinate.
            int y;
            /// @brief rectangle width.
            int width;
            /// @brief rectangle height.
            int height;
        };
        
    public:
        /// Copy constructor.
        Element(const Element& other) = default;
        /// Move constructor.
        Element(Element&& other) = default;
        /// Copy operator. 
        Element& operator=(const Element& other) {m_ui = other.m_ui; m_id = other.m_id; return *this;}
        /// Move operator
        Element& operator=(Element&& other) {m_ui = other.m_ui; m_id = std::move(other.m_id); return *this;}

        /// @brief Constructor for existing elements
        /// @param ui - ui ref
        /// @param id - if of element assumed to exists in HTML document (or DOM)
        /// @details Constructor is very lightweight and many cases its easier
        /// to create a new element rather than copy or move one.
        Element(Ui& ui, const std::string& id);
        /// @brief Constructor for exiting elements
        /// @param ui - ui ref
        /// @param id - assumed be unique, if id is not important to preset, use constructor without id.
        /// @param htmlElement - element type
        /// @param parent - parent element where this element is created
        Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent);
        /// @brief Constructor for exiting elements
        /// @param ui - ui ref
        /// @param htmlElement - element type
        /// @param parent - parent element where this element is created
        Element(Ui& ui, const std::string& htmlElement, const Element& parent);
        /// Destructor
        virtual ~Element();
        /// Get Ui
        [[nodiscard]] const Ui& ui() const { return *m_ui; }
        /// Get Ui 
        [[nodiscard]] Ui& ui() { return *m_ui;}
        /// Get id of element
        [[nodiscard]] std::string id() const {return m_id;}
        /// @brief Subscribe UI event
        /// @param name - name of the event. 
        /// @param handler - function executed on event.
        /// @param properties - optional, event properties to listen.
        /// @param throttle - optional, throttle callback calls
        /// @return this element
        /// @details Listen element events. The callback properties is populated only with values listed in properties parameter.
        /// Some events (like mouse move) can emit so often that it would impact to performance, that can be eased
        /// with a suitable throttle value. If two (or more) messages are received in shorted period than throttle value, only the
        /// last is received.
        Element& subscribe(const std::string& name, const SubscribeFunction& handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& throttle = 0ms);
        /// @brief Set HTML text value of the element
        /// @param htmlText - HTML encoded string
        /// @return this element
        Element& set_html(const std::string& htmlText);
        /// @brief Set HTML a attribute of this element
        /// @param attr - attribute name
        /// @param value - attribute value
        /// @return this element
        Element& set_attribute(const std::string& attr, const std::string& value = "");
        /// Get this element attributes 
        std::optional<Attributes> attributes() const;
        /// @brief Set CSS style of this element
        /// @param style - style name 
        /// @param value - CSS style value
        /// @return this element
        Element& set_style(const std::string& style, const std::string& value);
        /// @brief Remove attribute
        /// @param attr - attribute name
        /// @return this Element
        Element& remove_attribute(const std::string& attr);
        /// @brief Get element styles
        /// @param keys - style keys to fetch.
        [[nodiscard]] std::optional<Values> styles(const std::vector<std::string>& keys) const;
        /// Get element children
        [[nodiscard]] std::optional<Elements> children() const;
        /// Applies to form elements only - receive values bound to the element
        [[nodiscard]] std::optional<Values> values() const;
        /// Get HTML value bound to this element (does not apply all elements)
        [[nodiscard]] std::optional<std::string> html() const;
        /// Remove this element from UI
        void remove();
        /// Get this element type, mostly a HTML tag.
        [[nodiscard]] std::optional<std::string> type() const;
        /// Get this element UI rect. I.e area it occupies on screen (if applicable)
        [[nodiscard]] std::optional<Rect> rect() const;
    protected:
    /// @cond INTERNAL    
        const GempyreInternal& ref() const;
        GempyreInternal& ref();
        static const std::string generateId(const std::string& prefix);        
    protected:
        Ui* m_ui;
        std::string m_id;
    /// @endcond    
    private:   
        friend class GempyreInternal;
    };

    /// @brief Event received
    struct Event {
        /// Mouse moving
        static constexpr auto MOUSE_MOVE = "mousemove";
        /// Mouse button up
        static constexpr auto  MOUSE_UP = "mouseup";
        /// Mouse button down
        static constexpr auto  MOUSE_DOWN = "mousedown";
        /// Click event
        static constexpr auto  MOUSE_CLICK = "click";
        /// Mouse button double click 
        static constexpr auto  MOUSE_DBLCLICK = "dblclick";
        /// Key up
        static constexpr auto  KEY_UP = "keyup";
        /// Key pressed
        static constexpr auto  KEY_PRESS = "keypress";
        /// Key down
        static constexpr auto  KEY_DOWN = "keydown";
        /// Scroll
        static constexpr auto  SCROLL = "scroll";
        /// Generic click
        static constexpr auto  CLICK = "click";
        /// Generic change
        static constexpr auto  CHANGE = "change";
        /// Generic select
        static constexpr auto  SELECT = "select";
        /// On focus 
        static constexpr auto  FOCUS = "focus";
        /// Off focus
        static constexpr auto  BLUR = "blur";
        /// In Focus
        static constexpr auto  FOCUS_IN = "focusin";
        /// Focus Out
        static constexpr auto  FOCUS_OUT = "focusout";
        /// Load
        static constexpr auto  LOAD = "load";
       
        /// @brief element that has emitted the event, the same that did subscription.
        Element element;
        /// @brief List of requested properties. @see Element::subscribe()
        std::unordered_map<std::string, std::string> properties;
    };

    /// @brief The application UI 
    class GEMPYRE_EX Ui {
    public:
        /// UI flags for a Window UI
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
        
        /// Resource map type
        using Filemap = std::unordered_map<std::string, std::string>;

        /// @cond INTERNAL
        using TimerId = int;
        static constexpr unsigned short UseDefaultPort = 0; //zero means default port
        static constexpr auto UseDefaultRoot = "";   //zero means default root
        /// @endcond
        
        /// @brief  Create UI using default ui app or gempyre.conf
        /// @param filemap resource map used
        /// @param indexHtml page to show
        /// @param port optional
        /// @param root optional
        Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

        /// @brief Create a browser UI using given ui app and command line.
        /// @param filemap resource map used.
        /// @param indexHtml page to show.
        /// @param browser browser to use.
        /// @param browser_params params passed to browse.
        /// @param port optional
        /// @param root optional
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser,  const std::string& browser_params, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

        /// @brief Create a window UI
        /// @param filemap resource map used.
        /// @param indexHtml page to show.
        /// @param title window title.
        /// @param width window width - this may be less than a window client area.
        /// @param height window height - this may be less than a window client area. 
        /// @param flags flags passed to the window - see UiFlags.
        /// @param ui_params extra parameters passed to the window.
        /// @param port optional
        /// @param root optional
        Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& title,  int width, int height, unsigned flags = 0,
            const std::unordered_map<std::string, std::string>& ui_params = {}, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot);

        /// Destructor.
        ~Ui();
        Ui(const Ui& other) = delete;
        Ui(Ui&& other) = delete;

        ///Application gracefully finish the event loop and exits.
        void exit();
        ///Requires Client window to close (that cause the application to close).
        [[deprecated("Prefer exit")]] void close();

        /// @brief Function called on exit.
        using ExitFunction = std::function<void ()>;
        /// @brief Function called on reload. (page reload)
        using ReloadFunction = std::function<void ()>;
        /// @brief Function called on open.x
        using OpenFunction = std::function<void ()>;
        /// @brief Function called on UI error.
        using ErrorFunction = std::function<void (const std::string& element, const std::string& info)>;


        /// @brief The callback is called before before the eventloop exit.
        /// @param onExitFunction 
        /// @return previous exit function.
        ExitFunction on_exit(const ExitFunction& onExitFunction);

        /// @brief The callback is called on UI reload.
        /// @param onReloadFunction 
        /// @return previous reload function.
        ReloadFunction on_reload(const ReloadFunction& onReloadFunction);

        /// @brief The callback is called on UI open.
        /// @param onOpenFunction 
        /// @return previous open function
        /// @details this function is 1st function that is called when UI is initialized. 
        /// Hence it can used for tasks that requires fetching information from UI. @see run.
        OpenFunction on_open(const OpenFunction& onOpenFunction);
        /// @brief The callback called on UI error.
        /// @param onErrorFunction 
        /// @return previous error function
        ErrorFunction on_error(const ErrorFunction& onErrorFunction);

        /// @brief Starts the event loop.
        /// @details The window or browser UI is called only after 'run' and callbacks can be received.
        /// this also means that no queries can be done before, therefore it is suggested that initialization
        /// of UI is finalished in @see onOpenFunction callback. 'run' return on UI exit.  
        void run();

        /// @brief Set browser to verbose mode
        /// @param logging 
        void set_logging(bool logging);
        /// @brief Executes eval string in UI context.
        /// @param eval Javascript to be executed.
        void eval(const std::string& eval);
        ///Send a debug message to UI. Message is get received is set_logging is true.
        void debug(const std::string& msg);
        ///Show an alert window.
        void alert(const std::string& msg);
        /// @brief Opens an url in the UI view
        /// @param url address
        /// @param name title
        void open(const std::string& url, const std::string& name = "");

        /// @brief Start a periodic timer
        /// @param ms period.
        /// @param timerFunc callback  
        /// @return timer id that can be used to cancel this timer.
        TimerId start_periodic(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);
        /// @brief Start a periodic timer
        /// @param ms period.
        /// @param timerFunc callback  
        /// @return timer id that can be used to cancel this timer.
        TimerId start_periodic(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        /// @brief Starts a single shot timer.
        /// @param ms period
        /// @param timerFunc - callback 
        /// @return timer id that can be used to cancel this timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void (TimerId id)>& timerFunc);

        /// @brief Starts a single shot timer.
        /// @param ms period
        /// @param timerFunc - callback 
        /// @return timer id that can be used to cancel this timer.
        TimerId after(const std::chrono::milliseconds& ms, const std::function<void ()>& timerFunc);

        /// Stop a timer.
        bool cancel_timer(TimerId timerId);


        /// Get a (virtual) root element.
        [[nodiscard]] Element root() const;
        
        /// Get a local file path an URL, can be used with open.
        [[nodiscard]] std::string address_of(const std::string& filepath) const;
        
        /// Get elements by class name
        [[nodiscard]] std::optional<Element::Elements> by_class(const std::string& className) const;
        
        /// Get elements by name
        [[nodiscard]] std::optional<Element::Elements> by_name(const std::string& className) const;
        
        /// Test function to measure round trip time
        [[nodiscard]] std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const;
        
        /// @cond INTERNAL
        // do extension call - document upon request
        void extension_call(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);
        // get value from extension - document upon request
        [[nodiscard]] std::optional<std::any> extension_get(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters);
        /// @endcond

        /// @brief Read a resource.
        /// @param url resource name.
        /// @return resource as bytes.
        [[nodiscard]] std::optional<std::vector<uint8_t>> resource(const std::string& url) const;
        
        /// @brief Add a file data into Gempyre to be accessed via url.
        /// @param url string bound to data.
        /// @param file filename to read.
        /// @return success.
        bool add_file(const std::string& url, const std::string& file);

        /// @brief Add file data into map to be added as a map.
        /// @param map resource data.
        /// @param filename filename to read.
        /// @return name bound to the file 
        static std::optional<std::string> add_file(Filemap& map, const std::string& filename);
        
        /// Starts an UI write batch, no messages are sent to USER until endBatch
        void begin_batch();
        
        /// Ends an UI read batch, push all stored messages at once.
        void end_batch();
        
        /// Set all timers to hold. Can be used to pause UI actions.
        void set_timer_on_hold(bool on_hold);
        
        /// Tells if timers are on hold.
        [[nodiscard]] bool is_timer_on_hold() const;
        
        /// Get an native UI device pixel ratio.
        [[nodiscard]] std::optional<double> device_pixel_ratio() const;
        
        /// Set application icon, fail silently if backend wont support
        void set_application_icon(const uint8_t* data, size_t dataLen, const std::string& type);
        
        /// Resize, fail silently if backend wont support
        void resize(int width, int height);
        
        /// Set title, fail silently if backend wont support
        void set_title(const std::string& name);
        
        /// Read list files as a maps
        static Ui::Filemap to_file_map(const std::vector<std::string>& filenames);

        /// @cond INTERNAL
        // for testing
        void resume();
        // for testing
        void suspend();
        /// @endcond
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
