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
    * [  get(int x, int y) const ](#getint-x-int-y-const)
    * [ static constexpr inline dataT rgbaClamped(dataT r, dataT g, dataT b, dataT a = 0xFF) ](#static-constexpr-inline-datat-rgbaclampeddatat-r-datat-g-datat-b-datat-a-0xff)
    * [ static constexpr inline dataT rgba(dataT r, dataT g, dataT b, dataT a = 0xFF) ](#static-constexpr-inline-datat-rgbadatat-r-datat-g-datat-b-datat-a-0xff)
    * [ static constexpr inline dataT r(dataT pixel) ](#static-constexpr-inline-datat-rdatat-pixel)
    * [ static constexpr inline dataT g(dataT pixel) ](#static-constexpr-inline-datat-gdatat-pixel)
    * [ static constexpr inline dataT b(dataT pixel) ](#static-constexpr-inline-datat-bdatat-pixel)
    * [ static constexpr inline dataT alpha(dataT pixel) ](#static-constexpr-inline-datat-alphadatat-pixel)
  * [ class CanvasElement ](#canvaselement)
    * [ lement(const CanvasElement&amp; other) : Element(other) ](#lementconst-canvaselement-other-elementother)
    * [ lement(CanvasElement&amp;&amp; other) : Element(std::move(other)) {m_tile = std::move(other.m_tile) ](#lementcanvaselement-other-elementstdmoveother-m_tile-stdmoveotherm_tile)
    * [ lement(Ui&amp; ui, const std::string&amp; id) : Element(ui, id) ](#lementui-ui-const-stdstring-id-elementui-id)
    * [ std::string addImage(const std::string&amp; url, const std::function&lt;void (const std::string&amp; id)&gt;&amp; loaded = nullptr) ](#stdstring-addimageconst-stdstring-url-const-stdfunctionvoid-const-stdstring-id-loaded-nullptr)
    * [ void paintImage(const std::string imageId, int x, int y, const Element::Rect&amp; clippingRect  = {0, 0, 0, 0}) ](#void-paintimageconst-stdstring-imageid-int-x-int-y-const-elementrect-clippingrect-0-0-0-0)
    * [ void paintImage(const std::string imageId, const Element::Rect&amp; targetRect, const Element::Rect&amp; clippingRect = {0, 0, 0, 0}) ](#void-paintimageconst-stdstring-imageid-const-elementrect-targetrect-const-elementrect-clippingrect-0-0-0-0)
  * [ class Graphics ](#graphics)
    * [ Graphics(const Telex::CanvasElement&amp; element, int width, int height) : m_element(element), m_canvas(m_element.makeCanvas(width, height)) ](#graphicsconst-telexcanvaselement-element-int-width-int-height-m_elementelement-m_canvasm_elementmakecanvaswidth-height)
    * [ Graphics(const Telex::CanvasElement&amp; element) : m_element(element) ](#graphicsconst-telexcanvaselement-element-m_elementelement)
    * [ void create(int width, int height) ](#void-createint-width-int-height)
    * [ Graphics clone() const ](#graphics-clone-const)
    * [ static constexpr Color pix(Color r, Color g, Color b, Color a = 0xFF) {return Telex::Canvas::rgba(r, g, b, a) ](#static-constexpr-color-pixcolor-r-color-g-color-b-color-a-0xff-return-telexcanvasrgbar-g-b-a)
    * [ void setPixel(int x, int y, Color color) {m_canvas-&gt;put(x, y, color) ](#void-setpixelint-x-int-y-color-color-m_canvasputx-y-color)
    * [ void setAlpha(int x, int y, Color alpha) ](#void-setalphaint-x-int-y-color-alpha)
    * [ int width() const ](#int-width-const)
    * [ int height() const ](#int-height-const)
    * [ void drawRect(const Element::Rect&amp; rect, Color color) ](#void-drawrectconst-elementrect-rect-color-color)
    * [ void merge(const Graphics&amp; other) ](#void-mergeconst-graphics-other)
    * [ void swap(Graphics&amp; other) ](#void-swapgraphics-other)
    * [ void update() ](#void-update)

---
<a id="Telex"></a>
### Telex 

Common namespace for Telex implementation.

---

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
<a id="getint-x-int-y-const"></a>
#####  get(int x, int y) const 
###### *Param:* x 
###### *Param:* y 
###### *Return:* dataT 

Get a pixel value.
<a id="static-constexpr-inline-datat-rgbaclampeddatat-r-datat-g-datat-b-datat-a-0xff"></a>
##### static constexpr inline dataT rgbaClamped(dataT r, dataT g, dataT b, dataT a = 0xFF) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* pixel value 

Creates a pixel from rgb or rgba.
<a id="static-constexpr-inline-datat-rgbadatat-r-datat-g-datat-b-datat-a-0xff"></a>
##### static constexpr inline dataT rgba(dataT r, dataT g, dataT b, dataT a = 0xFF) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* dataT 

Creates a pixel from rgb or rgba.
<a id="static-constexpr-inline-datat-rdatat-pixel"></a>
##### static constexpr inline dataT r(dataT pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only red component.
<a id="static-constexpr-inline-datat-gdatat-pixel"></a>
##### static constexpr inline dataT g(dataT pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only green component.
<a id="static-constexpr-inline-datat-bdatat-pixel"></a>
##### static constexpr inline dataT b(dataT pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only blue component.
<a id="static-constexpr-inline-datat-alphadatat-pixel"></a>
##### static constexpr inline dataT alpha(dataT pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only alpha component.
###### width of canvas 
###### height height of canvas 

---

---
<a id="CanvasElement"></a>
#### Telex::CanvasElement 

The CanvasElement class
<a id="lementconst-canvaselement-other-elementother"></a>
##### lement(const CanvasElement&amp; other) : Element(other) 
###### *Param:* other 

Copy
<a id="lementcanvaselement-other-elementstdmoveother-m_tile-stdmoveotherm_tile"></a>
##### lement(CanvasElement&amp;&amp; other) : Element(std::move(other)) {m_tile = std::move(other.m_tile) 
###### *Param:* other 

Move
<a id="lementui-ui-const-stdstring-id-elementui-id"></a>
##### lement(Ui&amp; ui, const std::string&amp; id) : Element(ui, id) 
###### *Param:* ui 
###### *Param:* id 

Access an existing HTML Canvas element.
###### CanvasElement 
###### *Param:* ui 
###### *Param:* id 
###### *Param:* parent 

Createa a new HTML Canvas element.
###### makeCanvas 
###### *Param:* width 
###### *Param:* height 
###### *Return:* CanvasPtr 

Pointer to bitmap. You normally call this class directly, but create a Graphics compositor (below).
###### paint 
###### *Param:* canvas 

Draw a given bitmap.
<a id="stdstring-addimageconst-stdstring-url-const-stdfunctionvoid-const-stdstring-id-loaded-nullptr"></a>
##### std::string addImage(const std::string&amp; url, const std::function&lt;void (const std::string&amp; id)&gt;&amp; loaded = nullptr) 
###### *Param:* url 
###### *Param:* loaded 
###### *Return:* string 

Add a image into Ui.
<a id="void-paintimageconst-stdstring-imageid-int-x-int-y-const-elementrect-clippingrect-0-0-0-0"></a>
##### void paintImage(const std::string imageId, int x, int y, const Element::Rect&amp; clippingRect  = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* x 
###### *Param:* y 
###### *Param:* clippingRect 

Paint an image.
<a id="void-paintimageconst-stdstring-imageid-const-elementrect-targetrect-const-elementrect-clippingrect-0-0-0-0"></a>
##### void paintImage(const std::string imageId, const Element::Rect&amp; targetRect, const Element::Rect&amp; clippingRect = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* targetRect 
###### *Param:* clippingRect 

Paint an image.

---

---
<a id="Graphics"></a>
#### Telex::Graphics 
The Graphics class, a simple compositor to clone and merge Canvases. Besides a simple bitmap manipulation the a rect drawing
primitive is provided.
<a id="graphicsconst-telexcanvaselement-element-int-width-int-height-m_elementelement-m_canvasm_elementmakecanvaswidth-height"></a>
##### Graphics(const Telex::CanvasElement&amp; element, int width, int height) : m_element(element), m_canvas(m_element.makeCanvas(width, height)) 
###### *Param:* element 
###### *Param:* width 
###### *Param:* height 

Construct a Graphics and create a Canvas.
<a id="graphicsconst-telexcanvaselement-element-m_elementelement"></a>
##### Graphics(const Telex::CanvasElement&amp; element) : m_element(element) 
###### *Param:* element 

Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
<a id="void-createint-width-int-height"></a>
##### void create(int width, int height) 
###### *Param:* width 
###### *Param:* height 

Create a canvas.
<a id="graphics-clone-const"></a>
##### Graphics clone() const 
###### *Return:* Graphics 

Clone this Graphics.
<a id="static-constexpr-color-pixcolor-r-color-g-color-b-color-a-0xff-return-telexcanvasrgbar-g-b-a"></a>
##### static constexpr Color pix(Color r, Color g, Color b, Color a = 0xFF) {return Telex::Canvas::rgba(r, g, b, a) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* Color 

Return a pixel.
<a id="void-setpixelint-x-int-y-color-color-m_canvasputx-y-color"></a>
##### void setPixel(int x, int y, Color color) {m_canvas-&gt;put(x, y, color) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* color 

Set a pixel.
<a id="void-setalphaint-x-int-y-color-alpha"></a>
##### void setAlpha(int x, int y, Color alpha) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* alpha 

Set alpha.
<a id="int-width-const"></a>
##### int width() const 
###### *Return:* int 

<a id="int-height-const"></a>
##### int height() const 
###### *Return:* int 
<a id="void-drawrectconst-elementrect-rect-color-color"></a>
##### void drawRect(const Element::Rect&amp; rect, Color color) 
###### *Param:* rect 
###### *Param:* color 

Draw a rect.
<a id="void-mergeconst-graphics-other"></a>
##### void merge(const Graphics&amp; other) 
###### *Param:* other 

Merge another Graphics to this.
<a id="void-swapgraphics-other"></a>
##### void swap(Graphics&amp; other) 
###### *Param:* other 

Swap Graphics data with another.
<a id="void-update"></a>
##### void update() 

Draw Graphics.

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2020 
