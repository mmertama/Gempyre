![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

Telex
=====
GUI Framework
-------------

* [Telex ](#telex)
* [ void setDebug() ](#-void-setdebug-)
* [Data ](#data)
* [Element ](#element)
* [Element(Ui& ui, const std::string& id) ](#element-ui-ui-const-std-string-id-)
* [Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) ](#element-ui-ui-const-std-string-id-const-std-string-htmlelement-const-element-parent-)
* [const Ui& ui() const ](#const-ui-ui-const)
* [Ui& ui() ](#ui-ui-)
* [std::string id() const ](#std-string-id-const)
* [Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& = 0ms) ](#element-subscribe-const-std-string-name-std-functionvoid-const-event-ev-handler-const-std-vectorstd-string-properties-const-std-chrono-milliseconds-0ms-)
* [Element& setHTML(const std::string& htmlText) ](#element-sethtml-const-std-string-htmltext-)
* [Element& setAttribute(const std::string& attr, const std::string& values) ](#element-setattribute-const-std-string-attr-const-std-string-values-)
* [std::optional<Attributes> attributes() const ](#std-optionalattributes-attributes-const)
* [std::optional<Elements> children() const ](#std-optionalelements-children-const)
* [std::optional<Values> values() const ](#std-optionalvalues-values-const)
* [std::optional<std::string> html() const ](#std-optionalstd-string-html-const)
* [void remove() ](#void-remove-)
* [std::optional<std::string> type() const ](#std-optionalstd-string-type-const)
* [std::optional<Rect> rect() const ](#std-optionalrect-rect-const)
* [Ui ](#ui)
* [void exit() ](#void-exit-)
* [void close() ](#void-close-)
* [Ui& onUiExit(std::function<void ()> onExitFunction = nullptr) ](#ui-onuiexit-std-functionvoid-onexitfunction-nullptr-)
* [Ui& onReload(std::function<void ()> onReleadFunction = nullptr) ](#ui-onreload-std-functionvoid-onreleadfunction-nullptr-)
* [Ui& onOpen(std::function<void ()> onOpenFunction = nullptr) ](#ui-onopen-std-functionvoid-onopenfunction-nullptr-)
* [Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction = nullptr) ](#ui-onerror-std-functionvoid-const-std-string-element-const-std-string-info-onerrorfunction-nullptr-)
* [void run() ](#void-run-)
* [void setLogging(bool logging) ](#void-setlogging-bool-logging-)
* [void eval(const std::string& eval) ](#void-eval-const-std-string-eval-)
* [void debug(const std::string& msg) ](#void-debug-const-std-string-msg-)
* [void alert(const std::string& msg) ](#void-alert-const-std-string-msg-)
* [void open(const std::string& url, const std::string& name = "") ](#void-open-const-std-string-url-const-std-string-name-)
* [TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc) ](#timerid-starttimer-const-std-chrono-milliseconds-ms-bool-singleshot-const-std-functionvoid-timerid-id-timerfunc-)
* [TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc) ](#timerid-starttimer-const-std-chrono-milliseconds-ms-bool-singleshot-const-std-functionvoid-timerfunc-)
* [bool stopTimer(TimerId) ](#bool-stoptimer-timerid-)
* [Element root() const ](#element-root-const)
* [std::string addressOf(const std::string& filepath) const ](#std-string-addressof-const-std-string-filepath-const)
* [std::optional<Element::Elements> byClass(const std::string& className) const ](#std-optionalelement-elements-byclass-const-std-string-classname-const)
* [std::optional<Element::Elements> byName(const std::string& className) const ](#std-optionalelement-elements-byname-const-std-string-classname-const)
* [std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const ](#std-optionalstd-pairstd-chrono-microseconds-std-chrono-microseconds-ping-const)
* [std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) ](#std-optionalstd-any-extension-const-std-string-callid-const-std-unordered_mapstd-string-std-any-parameters-)

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
#### Data 
Generic container for data

---
#### Element 
Element is any UI element
##### Element(Ui& ui, const std::string& id) 
###### *Param:* ui 
###### *Param:* id 
Creates instance that refers to existing element
##### Element(Ui& ui, const std::string& id, const std::string& htmlElement, const Element& parent) 
###### *Param:* ui 
###### *Param:* id 
###### *Param:* htmlElement 
###### *Param:* parent 
Creates a new elements as given HTML type and parent
##### const Ui& ui() const 
###### *Return:* current Ui 
##### Ui& ui() 
###### *Return:*  
##### std::string id() const 
###### *Return:* HTML identifer of this element 
##### Element& subscribe(const std::string& name, std::function<void(const Event& ev)> handler, const std::vector<std::string>& properties = {}, const std::chrono::milliseconds& = 0ms) 
###### *Param:* name 
###### *Param:* handler 
###### *Param:* throttle 
set function to listen ui element
###### *Return:*  
##### Element& setHTML(const std::string& htmlText) 
###### *Param:* name 
###### *Param:* handler 
###### *Return:*  
##### Element& setAttribute(const std::string& attr, const std::string& values) 
###### *Param:* attr 
###### *Param:* values 
###### *Return:*  
Set a given attribute a given name
##### std::optional<Attributes> attributes() const 
###### *Return:*  
##### std::optional<Elements> children() const 
###### *Return:*  
All attributes this element has
##### std::optional<Values> values() const 
###### *Return:*  
Children ot this element (non-recursive)
##### std::optional<std::string> html() const 
###### *Return:*  
For input types returns all the values this element has
##### void remove() 
Remove this element from page
##### std::optional<std::string> type() const 
###### *Return:* a HTML tag in lower case, empty is not found, nullopt if page is not ready. 

##### std::optional<Rect> rect() const 
###### *Return:* bounding rect of element 

---
#### Ui 
##### void exit() 
Exits the eventloop and make application to close
##### void close() 
Asks Client window to close (which then signals application to close)
##### Ui& onUiExit(std::function<void ()> onExitFunction = nullptr) 
###### *Param:* onExitFunction 
###### *Return:*  
Callback just before exit
##### Ui& onReload(std::function<void ()> onReleadFunction = nullptr) 
###### *Param:* onReleadFunction 
###### *Return:*  
Callback when browser window reload occurs.
##### Ui& onOpen(std::function<void ()> onOpenFunction = nullptr) 
###### *Param:* onOpenFunction 
###### *Return:*  
Callback when browser UI is running.
##### Ui& onError(std::function<void (const std::string& element, const std::string& info)> onErrorFunction = nullptr) 
###### *Param:* onErrorFunction 
###### *Return:*  
Browser reports an issue
##### void run() 
Starts eventloop
##### void setLogging(bool logging) 
###### *Param:* logging 
Enforces Browser being verbose when serving Telex
##### void eval(const std::string& eval) 
###### *Param:* eval 
Execute code on browser (calls JS eval)
##### void debug(const std::string& msg) 
###### *Param:* msg 
Echoes a message
##### void alert(const std::string& msg) 
###### *Param:* msg 
Shown a browser&#39;s alert dialogue
##### void open(const std::string& url, const std::string& name = "") 
###### *Param:* url 
###### *Param:* name 
Opens an extrenal URL on browser tab
##### TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void (TimerId id)>& timerFunc) 
###### *Param:* ms 
###### *Param:* singleShot 
###### *Param:* timerFunc 
###### *Return:*  
##### TimerId startTimer(const std::chrono::milliseconds& ms, bool singleShot, const std::function<void ()>& timerFunc) 
###### *Param:* ms 
###### *Param:* singleShot 
###### *Param:* timerFunc 
###### *Return:*  
Starts a timer that is called after given amount of milliseconds
##### bool stopTimer(TimerId) 
###### *Return:*  
##### Element root() const 
###### *Return:*  
Pseudo element that represents root of the element structure
##### std::string addressOf(const std::string& filepath) const 
###### *Param:* filepath 
###### *Return:*  
Translates given path to address that Telex can read when provided as a link.
##### std::optional<Element::Elements> byClass(const std::string& className) const 
###### *Param:* className 
###### *Return:*  
Returns all elements match to given class
##### std::optional<Element::Elements> byName(const std::string& className) const 
###### *Param:* className 
###### *Return:*  
Returns all elements match to given name
##### std::optional<std::pair<std::chrono::microseconds, std::chrono::microseconds>> ping() const 
###### *Return:*  
Just a ping
##### std::optional<std::any> extension(const std::string& callId, const std::unordered_map<std::string, std::any>& parameters) 
###### *Param:* callId 
###### *Param:* parameters 
###### *Return:*  
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
###### resource 
###### *Param:* url 
###### *Return:*  
Get data stored as a resource
###### addFile 
###### *Param:* url 
###### *Param:* file 
###### *Return:*  
Adds a gile as a resources
###### beginBatch 
Starts
###### beginBatch 
Starts buffering non-DataPtr message locally
###### endBatch 
Sends locally buffered message to UI
###### Generated by MarkdownMaker, (c) Markus Mertama 2018 
