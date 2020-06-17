function not found 'function', C:/Users/intopalo/Documents/Telex-framework/telexlib/include/telex_graphics.h at 214 (ref:(288)<br/>
![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

telex_graphics.h
=====
Telex GUI Framework
-------------

telex_graphics.h provides a low level graphics capabilites for Telex. As Telex Element
provides access to HTML elements, their values and attributes - the bitmap graphics is
applied with inherited CanvasElement class. The bitmap can be a raw byte Canvas that can
be modified using RGBA pixels or image files. The image files can be added dynamically or
upon Ui construction.

See mandelbrot application for a bitmap example.
See imageblit for image drawing example.

* [ namespace Telex ](#telex)
  * [ class The Canvas class ](#the-canvas-class)
    * [ void put(int x, int y, dataT pixel) ](#void-putint-x-int-y-datat-pixel)
    * [ dataT get(int x, int y) const ](#datat-getint-x-int-y-const)
  * [ class CanvasElement ](#canvaselement)
    * [ CanvasElement(const CanvasElement&amp; other) : Element(other) ](#canvaselementconst-canvaselement-other-elementother)
    * [ CanvasElement(CanvasElement&amp;&amp; other) : Element(std::move(other)) ](#canvaselementcanvaselement-other-elementstdmoveother)
    * [ CanvasElement(Ui&amp; ui, const std::string&amp; id) : Element(ui, id) ](#canvaselementui-ui-const-stdstring-id-elementui-id)
    * [ CanvasElement(Ui&amp; ui, const std::string&amp; id, const Element&amp; parent) : Element(ui, id, &quot;canvas&quot;, parent) ](#canvaselementui-ui-const-stdstring-id-const-element-parent-elementui-id-canvas-parent)
    * [ CanvasDataPtr makeCanvas(int width, int height) ](#canvasdataptr-makecanvasint-width-int-height)
    * [ void paint(const CanvasDataPtr&amp; canvas) ](#void-paintconst-canvasdataptr-canvas)
    * [ std::string addImage(const std::string&amp; url, const std::function&lt;void (const std::string&amp; id)&gt;&amp; loaded = nullptr) ](#stdstring-addimageconst-stdstring-url-const-stdfunctionvoid-const-stdstring-id-loaded-nullptr)
    * [ std::vector&lt;std::string&gt; addImages(const std::vector&lt;std::string&gt;&amp; urls, const std::function&lt;void(const std::vector&lt;std::string&gt;)&gt;&amp;loaded = nullptr) ](#stdvectorstdstring-addimagesconst-stdvectorstdstring-urls-const-stdfunctionvoidconst-stdvectorstdstringloaded-nullptr)
    * [ void paintImage(const std::string&amp; imageId, int x, int y, const Element::Rect&amp; clippingRect  = {0, 0, 0, 0}) ](#void-paintimageconst-stdstring-imageid-int-x-int-y-const-elementrect-clippingrect-0-0-0-0)
    * [ void paintImage(const std::string&amp; imageId, const Element::Rect&amp; targetRect, const Element::Rect&amp; clippingRect = {0, 0, 0, 0}) ](#void-paintimageconst-stdstring-imageid-const-elementrect-targetrect-const-elementrect-clippingrect-0-0-0-0)
    * [ void draw(const CommandList&amp; canvasCommands) ](#void-drawconst-commandlist-canvascommands)
Unbalanced scope (0 != 2), C:/Users/intopalo/Documents/Telex-framework/telexlib/include/telex_graphics.h at 214 (ref:(-1)<br/>

---
<a id="Telex"></a>
### Telex 

Common namespace for Telex implementation.

---
<a id="The Canvas class"></a>
#### Telex::The Canvas class 

Bitmap canvas. You normally use this calls using the Graphics (below) compositor.
<a id="void-putint-x-int-y-datat-pixel"></a>
##### void put(int x, int y, dataT pixel) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* p 

Set a pixel value.
<a id="datat-getint-x-int-y-const"></a>
##### dataT get(int x, int y) const 
###### *Param:* x 
###### *Param:* y 
###### *Return:* dataT 

Get a pixel value.
###### width of canvas 
###### height height of canvas 

---

---
<a id="CanvasElement"></a>
#### Telex::CanvasElement 

The CanvasElement class
<a id="canvaselementconst-canvaselement-other-elementother"></a>
##### CanvasElement(const CanvasElement&amp; other) : Element(other) 
###### *Param:* other 

Copy
<a id="canvaselementcanvaselement-other-elementstdmoveother"></a>
##### CanvasElement(CanvasElement&amp;&amp; other) : Element(std::move(other)) 
###### *Param:* other 

Move
<a id="canvaselementui-ui-const-stdstring-id-elementui-id"></a>
##### CanvasElement(Ui&amp; ui, const std::string&amp; id) : Element(ui, id) 
###### *Param:* ui 
###### *Param:* id 

Access an existing HTML Canvas element.
<a id="canvaselementui-ui-const-stdstring-id-const-element-parent-elementui-id-canvas-parent"></a>
##### CanvasElement(Ui&amp; ui, const std::string&amp; id, const Element&amp; parent) : Element(ui, id, &quot;canvas&quot;, parent) 
###### *Param:* ui 
###### *Param:* id 
###### *Param:* parent 

Createa a new HTML Canvas element.
<a id="canvasdataptr-makecanvasint-width-int-height"></a>
##### CanvasDataPtr makeCanvas(int width, int height) 
###### *Param:* width 
###### *Param:* height 
###### *Return:* CanvasPtr 

Pointer to bitmap. You normally call dont this class directly, but create a Graphics compositor (below).
<a id="void-paintconst-canvasdataptr-canvas"></a>
##### void paint(const CanvasDataPtr&amp; canvas) 
###### *Param:* canvas 

Draw a given bitmap.
<a id="stdstring-addimageconst-stdstring-url-const-stdfunctionvoid-const-stdstring-id-loaded-nullptr"></a>
##### std::string addImage(const std::string&amp; url, const std::function&lt;void (const std::string&amp; id)&gt;&amp; loaded = nullptr) 
###### *Param:* url 
###### *Param:* loaded 
###### *Return:* string 

Add a image into Ui.
<a id="stdvectorstdstring-addimagesconst-stdvectorstdstring-urls-const-stdfunctionvoidconst-stdvectorstdstringloaded-nullptr"></a>
##### std::vector&lt;std::string&gt; addImages(const std::vector&lt;std::string&gt;&amp; urls, const std::function&lt;void(const std::vector&lt;std::string&gt;)&gt;&amp;loaded = nullptr) 
###### *Param:* urls 
###### *Param:* loaded 
###### *Return:*  
<a id="void-paintimageconst-stdstring-imageid-int-x-int-y-const-elementrect-clippingrect-0-0-0-0"></a>
##### void paintImage(const std::string&amp; imageId, int x, int y, const Element::Rect&amp; clippingRect  = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* x 
###### *Param:* y 
###### *Param:* clippingRect 

Paint an image.
<a id="void-paintimageconst-stdstring-imageid-const-elementrect-targetrect-const-elementrect-clippingrect-0-0-0-0"></a>
##### void paintImage(const std::string&amp; imageId, const Element::Rect&amp; targetRect, const Element::Rect&amp; clippingRect = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* targetRect 
###### *Param:* clippingRect 

Paint an image.
<a id="void-drawconst-commandlist-canvascommands"></a>
##### void draw(const CommandList&amp; canvasCommands) 
###### *Param:* canvasCommands 
##### function 
###### *Param:* frameComposer 
###### Generated by MarkdownMaker, (c) Markus Mertama 2020 
