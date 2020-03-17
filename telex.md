![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

telex.h
=====
Telex GUI Framework
-------------

telex.h contains core functionality, everything that is needed for basic application using Telex framework.

There are two classes: The Ui creates a Ui framework, and Element represents visual on screen. Basically flow is
create Ui -&gt; intialize elements -&gt; start event loop. There are several type of constructor - Ui::Filemap are using
compile time encoded files. File are added using `addResource` CMake function:
```
addResource(  
PROJECT <PROJECT_NAME>  
TARGET <GENERATED_HEADER>  
SOURCES <LIST-OF-ENCODED_FILES>)  
```
a simple main goes as follows:

```
int main() {  
Telex::Ui ui({{"/index.html", Tickhtml}}, "index.html");  
Telex::Element header(ui, "header");  
ui.startTimer(10000ms, true, [header]() mutable {  
header.setHTML("time force");  
});  
ui.run();  
}  
```
Please note that UI is not created until event loop is running,
that means any Ui query is not possible before event loop is running.
To resolve issue there is onOpen callback that is called immediately
after event loop has been started.

* [ namespace Telex ](#telex)
  * [  void setDebug() ](#void-setdebug)
  * [ class Element ](#element)
    * [ Element(Ui&amp; ui, const std::string&amp; id) ](#elementui-ui-const-stdstring-id)
    * [ Element(Ui&amp; ui, const std::string&amp; id, const std::string&amp; htmlElement, const Element&amp; parent) ](#elementui-ui-const-stdstring-id-const-stdstring-htmlelement-const-element-parent)
    * [ const Ui&amp; ui() const ](#const-ui-ui-const)
    * [ Ui&amp; ui() ](#ui-ui)
    * [ std::string id() const ](#stdstring-id-const)
    * [ Element&amp; subscribe(const std::string&amp; name, std::function&lt;void(const Event&amp; ev)&gt; handler, const std::vector&lt;std::string&gt;&amp; properties = {}, const std::chrono::milliseconds&amp; = 0ms) ](#element-subscribeconst-stdstring-name-stdfunctionvoidconst-event-ev-handler-const-stdvectorstdstring-properties-const-stdchronomilliseconds-0ms)
    * [ Element&amp; setHTML(const std::string&amp; htmlText) ](#element-sethtmlconst-stdstring-htmltext)
    * [ Element&amp; setAttribute(const std::string&amp; attr, const std::string&amp; values) ](#element-setattributeconst-stdstring-attr-const-stdstring-values)
    * [ std::optional&lt;Attributes&gt; attributes() const ](#stdoptionalattributes-attributes-const)
    * [ std::optional&lt;Elements&gt; children() const ](#stdoptionalelements-children-const)
    * [ std::optional&lt;Values&gt; values() const ](#stdoptionalvalues-values-const)
    * [ std::optional&lt;std::string&gt; html() const ](#stdoptionalstdstring-html-const)
    * [ void remove() ](#void-remove)
    * [ std::optional&lt;std::string&gt; type() const ](#stdoptionalstdstring-type-const)
    * [ std::optional&lt;Rect&gt; rect() const ](#stdoptionalrect-rect-const)
  * [ class Ui ](#ui)
    * [ explicit Ui(const std::string&amp; indexHtml, const std::string&amp; browser, const std::string&amp; extraParams = &quot;&quot;, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) ](#explicit-uiconst-stdstring-indexhtml-const-stdstring-browser-const-stdstring-extraparams-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot)
    * [ explicit Ui(const std::string&amp; indexHtml, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) ](#explicit-uiconst-stdstring-indexhtml-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot)
    * [ explicit Ui(const Filemap&amp; filemap, const std::string&amp; indexHtml, const std::string&amp; browser, const std::string&amp; extraParams = &quot;&quot;, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) ](#explicit-uiconst-filemap-filemap-const-stdstring-indexhtml-const-stdstring-browser-const-stdstring-extraparams-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot)
    * [ explicit Ui(const Filemap&amp; filemap, const std::string&amp; indexHtml, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) ](#explicit-uiconst-filemap-filemap-const-stdstring-indexhtml-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot)
    * [ void exit() ](#void-exit)
    * [ void close() ](#void-close)
    * [ Ui&amp; onExit(std::function&lt;void ()&gt; onExitFunction = nullptr) ](#ui-onexitstdfunctionvoid-onexitfunction-nullptr)
    * [ Ui&amp; onReload(std::function&lt;void ()&gt; onReleadFunction = nullptr) ](#ui-onreloadstdfunctionvoid-onreleadfunction-nullptr)
    * [ Ui&amp; onOpen(std::function&lt;void ()&gt; onOpenFunction = nullptr) ](#ui-onopenstdfunctionvoid-onopenfunction-nullptr)
    * [ Ui&amp; onError(std::function&lt;void (const std::string&amp; element, const std::string&amp; info)&gt; onErrorFunction = nullptr) ](#ui-onerrorstdfunctionvoid-const-stdstring-element-const-stdstring-info-onerrorfunction-nullptr)
    * [ void run() ](#void-run)
    * [ void setLogging(bool logging) ](#void-setloggingbool-logging)
    * [ void eval(const std::string&amp; eval) ](#void-evalconst-stdstring-eval)
    * [ void debug(const std::string&amp; msg) ](#void-debugconst-stdstring-msg)
    * [ void alert(const std::string&amp; msg) ](#void-alertconst-stdstring-msg)
    * [ void open(const std::string&amp; url, const std::string&amp; name = &quot;&quot;) ](#void-openconst-stdstring-url-const-stdstring-name)
    * [ TimerId startTimer(const std::chrono::milliseconds&amp; ms, bool singleShot, const std::function&lt;void (TimerId id)&gt;&amp; timerFunc) ](#timerid-starttimerconst-stdchronomilliseconds-ms-bool-singleshot-const-stdfunctionvoid-timerid-id-timerfunc)
    * [ TimerId startTimer(const std::chrono::milliseconds&amp; ms, bool singleShot, const std::function&lt;void ()&gt;&amp; timerFunc) ](#timerid-starttimerconst-stdchronomilliseconds-ms-bool-singleshot-const-stdfunctionvoid-timerfunc)
    * [ bool stopTimer(TimerId) ](#bool-stoptimertimerid)
    * [ Element root() const ](#element-root-const)
    * [ std::string addressOf(const std::string&amp; filepath) const ](#stdstring-addressofconst-stdstring-filepath-const)
    * [ std::optional&lt;Element::Elements&gt; byClass(const std::string&amp; className) const ](#stdoptionalelementelements-byclassconst-stdstring-classname-const)
    * [ std::optional&lt;Element::Elements&gt; byName(const std::string&amp; className) const ](#stdoptionalelementelements-bynameconst-stdstring-classname-const)
    * [ std::optional&lt;std::pair&lt;std::chrono::microseconds, std::chrono::microseconds&gt;&gt; ping() const ](#stdoptionalstdpairstdchronomicroseconds-stdchronomicroseconds-ping-const)
    * [ std::optional&lt;std::any&gt; extension(const std::string&amp; callId, const std::unordered_map&lt;std::string, std::any&gt;&amp; parameters) ](#stdoptionalstdany-extensionconst-stdstring-callid-const-stdunordered_mapstdstring-stdany-parameters)
    * [ std::optional&lt;std::vector&lt;uint8_t&gt;&gt; resource(const std::string&amp; url) const ](#stdoptionalstdvectoruint8_t-resourceconst-stdstring-url-const)
    * [ bool addFile(const std::string&amp; url, const std::string&amp; file) ](#bool-addfileconst-stdstring-url-const-stdstring-file)
    * [ void beginBatch() ](#void-beginbatch)
    * [ void endBatch() ](#void-endbatch)


---
<a id="Telex"></a>
### Telex 
Common namespace for Telex implementation.
<a id="void-setdebug"></a>
#####  void setDebug() 
Enable debug outputs

---

---
<a id="Element"></a>
#### Telex::Element 
Element represents any UI element
<a id="elementui-ui-const-stdstring-id"></a>
##### Element(Ui&amp; ui, const std::string&amp; id) 
###### *Param:* ui 
###### *Param:* id 

Creates instance that refers to existing element.
<a id="elementui-ui-const-stdstring-id-const-stdstring-htmlelement-const-element-parent"></a>
##### Element(Ui&amp; ui, const std::string&amp; id, const std::string&amp; htmlElement, const Element&amp; parent) 
###### *Param:* ui 
###### *Param:* id 
###### *Param:* htmlElement 
###### *Param:* parent 

Creates a new elements as given HTML type and parent.
<a id="const-ui-ui-const"></a>
##### const Ui&amp; ui() const 
###### *Return:* Ui 

Return current Ui.
<a id="ui-ui"></a>
##### Ui&amp; ui() 
###### *Return:* Ui 

Return current Ui.
<a id="stdstring-id-const"></a>
##### std::string id() const 
###### *Return:* string 

HTML identifer of this element
<a id="element-subscribeconst-stdstring-name-stdfunctionvoidconst-event-ev-handler-const-stdvectorstdstring-properties-const-stdchronomilliseconds-0ms"></a>
##### Element&amp; subscribe(const std::string&amp; name, std::function&lt;void(const Event&amp; ev)&gt; handler, const std::vector&lt;std::string&gt;&amp; properties = {}, const std::chrono::milliseconds&amp; = 0ms) 
###### *Param:* name 
###### *Param:* handler 
###### *Param:* throttle 
###### *Return:* Element 

Set a function to listen ui event.
<a id="element-sethtmlconst-stdstring-htmltext"></a>
##### Element&amp; setHTML(const std::string&amp; htmlText) 
###### *Param:* name 
###### *Param:* handler 
###### *Return:* Element 

Set HTML content for element.
<a id="element-setattributeconst-stdstring-attr-const-stdstring-values"></a>
##### Element&amp; setAttribute(const std::string&amp; attr, const std::string&amp; values) 
###### *Param:* attr 
###### *Param:* values 
###### *Return:* Element 

Set a given attribute a given name
<a id="stdoptionalattributes-attributes-const"></a>
##### std::optional&lt;Attributes&gt; attributes() const 
###### *Return:* optional Attributes 

Return Attributes.
<a id="stdoptionalelements-children-const"></a>
##### std::optional&lt;Elements&gt; children() const 
###### *Return:* optional Elements 

Return child elements (only direct children).
<a id="stdoptionalvalues-values-const"></a>
##### std::optional&lt;Values&gt; values() const 
###### *Return:* optional Values 

Return values. (e.g. input values).
<a id="stdoptionalstdstring-html-const"></a>
##### std::optional&lt;std::string&gt; html() const 
###### *Return:* optional string 

Return html content of this element.
<a id="void-remove"></a>
##### void remove() 

Remove this element from Ui.
<a id="stdoptionalstdstring-type-const"></a>
##### std::optional&lt;std::string&gt; type() const 
###### *Return:* optiona string. 

Return type as HTML tag in lower case, empty is not found.
<a id="stdoptionalrect-rect-const"></a>
##### std::optional&lt;Rect&gt; rect() const 
###### *Return:* optional Rect 

Return bounding rect of element

---

---
<a id="Ui"></a>
#### Telex::Ui 
<a id="explicit-uiconst-stdstring-indexhtml-const-stdstring-browser-const-stdstring-extraparams-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot"></a>
##### explicit Ui(const std::string&amp; indexHtml, const std::string&amp; browser, const std::string&amp; extraParams = &quot;&quot;, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) 
###### *Param:* indexHtml 
###### *Param:* browser 
###### *Param:* extraParams 
###### *Param:* port, has default 
###### *Param:* root, has default 

Constructor
<a id="explicit-uiconst-stdstring-indexhtml-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot"></a>
##### explicit Ui(const std::string&amp; indexHtml, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) 
###### *Param:* indexHtml 
###### *Param:* port, has default 
###### *Param:* root, has default 

Constructor
<a id="explicit-uiconst-filemap-filemap-const-stdstring-indexhtml-const-stdstring-browser-const-stdstring-extraparams-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot"></a>
##### explicit Ui(const Filemap&amp; filemap, const std::string&amp; indexHtml, const std::string&amp; browser, const std::string&amp; extraParams = &quot;&quot;, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) 
###### *Param:* filemap 
###### *Param:* indexHtml 
###### *Param:* browser 
###### *Param:* extraParams, has defaul 
###### *Param:* port, has default 
###### *Param:* root, has default 

Constructor
<a id="explicit-uiconst-filemap-filemap-const-stdstring-indexhtml-unsigned-short-port-usedefaultport-const-stdstring-root-usedefaultroot"></a>
##### explicit Ui(const Filemap&amp; filemap, const std::string&amp; indexHtml, unsigned short port = UseDefaultPort, const std::string&amp; root = UseDefaultRoot) 
###### *Param:* filemap 
###### *Param:* indexHtml 
###### *Param:* port, has default 
###### *Param:* root, has default 

Constructor
<a id="void-exit"></a>
##### void exit() 

Exits the eventloop and make application to close.
<a id="void-close"></a>
##### void close() 

Asks Client window to close (which then signals application to close)
<a id="ui-onexitstdfunctionvoid-onexitfunction-nullptr"></a>
##### Ui&amp; onExit(std::function&lt;void ()&gt; onExitFunction = nullptr) 
###### *Param:* onExitFunction 
###### *Return:* Ui 

Callback just before exit.
<a id="ui-onreloadstdfunctionvoid-onreleadfunction-nullptr"></a>
##### Ui&amp; onReload(std::function&lt;void ()&gt; onReleadFunction = nullptr) 
###### *Param:* onReleadFunction 
###### *Return:* Ui 

Callback when browser window reload occurs.
<a id="ui-onopenstdfunctionvoid-onopenfunction-nullptr"></a>
##### Ui&amp; onOpen(std::function&lt;void ()&gt; onOpenFunction = nullptr) 
###### *Param:* onOpenFunction 
###### *Return:* Ui 

Callback when browser UI is running.
<a id="ui-onerrorstdfunctionvoid-const-stdstring-element-const-stdstring-info-onerrorfunction-nullptr"></a>
##### Ui&amp; onError(std::function&lt;void (const std::string&amp; element, const std::string&amp; info)&gt; onErrorFunction = nullptr) 
###### *Param:* onErrorFunction 
###### *Return:* Ui 

Browser reports an issue
<a id="void-run"></a>
##### void run() 

Starts eventloop
<a id="void-setloggingbool-logging"></a>
##### void setLogging(bool logging) 
###### *Param:* logging 

Enforces Browser being verbose when serving Telex.
<a id="void-evalconst-stdstring-eval"></a>
##### void eval(const std::string&amp; eval) 
###### *Param:* eval 

Execute code on browser (calls JS eval)
<a id="void-debugconst-stdstring-msg"></a>
##### void debug(const std::string&amp; msg) 
###### *Param:* msg 

Echoes a message
<a id="void-alertconst-stdstring-msg"></a>
##### void alert(const std::string&amp; msg) 
###### *Param:* msg 

Shown a browser&#39;s alert dialogue
<a id="void-openconst-stdstring-url-const-stdstring-name"></a>
##### void open(const std::string&amp; url, const std::string&amp; name = &quot;&quot;) 
###### *Param:* url 
###### *Param:* name 

Opens an extrenal URL on browser tab
<a id="timerid-starttimerconst-stdchronomilliseconds-ms-bool-singleshot-const-stdfunctionvoid-timerid-id-timerfunc"></a>
##### TimerId startTimer(const std::chrono::milliseconds&amp; ms, bool singleShot, const std::function&lt;void (TimerId id)&gt;&amp; timerFunc) 
###### *Param:* ms 
###### *Param:* singleShot 
###### *Param:* timerFunc 
###### *Return:* TimerId 

Starts a timer that is called after given amount of milliseconds.
<a id="timerid-starttimerconst-stdchronomilliseconds-ms-bool-singleshot-const-stdfunctionvoid-timerfunc"></a>
##### TimerId startTimer(const std::chrono::milliseconds&amp; ms, bool singleShot, const std::function&lt;void ()&gt;&amp; timerFunc) 
###### *Param:* ms 
###### *Param:* singleShot 
###### *Param:* timerFunc 
###### *Return:* TimerId 

Starts a timer that is called after given amount of milliseconds.
<a id="bool-stoptimertimerid"></a>
##### bool stopTimer(TimerId) 
###### *Return:* Boolean 

Stop the timer.
<a id="element-root-const"></a>
##### Element root() const 
###### *Return:* Element 

Pseudo element that represents root of the element structure
<a id="stdstring-addressofconst-stdstring-filepath-const"></a>
##### std::string addressOf(const std::string&amp; filepath) const 
###### *Param:* filepath 
###### *Return:* string 

Translates given path to address that Telex can read when provided as a link.
<a id="stdoptionalelementelements-byclassconst-stdstring-classname-const"></a>
##### std::optional&lt;Element::Elements&gt; byClass(const std::string&amp; className) const 
###### *Param:* className 
###### *Return:* optional list of Elements 

Returns all elements match to given class
<a id="stdoptionalelementelements-bynameconst-stdstring-classname-const"></a>
##### std::optional&lt;Element::Elements&gt; byName(const std::string&amp; className) const 
###### *Param:* className 
###### *Return:* optional list of Elements. 

Returns all elements match to given name
<a id="stdoptionalstdpairstdchronomicroseconds-stdchronomicroseconds-ping-const"></a>
##### std::optional&lt;std::pair&lt;std::chrono::microseconds, std::chrono::microseconds&gt;&gt; ping() const 
###### *Return:* optional pair of times. 

Just a ping.
<a id="stdoptionalstdany-extensionconst-stdstring-callid-const-stdunordered_mapstdstring-stdany-parameters"></a>
##### std::optional&lt;std::any&gt; extension(const std::string&amp; callId, const std::unordered_map&lt;std::string, std::any&gt;&amp; parameters) 
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
<a id="stdoptionalstdvectoruint8_t-resourceconst-stdstring-url-const"></a>
##### std::optional&lt;std::vector&lt;uint8_t&gt;&gt; resource(const std::string&amp; url) const 
###### *Param:* url 
###### *Return:* optional byte vector 

Get data stored as a resource
<a id="bool-addfileconst-stdstring-url-const-stdstring-file"></a>
##### bool addFile(const std::string&amp; url, const std::string&amp; file) 
###### *Param:* url 
###### *Param:* file 
###### *Return:* boolean 

Adds a file as a resources
<a id="void-beginbatch"></a>
##### void beginBatch() 

Starts buffering non-DataPtr message locally
<a id="void-endbatch"></a>
##### void endBatch() 

Sends locally buffered message to UI

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2020 
