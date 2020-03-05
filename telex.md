![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Telex
=====
GUI Framework
-------------

telex.h contains core functionality, everything that is needed for basic application using Telex framework.


* [ namespace Telex ](#telex)
  * [  void setDebug() ](#-void-setdebug-)
  * [ class Element ](#element)
    * [ Element(Ui& ui, const std::string& id) ](#element-ui-ui-const-std-string-id-)
    * [ Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) ](#element-ui-ui-const-std-string-id-const-std-string-htmlelement-const-element-parent-)
    * [ const Ui& ui() const ](#const-ui-ui-const)
    * [ Ui& ui() ](#ui-ui-)
    * [ std::string id() const ](#std-string-id-const)
    * [ Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& = 0ms) ](#element-subscribe-const-std-string-name-std-functionvoid-const-event-ev-handler-const-std-vectorstd-string-properties-const-std-chrono-milliseconds-0ms-)
    * [ Element& setHTML(const std::string& htmlText) ](#element-sethtml-const-std-string-htmltext-)
    * [ Element& setAttribute(const std::string& attr, const std::string& values) ](#element-setattribute-const-std-string-attr-const-std-string-values-)
    * [ std::optional<Attributes> attributes() const ](#std-optionalattributes-attributes-const)
    * [ std::optional<Elements> children() const ](#std-optionalelements-children-const)
    * [ std::optional<Values> values() const ](#std-optionalvalues-values-const)
    * [ std::optional<std::string> html() const ](#std-optionalstd-string-html-const)
    * [ void remove() ](#void-remove-)
    * [ std::optional<std::string> type() const ](#std-optionalstd-string-type-const)
    * [ std::optional<Rect> rect() const ](#std-optionalrect-rect-const)
  * [ class Ui ](#ui)
    * [ explicit Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot) ](#explicit-ui-const-std-string-indexhtml-const-std-string-browser-const-std-string-extraparams-unsigned-short-port-usedefaultport-const-std-string-root-usedefaultroot-)
    * [ explicit Ui(const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot) ](#explicit-ui-const-std-string-indexhtml-unsigned-short-port-usedefaultport-const-std-string-root-usedefaultroot-)
    * [ explicit Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot) ](#explicit-ui-const-filemap-filemap-const-std-string-indexhtml-const-std-string-browser-const-std-string-extraparams-unsigned-short-port-usedefaultport-const-std-string-root-usedefaultroot-)
    * [ explicit Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot) ](#explicit-ui-const-filemap-filemap-const-std-string-indexhtml-unsigned-short-port-usedefaultport-const-std-string-root-usedefaultroot-)
    * [ void exit() ](#void-exit-)
    * [ void close() ](#void-close-)
    * [ Ui& onUiExit(std::function<void ()> onExitFunction = nullptr) ](#ui-onuiexit-std-functionvoid-onexitfunction-nullptr-)
    * [ Ui& onReload(std::function<void ()> onReleadFunction = nullptr) ](#ui-onreload-std-functionvoid-onreleadfunction-nullptr-)
    * [ Ui& onOpen(std::function<void ()> onOpenFunction = nullptr) ](#ui-onopen-std-functionvoid-onopenfunction-nullptr-)
    * [ Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction = nullptr) ](#ui-onerror-std-functionvoid-const-std-string-element-const-std-string-info-onerrorfunction-nullptr-)
    * [ void run() ](#void-run-)
    * [ void setLogging(bool logging) ](#void-setlogging-bool-logging-)
    * [ void eval(const std::string& eval) ](#void-eval-const-std-string-eval-)
    * [ void debug(const std::string& msg) ](#void-debug-const-std-string-msg-)
    * [ void alert(const std::string& msg) ](#void-alert-const-std-string-msg-)
    * [ void open(const std::string& url, const std::string& name = "") ](#void-open-const-std-string-url-const-std-string-name-)
    * [ TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc) ](#timerid-starttimer-const-std-chrono-milliseconds-ms-bool-singleshot-const-std-functionvoid-timerid-id-timerfunc-)
    * [ TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc) ](#timerid-starttimer-const-std-chrono-milliseconds-ms-bool-singleshot-const-std-functionvoid-timerfunc-)
    * [ bool stopTimer(TimerId) ](#bool-stoptimer-timerid-)
    * [ Element root() const ](#element-root-const)
    * [ std::string addressOf(const std::string& filepath) const ](#std-string-addressof-const-std-string-filepath-const)
    * [ std::optional<Element::Elements> byClass(const std::string& className) const ](#std-optionalelement-elements-byclass-const-std-string-classname-const)
    * [ std::optional<Element::Elements> byName(const std::string& className) const ](#std-optionalelement-elements-byname-const-std-string-classname-const)
    * [ std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const ](#std-optionalstd-pairstd-chrono-microseconds-std-chrono-microseconds-ping-const)
    * [ std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) ](#std-optionalstd-any-extension-const-std-string-callid-const-std-unordered_mapstd-string-std-any-parameters-)
    * [ std::optional<std::vector<uint8_t>> resource(const std::string& url) const ](#std-optionalstd-vectoruint8_t-resource-const-std-string-url-const)
    * [ bool addFile(const std::string& url, const std::string& file) ](#bool-addfile-const-std-string-url-const-std-string-file-)
    * [ void beginBatch() ](#void-beginbatch-)
    * [ void endBatch() ](#void-endbatch-)


---
#####
Telex contains three header files
* `telex.h`
* `telex_utils.h`
* `telex_graphics.h`

---

---
### Telex

Common namespace for Telex implementation.
#####  void setDebug()
Enable debug outputs

---

---
#### Telex::Element

Element represents any UI element
##### Element(Ui& ui, const std::string& id)
###### *Param:* ui
###### *Param:* id

Creates instance that refers to existing element.
##### Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent)
###### *Param:* ui
###### *Param:* id
###### *Param:* htmlElement
###### *Param:* parent

Creates a new elements as given HTML type and parent.
##### const Ui& ui() const
###### *Return:* Ui

Return current Ui.
##### Ui& ui()
###### *Return:* Ui

Return current Ui.
##### std::string id() const
###### *Return:* string

HTML identifer of this element
##### Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& = 0ms)
###### *Param:* name
###### *Param:* handler
###### *Param:* throttle
###### *Return:* Element

Set a function to listen ui event.
##### Element& setHTML(const std::string& htmlText)
###### *Param:* name
###### *Param:* handler
###### *Return:* Element

Set HTML content for element.
##### Element& setAttribute(const std::string& attr, const std::string& values)
###### *Param:* attr
###### *Param:* values
###### *Return:* Element

Set a given attribute a given name
##### std::optional<Attributes> attributes() const
###### *Return:* optional Attributes

Return Attributes.
##### std::optional<Elements> children() const
###### *Return:* optional Elements

Return child elements (only direct children).
##### std::optional<Values> values() const
###### *Return:* optional Values

Return values. (e.g. input values).
##### std::optional<std::string> html() const
###### *Return:* optional string

Return html content of this element.
##### void remove()

Remove this element from Ui.
##### std::optional<std::string> type() const
###### *Return:* optiona string.

Return type as HTML tag in lower case, empty is not found.
##### std::optional<Rect> rect() const
###### *Return:* optional Rect

Return bounding rect of element

---

---
#### Telex::Ui
##### explicit Ui(const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot)
###### *Param:* indexHtml
###### *Param:* browser
###### *Param:* extraParams
###### *Param:* port, has default
###### *Param:* root, has default

Constructor
##### explicit Ui(const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot)
###### *Param:* indexHtml
###### *Param:* port, has default
###### *Param:* root, has default

Constructor
##### explicit Ui(const Filemap& filemap, const std::string& indexHtml, const std::string& browser, const std::string& extraParams = "", unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot)
###### *Param:* filemap
###### *Param:* indexHtml
###### *Param:* browser
###### *Param:* extraParams, has defaul
###### *Param:* port, has default
###### *Param:* root, has default

Constructor
##### explicit Ui(const Filemap& filemap, const std::string& indexHtml, unsigned short port = UseDefaultPort, const std::string& root = UseDefaultRoot)
###### *Param:* filemap
###### *Param:* indexHtml
###### *Param:* port, has default
###### *Param:* root, has default

Constructor
##### void exit()

Exits the eventloop and make application to close.
##### void close()

Asks Client window to close (which then signals application to close)
##### Ui& onUiExit(std::function<void ()> onExitFunction = nullptr)
###### *Param:* onExitFunction
###### *Return:* Ui

Callback just before exit.
##### Ui& onReload(std::function<void ()> onReleadFunction = nullptr)
###### *Param:* onReleadFunction
###### *Return:* Ui

Callback when browser window reload occurs.
##### Ui& onOpen(std::function<void ()> onOpenFunction = nullptr)
###### *Param:* onOpenFunction
###### *Return:* Ui

Callback when browser UI is running.
##### Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction = nullptr)
###### *Param:* onErrorFunction
###### *Return:* Ui

Browser reports an issue
##### void run()

Starts eventloop
##### void setLogging(bool logging)
###### *Param:* logging

Enforces Browser being verbose when serving Telex.
##### void eval(const std::string& eval)
###### *Param:* eval

Execute code on browser (calls JS eval)
##### void debug(const std::string& msg)
###### *Param:* msg

Echoes a message
##### void alert(const std::string& msg)
###### *Param:* msg

Shown a browser&amp;#39;s alert dialogue
##### void open(const std::string& url, const std::string& name = "")
###### *Param:* url
###### *Param:* name

Opens an extrenal URL on browser tab
##### TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc)
###### *Param:* ms
###### *Param:* singleShot
###### *Param:* timerFunc
###### *Return:* TimerId

Starts a timer that is called after given amount of milliseconds.
##### TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc)
###### *Param:* ms
###### *Param:* singleShot
###### *Param:* timerFunc
###### *Return:* TimerId

Starts a timer that is called after given amount of milliseconds.
##### bool stopTimer(TimerId)
###### *Return:* Boolean

Stop the timer.
##### Element root() const
###### *Return:* Element

Pseudo element that represents root of the element structure
##### std::string addressOf(const std::string& filepath) const
###### *Param:* filepath
###### *Return:* string

Translates given path to address that Telex can read when provided as a link.
##### std::optional<Element::Elements> byClass(const std::string& className) const
###### *Param:* className
###### *Return:* optional list of Elements

Returns all elements match to given class
##### std::optional<Element::Elements> byName(const std::string& className) const
###### *Param:* className
###### *Return:* optional list of Elements.

Returns all elements match to given name
##### std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const
###### *Return:* optional pair of times.

Just a ping.
##### std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters)
###### *Param:* callId
###### *Param:* parameters
###### *Return:* optional any

Low level access to extension services that browser may implement. The return parameter and return value are JSON kind
of structures or types and depends on given callId.
Note if return value contain a string it is very straighforward

for example
```
const auto out = ui.extension("openFile", {{"caption", "hexview - open"}});
const std::string filename = std::any_cast<std::string>(*out);
```
but if is an array of string values must be converted accordint to structure
```
const auto out = ui.extension("openFile", {{"caption", "hexview - open"}});
const auto anyvec = std::any_cast<std::vector<std::any>(*out);
std::vector<std::string> vec;
std::transform(anyvec.begin(), anyvec.end, std::back_inserter(vec), [](const auto& a){return std::any_cast<std::string>(a)});
```
##### std::optional<std::vector<uint8_t>> resource(const std::string& url) const
###### *Param:* url
###### *Return:* optional byte vector

Get data stored as a resource
##### bool addFile(const std::string& url, const std::string& file)
###### *Param:* url
###### *Param:* file
###### *Return:* boolean

Adds a file as a resources
##### void beginBatch()

Starts buffering non-DataPtr message locally
##### void endBatch()

Sends locally buffered message to UI

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2020
