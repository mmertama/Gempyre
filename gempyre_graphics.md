![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

gempyre_graphics.h
=====
Gempyre GUI Framework
-------------

gempyre_graphics.h provides a low level graphics capabilites for Gempyre. As Gempyre Element
provides access to HTML elements, their values and attributes - the bitmap graphics is
applied with inherited CanvasElement class. The bitmap can be a raw byte Canvas that can
be modified using RGBA pixels or image files. The image files can be added dynamically or
upon Ui construction.

See mandelbrot application for a bitmap example.
See imageblit for image drawing example.

* [Gempyre ](#gempyre)
* [The Canvas class ](#the-canvas-class)
* [void put(int x, int y, dataT pixel) ](#void-put-int-x-int-y-datat-pixel-)
* [ get(int x, int y) const ](#-get-int-x-int-y-const)
* [CanvasElement ](#canvaselement)
* [lement(const CanvasElement& other) ](#lement-const-canvaselement-other-)
* [lement(CanvasElement&& other) ](#lement-canvaselement-other-)
* [lement(Ui& ui, const std::string& id) ](#lement-ui-ui-const-std-string-id-)
* [lement(Ui& ui, const std::string& id, const Element& parent) ](#lement-ui-ui-const-std-string-id-const-element-parent-)
* [tr makeCanvas(int width, int height) ](#tr-makecanvas-int-width-int-height-)
* [std::string addImage(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr) ](#std-string-addimage-const-std-string-url-const-std-functionvoid-const-std-string-id-loaded-nullptr-)
* [std::vector<std::string> addImages(const std::vector<std::string>& urls, const std::function<void(const std::vector<std::string>)>&loaded = nullptr) ](#std-vectorstd-string-addimages-const-std-vectorstd-string-urls-const-std-functionvoid-const-std-vectorstd-string-loaded-nullptr-)
* [void paintImage(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) ](#void-paintimage-const-std-string-imageid-int-x-int-y-const-element-rect-clippingrect-0-0-0-0-)
* [void paintImage(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) ](#void-paintimage-const-std-string-imageid-const-element-rect-targetrect-const-element-rect-clippingrect-0-0-0-0-)
* [void draw(const CommandList& canvasCommands) ](#void-draw-const-commandlist-canvascommands-)
* [void draw(const FrameComposer& frameComposer) ](#void-draw-const-framecomposer-framecomposer-)
* [void erase(bool resized = false) ](#void-erase-bool-resized-false-)
* [static constexpr inline type rgbaClamped(type r, type g, type b, type a = 0xFF) ](#static-constexpr-inline-type-rgbaclamped-type-r-type-g-type-b-type-a-0xff-)
* [static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) ](#static-constexpr-inline-type-rgba-type-r-type-g-type-b-type-a-0xff-)
* [static constexpr inline type r(type pixel) ](#static-constexpr-inline-type-r-type-pixel-)
* [static constexpr inline type g(type pixel) ](#static-constexpr-inline-type-g-type-pixel-)
* [static constexpr inline type b(type pixel) ](#static-constexpr-inline-type-b-type-pixel-)
* [static constexpr inline type alpha(type pixel) ](#static-constexpr-inline-type-alpha-type-pixel-)
* [Graphics ](#graphics)
* [Graphics(const Gempyre::CanvasElement& element, int width, int height) ](#graphics-const-gempyre-canvaselement-element-int-width-int-height-)
* [Graphics(const Gempyre::CanvasElement& element) ](#graphics-const-gempyre-canvaselement-element-)
* [void create(int width, int height) ](#void-create-int-width-int-height-)
* [Graphics clone() const ](#graphics-clone-const)
* [static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a) ](#static-constexpr-color-type-pix-color-type-r-color-type-g-color-type-b-color-type-a-0xff-return-color-rgba-r-g-b-a-)
* [void setPixel(int x, int y, Color::type color) ](#void-setpixel-int-x-int-y-color-type-color-)
* [void setAlpha(int x, int y, Color::type alpha) ](#void-setalpha-int-x-int-y-color-type-alpha-)
* [int width() const ](#int-width-const)
* [int height() const ](#int-height-const)
* [void drawRect(const Element::Rect& rect, Color::type color) ](#void-drawrect-const-element-rect-rect-color-type-color-)
* [void merge(const Graphics& other) ](#void-merge-const-graphics-other-)
* [void swap(Graphics& other) ](#void-swap-graphics-other-)
* [void update() ](#void-update-)
* [tr ptr() ](#tr-ptr-)
* [The FrameComposer class ](#the-framecomposer-class)
* [FrameComposer() ](#framecomposer-)
* [FrameComposer(FrameComposer&& other) ](#framecomposer-framecomposer-other-)
* [FrameComposer strokeRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"strokeRect", r.x, r.y, r.width, r.height}) ](#framecomposer-strokerect-const-gempyre-element-rect-r-m_composition-insert-m_composition-end-strokerect-r-x-r-y-r-width-r-height-)
* [FrameComposer clearRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"clearRect", r.x, r.y, r.width, r.height}) ](#framecomposer-clearrect-const-gempyre-element-rect-r-m_composition-insert-m_composition-end-clearrect-r-x-r-y-r-width-r-height-)
* [FrameComposer fillRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"fillRect", r.x, r.y, r.width, r.height}) ](#framecomposer-fillrect-const-gempyre-element-rect-r-m_composition-insert-m_composition-end-fillrect-r-x-r-y-r-width-r-height-)
* [FrameComposer fillText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"fillText", text, x, y}) ](#framecomposer-filltext-const-std-string-text-double-x-double-y-m_composition-insert-m_composition-end-filltext-text-x-y-)
* [FrameComposer strokeText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"strokeText", text, x, y}) ](#framecomposer-stroketext-const-std-string-text-double-x-double-y-m_composition-insert-m_composition-end-stroketext-text-x-y-)
* [FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) ](#framecomposer-arc-double-x-double-y-double-r-double-sangle-double-eangle-)
* [FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) ](#framecomposer-ellipse-double-x-double-y-double-radiusx-double-radiusy-double-rotation-double-startangle-double-endangle-)
* [FrameComposer beginPath()  {m_composition.insert(m_composition.end(), {"beginPath"}) ](#framecomposer-beginpath-m_composition-insert-m_composition-end-beginpath-)
* [FrameComposer closePath() {m_composition.insert(m_composition.end(), {"closePath"}) ](#framecomposer-closepath-m_composition-insert-m_composition-end-closepath-)
* [FrameComposer lineTo(double x, double y) {m_composition.insert(m_composition.end(), {"lineTo", x, y}) ](#framecomposer-lineto-double-x-double-y-m_composition-insert-m_composition-end-lineto-x-y-)
* [FrameComposer moveTo(double x, double y)  {m_composition.insert(m_composition.end(), {"moveTo", x, y}) ](#framecomposer-moveto-double-x-double-y-m_composition-insert-m_composition-end-moveto-x-y-)
* [FrameComposer bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) ](#framecomposer-beziercurveto-double-cp1x-double-cp1y-double-cp2x-double-cp2y-double-x-double-y-)
* [FrameComposer quadraticCurveTo(double cpx, double cpy, double x, double y) ](#framecomposer-quadraticcurveto-double-cpx-double-cpy-double-x-double-y-)
* [FrameComposer arcTo(double x1, double y1, double x2, double y2, double radius) ](#framecomposer-arcto-double-x1-double-y1-double-x2-double-y2-double-radius-)
* [FrameComposer rect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"rect", r.x, r.y, r.width, r.height}) ](#framecomposer-rect-const-gempyre-element-rect-r-m_composition-insert-m_composition-end-rect-r-x-r-y-r-width-r-height-)
* [FrameComposer stroke() {m_composition.insert(m_composition.end(), {"stroke"}) ](#framecomposer-stroke-m_composition-insert-m_composition-end-stroke-)
* [FrameComposer fill() {m_composition.insert(m_composition.end(), {"fill"}) ](#framecomposer-fill-m_composition-insert-m_composition-end-fill-)
* [FrameComposer fillStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"fillStyle", color}) ](#framecomposer-fillstyle-const-std-string-color-m_composition-insert-m_composition-end-fillstyle-color-)
* [FrameComposer strokeStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"strokeStyle", color}) ](#framecomposer-strokestyle-const-std-string-color-m_composition-insert-m_composition-end-strokestyle-color-)
* [FrameComposer lineWidth(double width) {m_composition.insert(m_composition.end(), {"lineWidth", width}) ](#framecomposer-linewidth-double-width-m_composition-insert-m_composition-end-linewidth-width-)
* [FrameComposer font(const std::string& style) {m_composition.insert(m_composition.end(), {"font", style}) ](#framecomposer-font-const-std-string-style-m_composition-insert-m_composition-end-font-style-)
* [FrameComposer textAlign(const std::string& align) {m_composition.insert(m_composition.end(), {"textAlign", align}) ](#framecomposer-textalign-const-std-string-align-m_composition-insert-m_composition-end-textalign-align-)
* [FrameComposer save() {m_composition.insert(m_composition.end(), {"save"}) ](#framecomposer-save-m_composition-insert-m_composition-end-save-)
* [FrameComposer restore() {m_composition.insert(m_composition.end(), {"restore"}) ](#framecomposer-restore-m_composition-insert-m_composition-end-restore-)
* [FrameComposer rotate(double angle)  {m_composition.insert(m_composition.end(), {"rotate", angle}) ](#framecomposer-rotate-double-angle-m_composition-insert-m_composition-end-rotate-angle-)
* [FrameComposer translate(double x, double y)  {m_composition.insert(m_composition.end(), {"translate", x, y}) ](#framecomposer-translate-double-x-double-y-m_composition-insert-m_composition-end-translate-x-y-)
* [FrameComposer scale(const double x, double y)  {m_composition.insert(m_composition.end(), {"scale", x, y}) ](#framecomposer-scale-const-double-x-double-y-m_composition-insert-m_composition-end-scale-x-y-)
* [FrameComposer drawImage(const std::string& id, double x, double y)  {m_composition.insert(m_composition.end(), {"drawImage", id, x, y}) ](#framecomposer-drawimage-const-std-string-id-double-x-double-y-m_composition-insert-m_composition-end-drawimage-id-x-y-)
* [FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& rect)  {m_composition.insert(m_composition.end(), {"drawImageRect", id, rect.x, rect.y, rect.width, rect.height}) ](#framecomposer-drawimage-const-std-string-id-const-gempyre-element-rect-rect-m_composition-insert-m_composition-end-drawimagerect-id-rect-x-rect-y-rect-width-rect-height-)
* [FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {m_composition.insert(m_composition.end(), {"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height}) ](#framecomposer-drawimage-const-std-string-id-const-gempyre-element-rect-clip-const-gempyre-element-rect-rect-m_composition-insert-m_composition-end-drawimageclip-id-clip-x-clip-y-clip-width-clip-height-rect-x-rect-y-rect-width-rect-height-)
* [const Gempyre::CanvasElement::CommandList& composed() const ](#const-gempyre-canvaselement-commandlist-composed-const)

---
### Gempyre 

Common namespace for Gempyre implementation.
##### static constexpr inline type rgbaClamped(type r, type g, type b, type a = 0xFF) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* pixel value 

Creates a pixel from rgb or rgba.
##### static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* dataT 

Creates a pixel from rgb or rgba.
##### static constexpr inline type r(type pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only red component.
##### static constexpr inline type g(type pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only green component.
##### static constexpr inline type b(type pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only blue component.
##### static constexpr inline type alpha(type pixel) 
###### *Param:* pixel 
###### *Return:* dataT 

Return pixel with only alpha component.

---
#### The Canvas class 

Bitmap canvas. You normally use this calls using the Graphics (below) compositor.
##### void put(int x, int y, dataT pixel) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* p 

Set a pixel value.
#####  get(int x, int y) const 
###### *Param:* x 
###### *Param:* y 
###### *Return:* dataT 

Get a pixel value.
###### width of canvas 
###### height height of canvas 

---

---
#### CanvasElement 

The CanvasElement class
##### lement(const CanvasElement& other) 
###### *Param:* other 

Copy
##### lement(CanvasElement&& other) 
###### *Param:* other 

Move
##### lement(Ui& ui, const std::string& id) 
###### *Param:* ui 
###### *Param:* id 

Access an existing HTML Canvas element.
##### lement(Ui& ui, const std::string& id, const Element& parent) 
###### *Param:* ui 
###### *Param:* id 
###### *Param:* parent 

Createa a new HTML Canvas element.
##### tr makeCanvas(int width, int height) 
###### *Param:* width 
###### *Param:* height 
###### *Return:* CanvasPtr 

Pointer to bitmap. You normally call dont this class directly, but create a Graphics compositor (below).
##### std::string addImage(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr) 
###### *Param:* url 
###### *Param:* loaded 
###### *Return:* string 

Add a image into Ui.
##### std::vector<std::string> addImages(const std::vector<std::string>& urls, const std::function<void(const std::vector<std::string>)>&loaded = nullptr) 
###### *Param:* urls 
###### *Param:* loaded 
###### *Return:*  
##### void paintImage(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* x 
###### *Param:* y 
###### *Param:* clippingRect 

Paint an image.
##### void paintImage(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) 
###### *Param:* imageId 
###### *Param:* targetRect 
###### *Param:* clippingRect 

Paint an image.
##### void draw(const CommandList& canvasCommands) 
###### *Param:* canvasCommands 
##### void draw(const FrameComposer& frameComposer) 
###### *Param:* frameComposer 
##### void erase(bool resized = false) 
###### *Param:* resized 

---

---
#### Graphics 
The Graphics class, a simple compositor to clone and merge Canvases. Besides a simple bitmap manipulation the a rect drawing
primitive is provided.
##### Graphics(const Gempyre::CanvasElement& element, int width, int height) 
###### *Param:* element 
###### *Param:* width 
###### *Param:* height 

Construct a Graphics and create a Canvas.
##### Graphics(const Gempyre::CanvasElement& element) 
###### *Param:* element 

Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
##### void create(int width, int height) 
###### *Param:* width 
###### *Param:* height 

Create a canvas.
##### Graphics clone() const 
###### *Return:* Graphics 

Clone this Graphics.
##### static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a) 
###### *Param:* r 
###### *Param:* g 
###### *Param:* b 
###### *Param:* a 
###### *Return:* Color 

Return a pixel.
##### void setPixel(int x, int y, Color::type color) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* color 

Set a pixel.
##### void setAlpha(int x, int y, Color::type alpha) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* alpha 

Set alpha.
##### int width() const 
###### *Return:* int 

##### int height() const 
###### *Return:* int 
##### void drawRect(const Element::Rect& rect, Color::type color) 
###### *Param:* rect 
###### *Param:* color 

Draw a rect.
##### void merge(const Graphics& other) 
###### *Param:* other 

Merge another Graphics to this.
##### void swap(Graphics& other) 
###### *Param:* other 

Swap Graphics data with another.
##### void update() 

Draw Graphics.
##### tr ptr() 
###### *Return:*  

---

---
#### The FrameComposer class 
typesafe CommandList composer
##### FrameComposer() 
##### FrameComposer(FrameComposer&& other) 
###### *Param:* lst 
##### FrameComposer strokeRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"strokeRect", r.x, r.y, r.width, r.height}) 
###### *Param:* r 
###### *Return:*  
##### FrameComposer clearRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"clearRect", r.x, r.y, r.width, r.height}) 
###### *Param:* r 
###### *Return:*  
##### FrameComposer fillRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"fillRect", r.x, r.y, r.width, r.height}) 
###### *Param:* r 
###### *Return:*  
##### FrameComposer fillText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"fillText", text, x, y}) 
###### *Param:* text 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer strokeText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"strokeText", text, x, y}) 
###### *Param:* text 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* r 
###### *Param:* sAngle 
###### *Param:* eAngle 
###### *Return:*  
##### FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) 
###### *Param:* x 
###### *Param:* y 
###### *Param:* radiusX 
###### *Param:* radiusY 
###### *Param:* rotation 
###### *Param:* startAngle 
###### *Param:* endAngle 
###### *Return:*  
##### FrameComposer beginPath()  {m_composition.insert(m_composition.end(), {"beginPath"}) 
###### *Return:*  
##### FrameComposer closePath() {m_composition.insert(m_composition.end(), {"closePath"}) 
###### *Return:*  
##### FrameComposer lineTo(double x, double y) {m_composition.insert(m_composition.end(), {"lineTo", x, y}) 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer moveTo(double x, double y)  {m_composition.insert(m_composition.end(), {"moveTo", x, y}) 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) 
###### *Param:* cp1x 
###### *Param:* cp1y 
###### *Param:* cp2x 
###### *Param:* cp2y 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer quadraticCurveTo(double cpx, double cpy, double x, double y) 
###### *Param:* cpx 
###### *Param:* cpy 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer arcTo(double x1, double y1, double x2, double y2, double radius) 
###### *Param:* x1 
###### *Param:* y1 
###### *Param:* x2 
###### *Param:* y2 
###### *Param:* radius 
###### *Return:*  
##### FrameComposer rect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"rect", r.x, r.y, r.width, r.height}) 
###### *Param:* r 
###### *Return:*  
##### FrameComposer stroke() {m_composition.insert(m_composition.end(), {"stroke"}) 
###### *Return:*  
##### FrameComposer fill() {m_composition.insert(m_composition.end(), {"fill"}) 
###### *Return:*  
##### FrameComposer fillStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"fillStyle", color}) 
###### *Param:* color 
###### *Return:*  
##### FrameComposer strokeStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"strokeStyle", color}) 
###### *Param:* color 
###### *Return:*  
##### FrameComposer lineWidth(double width) {m_composition.insert(m_composition.end(), {"lineWidth", width}) 
###### *Param:* width 
###### *Return:*  
##### FrameComposer font(const std::string& style) {m_composition.insert(m_composition.end(), {"font", style}) 
###### *Param:* style 
###### *Return:*  
##### FrameComposer textAlign(const std::string& align) {m_composition.insert(m_composition.end(), {"textAlign", align}) 
###### *Param:* align 
###### *Return:*  
##### FrameComposer save() {m_composition.insert(m_composition.end(), {"save"}) 
###### *Return:*  
##### FrameComposer restore() {m_composition.insert(m_composition.end(), {"restore"}) 
###### *Return:*  
##### FrameComposer rotate(double angle)  {m_composition.insert(m_composition.end(), {"rotate", angle}) 
###### *Param:* angle 
###### *Return:*  
##### FrameComposer translate(double x, double y)  {m_composition.insert(m_composition.end(), {"translate", x, y}) 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer scale(const double x, double y)  {m_composition.insert(m_composition.end(), {"scale", x, y}) 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer drawImage(const std::string& id, double x, double y)  {m_composition.insert(m_composition.end(), {"drawImage", id, x, y}) 
###### *Param:* id 
###### *Param:* x 
###### *Param:* y 
###### *Return:*  
##### FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& rect)  {m_composition.insert(m_composition.end(), {"drawImageRect", id, rect.x, rect.y, rect.width, rect.height}) 
###### *Param:* id 
###### *Param:* rect 
###### *Return:*  
##### FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {m_composition.insert(m_composition.end(), {"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height}) 
###### *Param:* id 
###### *Param:* clip 
###### *Param:* rect 
###### *Return:*  
##### const Gempyre::CanvasElement::CommandList& composed() const 
###### *Return:*  

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2018 
