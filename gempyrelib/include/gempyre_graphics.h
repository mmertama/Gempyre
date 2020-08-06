#ifndef GEMPYRE_GRAPHICS_H
#define GEMPYRE_GRAPHICS_H

#include <gempyre.h>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * gempyre_graphics.h
  * =====
  * Gempyre GUI Framework
  * -------------
  *
  * gempyre_graphics.h provides a low level graphics capabilites for Gempyre. As Gempyre Element
  * provides access to HTML elements, their values and attributes - the bitmap graphics is
  * applied with inherited CanvasElement class. The bitmap can be a raw byte Canvas that can
  * be modified using RGBA pixels or image files. The image files can be added dynamically or
  * upon Ui construction.
  *
  * See mandelbrot application for a bitmap example.
  * See imageblit for image drawing example.
  *
  * @toc
  */

#ifdef WINDOWS_EXPORT
    #ifndef GEMPYRE_EX
        #define GEMPYRE_EX __declspec( dllexport )
    #else
        #define GEMPYRE_EX
    #endif
#endif

#define gempyre_graphics_assert(b, x) ((b) || GempyreUtils::doFatal(x, nullptr, __FILE__, __LINE__));

/**
 * @namespace Gempyre
 *
 * Common namespace for Gempyre implementation.
 */
namespace  Gempyre {

class FrameComposer;

/**
 * @class The Canvas class
 *
 * Bitmap canvas. You normally use this calls using the Graphics (below) compositor.
 */
class GEMPYRE_EX CanvasData : public Data {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
    ~CanvasData();
    /**
     * @function put
     * @param x
     * @param y
     * @param p
     *
     * Set a pixel value.
     */
    void put(int x, int y, dataT pixel) {
        data()[x + y * width] = pixel;
    }
    /**
     * @function get
     * @param x
     * @param y
     * @return dataT
     *
     * Get a pixel value.
     */
    dataT get(int x, int y) const {
        return data()[x + y * width];
    }
    /**
     * @brief width of canvas
     */
    const int width;
    /**
     * @brief height height of canvas
     */
    const int height;
private:
    CanvasData(int w, int h, const std::string& owner) : Data(static_cast<unsigned>(w * h), CanvasId, owner,
                                                         {0, 0, static_cast<dataT>(w), static_cast<dataT>(h)}),
        width(w),
        height(h) {}
    friend class CanvasElement;
};
/**
 * @scopeend
 */

using CanvasDataPtr = std::shared_ptr<CanvasData>;

/**
 * @class CanvasElement
 *
 * The CanvasElement class
 */
class GEMPYRE_EX CanvasElement : public Element {
    static constexpr auto TileWidth = 64;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes (There is a issue somewhere, this not really work if something else)
    static constexpr auto TileHeight = 63; // as there are some header info
public:
    using Command = std::variant<std::string, double, int>;   
    using CommandList = std::vector<Command>;
    ~CanvasElement();
    /**
     * @function CanvasElement
     * @param other
     *
     * Copy
     */
    CanvasElement(const CanvasElement& other)
        : Element(other) {
        m_tile = other.m_tile;
        m_width = other.m_width;
        m_height = other.m_height;
    }
    /**
     * @function CanvasElement
     * @param other
     *
     * Move
     */
    CanvasElement(CanvasElement&& other)
        : Element(std::move(other)) {
        m_tile = std::move(other.m_tile);
        m_width = other.m_width;
        m_height = other.m_height;
    }
    /**
     * @function CanvasElement
     * @param ui
     * @param id
     *
     * Access an existing HTML Canvas element.
     */
    CanvasElement(Ui& ui, const std::string& id)
        : Element(ui, id) {}
    /**
     * @function CanvasElement
     * @param ui
     * @param id
     * @param parent
     *
     * Createa a new HTML Canvas element.
     */
    CanvasElement(Ui& ui, const std::string& id, const Element& parent)
        : Element(ui, id, "canvas", parent) {}
    CanvasElement& operator=(const CanvasElement& other) = default;
    CanvasElement& operator=(CanvasElement&& other) = default;
    /**
     * @function makeCanvas
     * @param width
     * @param height
     * @return CanvasPtr
     *
     * Pointer to bitmap. You normally call dont this class directly, but create a Graphics compositor (below).
     */
    CanvasDataPtr makeCanvas(int width, int height);
    /**
     * @function addImage
     * @param url
     * @param loaded
     * @return string
     *
     * Add a image into Ui.
     */
    std::string addImage(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr);
    /**
     * @function addImages
     * @param urls
     * @param loaded
     * @return
     */
    std::vector<std::string> addImages(const std::vector<std::string>& urls, const std::function<void(const std::vector<std::string>)>&loaded = nullptr);
    /**
      * @function paintImage
      * @param imageId
      * @param x
      * @param y
      * @param clippingRect
      *
      * Paint an image.
      */
    void paintImage(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0});
    /**
      * @function paintImage
      * @param imageId
      * @param targetRect
      * @param clippingRect
      *
      * Paint an image.
      */
    void paintImage(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0});
    /**
     * @function draw
     * @param canvasCommands
     */
    void draw(const CommandList& canvasCommands);

    /**
     * @function draw
     * @param frameComposer
     */
    void draw(const FrameComposer& frameComposer);
    /**
     * @function erase
     * @param resized
     */
    void erase(bool resized = false);

    bool hasCanvas() const {
        return !!m_tile;
    }
private:
    friend class Graphics;
    void paint(const CanvasDataPtr& canvas);
private:
    CanvasDataPtr m_tile;
    int m_width = 0;
    int m_height = 0;
};
/**
 * @scopeend
 */

namespace  Color {
using type = Gempyre::Data::dataT;
/**
 * @function rgbaClamped
 * @param r
 * @param g
 * @param b
 * @param a
 * @return pixel value
 *
 * Creates a pixel from rgb or rgba.
 */
static constexpr inline type rgbaClamped(type r, type g, type b, type a = 0xFF) {
    return (0xFF & r) | ((0xFF & g) << 8) | ((0xFF & b) << 16) | ((0xFF & a) << 24);
}
/**
 * @function rgba
 * @param r
 * @param g
 * @param b
 * @param a
 * @return dataT
 *
 * Creates a pixel from rgb or rgba.
 */
static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) {
    return r | (g << 8) | (b << 16) | (a << 24);
}
/**
 * @function r
 * @param pixel
 * @return dataT
 *
 * Return pixel with only red component.
 */
static constexpr inline type r(type pixel) {
    return pixel & static_cast<type>(0xFF);
}
/**
 * @function g
 * @param pixel
 * @return dataT
 *
 * Return pixel with only green component.
 */
static constexpr inline type g(type pixel) {
    return (pixel & static_cast<type>(0xFF00)) >> 8;
}
/**
 * @function b
 * @param pixel
 * @return dataT
 *
 * Return pixel with only blue component.
 */
static constexpr inline type b(type pixel) {
    return (pixel & static_cast<type>(0xFF0000)) >> 16;
}
/**
 * @function alpha
 * @param pixel
 * @return dataT
 *
 * Return pixel with only alpha component.
 */
static constexpr inline type alpha(type pixel) {
    return (pixel & static_cast<type>(0xFF000000)) >> 24;
}
}

/**
 * @class Graphics
 * The Graphics class, a simple compositor to clone and merge Canvases. Besides a simple bitmap manipulation the a rect drawing
 * primitive is provided.
 */
class GEMPYRE_EX Graphics {
public:
    /**
     * @function Graphics
     * @param element
     * @param width
     * @param height
     *
     * Construct a Graphics and create a Canvas.
     */
    Graphics(const Gempyre::CanvasElement& element, int width, int height);
    /**
     * @function Graphics
     * @param element
     *
     * Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
     */
    Graphics(const Gempyre::CanvasElement& element);
    Graphics(Graphics&& other) = default;
    Graphics(const Graphics& other) = default;
    Graphics& operator=(const Graphics& other) = default;
    Graphics& operator=(Graphics&& other) = default;
    /**
     * @function create
     * @param width
     * @param height
     *
     * Create a canvas.
     */
    void create(int width, int height) {
        m_canvas = m_element.makeCanvas(width, height);
    }
    /**
     * @function clone
     * @return Graphics
     *
     * Clone this Graphics.
     */
    Graphics clone() const;

    /**
     * @function pix
     * @param r
     * @param g
     * @param b
     * @param a
     * @return Color
     *
     * Return a pixel.
     */
    static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a);}
    static constexpr Color::type Black = Color::rgba(0, 0, 0, 0xFF);
    static constexpr Color::type White = Color::rgba(0xFF, 0xFF, 0xFF, 0xFF);
    static constexpr Color::type Red = Color::rgba(0xFF, 0, 0, 0xFF);
    static constexpr Color::type Green = Color::rgba(0, 0xFF, 0, 0xFF);
    static constexpr Color::type Blue = Color::rgba(0, 0xFF, 0, 0xFF);
    /**
     * @function setPixel
     * @param x
     * @param y
     * @param color
     *
     * Set a pixel.
     */
    void setPixel(int x, int y, Color::type color) {
        m_canvas->put(x, y, color);
    }
    /**
     * @function setAlpha
     * @param x
     * @param y
     * @param alpha
     *
     * Set alpha.
     */
    void setAlpha(int x, int y, Color::type alpha) {
        const auto c = m_canvas->get(x, y);
        m_canvas->put(x, y, pix(Color::r(c), Color::g(c), Color::b(c), alpha));
    }
    /**
     * @function width
     * @return int
     *
     */
    int width() const {
        return m_canvas->width;
    }
    /**
     * @function height
     * @return int
     */
    int height() const {
        return m_canvas->height;
    }
    /**
     * @function drawRect
     * @param rect
     * @param color
     *
     * Draw a rect.
     */
    void drawRect(const Element::Rect& rect, Color::type color);
    /**
     * @function merge
     * @param other
     *
     * Merge another Graphics to this.
     */
    void merge(const Graphics& other);
    /**
     * @function swap
     * @param other
     *
     * Swap Graphics data with another.
     */
    void swap(Graphics& other) {
        m_canvas.swap(other.m_canvas);
    }
    /**
     * @function update
     *
     * Draw Graphics.
     */
    void update();

    /**
     * @function ptr
     * @return
     */
    CanvasDataPtr ptr() {
        return m_canvas;
    }

private:
    Gempyre::CanvasElement m_element;
    Gempyre::CanvasDataPtr m_canvas;
};

/**
 * @class The FrameComposer class
 * typesafe CommandList composer
 */
class FrameComposer {
public:
    /**
     * @function FrameComposer
     */
    FrameComposer() {}
    /**
     * @function FrameComposer
     * @param lst
     */
    FrameComposer(Gempyre::CanvasElement::CommandList& lst) : m_composition(lst) {}
    FrameComposer(FrameComposer&& other) = default;
    FrameComposer(const FrameComposer& other) = default;
    /**
     * @function strokeRect
     * @param r
     * @return
     */
    FrameComposer strokeRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"strokeRect", r.x, r.y, r.width, r.height}); return *this;}
    /**
     * @function clearRect
     * @param r
     * @return
     */
    FrameComposer clearRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"clearRect", r.x, r.y, r.width, r.height}); return *this;}
    /**
     * @function fillRect
     * @param r
     * @return
     */
    FrameComposer fillRect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"fillRect", r.x, r.y, r.width, r.height}); return *this;}
    /**
     * @function fillText
     * @param text
     * @param x
     * @param y
     * @return
     */
    FrameComposer fillText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"fillText", text, x, y}); return *this;}
    /**
     * @function strokeText
     * @param text
     * @param x
     * @param y
     * @return
     */
    FrameComposer strokeText(const std::string& text, double x, double y) {m_composition.insert(m_composition.end(), {"strokeText", text, x, y}); return *this;}
    /**
     * @function arc
     * @param x
     * @param y
     * @param r
     * @param sAngle
     * @param eAngle
     * @return
     */
    FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) {
        m_composition.insert(m_composition.end(), {"arc", x, y, r, sAngle, eAngle}); return *this;}
    /**
     * @function ellipse
     * @param x
     * @param y
     * @param radiusX
     * @param radiusY
     * @param rotation
     * @param startAngle
     * @param endAngle
     * @return
     */
    FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) {
        m_composition.insert(m_composition.end(), {"ellipse", x, y, radiusX, radiusY, rotation, startAngle, endAngle}); return *this;}
    /**
     * @function beginPath
     * @return
     */
    FrameComposer beginPath()  {m_composition.insert(m_composition.end(), {"beginPath"}); return *this;}
    /**
     * @function closePath
     * @return
     */
    FrameComposer closePath() {m_composition.insert(m_composition.end(), {"closePath"}); return *this;}
    /**
     * @function lineTo
     * @param x
     * @param y
     * @return
     */
    FrameComposer lineTo(double x, double y) {m_composition.insert(m_composition.end(), {"lineTo", x, y}); return *this;}
    /**
     * @function moveTo
     * @param x
     * @param y
     * @return
     */
    FrameComposer moveTo(double x, double y)  {m_composition.insert(m_composition.end(), {"moveTo", x, y}); return *this;}
    /**
     * @function bezierCurveTo
     * @param cp1x
     * @param cp1y
     * @param cp2x
     * @param cp2y
     * @param x
     * @param y
     * @return
     */
    FrameComposer bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
        m_composition.insert(m_composition.end(), {"bezierCurveTo", cp1x, cp1y, cp2x, cp2y, x,  y}); return *this;}
    /**
     * @function quadraticCurveTo
     * @param cpx
     * @param cpy
     * @param x
     * @param y
     * @return
     */
    FrameComposer quadraticCurveTo(double cpx, double cpy, double x, double y) {
        m_composition.insert(m_composition.end(), {"quadraticCurveTo", cpx, cpy, x, y}); return *this;}
    /**
     * @function arcTo
     * @param x1
     * @param y1
     * @param x2
     * @param y2
     * @param radius
     * @return
     */
    FrameComposer arcTo(double x1, double y1, double x2, double y2, double radius) {
        m_composition.insert(m_composition.end(), {"arcTo", x1, y1, x2, y2, radius}); return *this;}
    /**
     * @function rect
     * @param r
     * @return
     */
    FrameComposer rect(const Gempyre::Element::Rect& r) {m_composition.insert(m_composition.end(), {"rect", r.x, r.y, r.width, r.height}); return *this;}
    /**
     * @function stroke
     * @return
     */
    FrameComposer stroke() {m_composition.insert(m_composition.end(), {"stroke"}); return *this;}
    /**
     * @function fill
     * @return
     */
    FrameComposer fill() {m_composition.insert(m_composition.end(), {"fill"}); return *this;}
    /**
     * @function fillStyle
     * @param color
     * @return
     */
    FrameComposer fillStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"fillStyle", color}); return *this;}
    /**
     * @function strokeStyle
     * @param color
     * @return
     */
    FrameComposer strokeStyle(const std::string& color) {m_composition.insert(m_composition.end(), {"strokeStyle", color}); return *this;}
    /**
     * @function lineWidth
     * @param width
     * @return
     */
    FrameComposer lineWidth(double width) {m_composition.insert(m_composition.end(), {"lineWidth", width}); return *this;}
    /**
     * @function font
     * @param style
     * @return
     */
    FrameComposer font(const std::string& style) {m_composition.insert(m_composition.end(), {"font", style}); return *this;}
    /**
     * @function textAlign
     * @param align
     * @return
     */
    FrameComposer textAlign(const std::string& align) {m_composition.insert(m_composition.end(), {"textAlign", align}); return *this;}
    /**
     * @function save
     * @return
     */
    FrameComposer save() {m_composition.insert(m_composition.end(), {"save"}); return *this;}
    /**
     * @function restore
     * @return
     */
    FrameComposer restore() {m_composition.insert(m_composition.end(), {"restore"}); return *this;}
    /**
     * @function rotate
     * @param angle
     * @return
     */
    FrameComposer rotate(double angle)  {m_composition.insert(m_composition.end(), {"rotate", angle}); return *this;}
    /**
     * @function translate
     * @param x
     * @param y
     * @return
     */
    FrameComposer translate(double x, double y)  {m_composition.insert(m_composition.end(), {"translate", x, y}); return *this;}
    /**
     * @function scale
     * @param x
     * @param y
     * @return
     */
    FrameComposer scale(const double x, double y)  {m_composition.insert(m_composition.end(), {"scale", x, y}); return *this;}
    /**
     * @function drawImage
     * @param id
     * @param x
     * @param y
     * @return
     */
    FrameComposer drawImage(const std::string& id, double x, double y)  {m_composition.insert(m_composition.end(), {"drawImage", id, x, y}); return *this;}
    /**
     * @function drawImage
     * @param id
     * @param rect
     * @return
     */
    FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& rect)  {m_composition.insert(m_composition.end(), {"drawImageRect", id, rect.x, rect.y, rect.width, rect.height}); return *this;}
    /**
     * @function drawImage
     * @param id
     * @param clip
     * @param rect
     * @return
     */
    FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {m_composition.insert(m_composition.end(), {"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height}); return *this;}
    /**
     * @function composed
     * @return
     */
    const Gempyre::CanvasElement::CommandList& composed() const {return m_composition;}
private:
    Gempyre::CanvasElement::CommandList m_composition;
};

/**
 * @scopeend
 */
}
/**
 * @scopeend
 */
#endif // GEMPYRE_GRAPHICS_H
