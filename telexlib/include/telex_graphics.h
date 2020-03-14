#ifndef TELEX_GRAPHICS_H
#define TELEX_GRAPHICS_H

#include <telex.h>

/**
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * telex_graphics.h
  * =====
  * Telex GUI Framework
  * -------------
  *
  * telex_graphics.h provides a low level graphics capabilites for Telex. As Telex Element
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
    #ifndef TELEX_EX
        #define TELEX_EX __declspec( dllexport )
    #else
        #define TELEX_EX
    #endif
#endif

#define telex_graphics_assert(b, x) (b || TelexUtils::doFatal(x, nullptr, __FILE__, __LINE__));

/**
 * @namespace Telex
 *
 * Common namespace for Telex implementation.
 */
namespace  Telex {

/**
 * @class The Canvas class
 *
 * Bitmap canvas. You normally use this calls using the Graphics (below) compositor.
 */
class TELEX_EX Canvas : public Data {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
    ~Canvas();
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
     * @function rgbaClamped
     * @param r
     * @param g
     * @param b
     * @param a
     * @return pixel value
     *
     * Creates a pixel from rgb or rgba.
     */
    static constexpr inline dataT rgbaClamped(dataT r, dataT g, dataT b, dataT a = 0xFF) {
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
    static constexpr inline dataT rgba(dataT r, dataT g, dataT b, dataT a = 0xFF) {
        return r | (g << 8) | (b << 16) | (a << 24);
    }
    /**
     * @function r
     * @param pixel
     * @return dataT
     *
     * Return pixel with only red component.
     */
    static constexpr inline dataT r(dataT pixel) {
        return pixel & static_cast<dataT>(0xFF);
    }
    /**
     * @function g
     * @param pixel
     * @return dataT
     *
     * Return pixel with only green component.
     */
    static constexpr inline dataT g(dataT pixel) {
        return (pixel & static_cast<dataT>(0xFF00)) >> 8;
    }
    /**
     * @function b
     * @param pixel
     * @return dataT
     *
     * Return pixel with only blue component.
     */
    static constexpr inline dataT b(dataT pixel) {
        return (pixel & static_cast<dataT>(0xFF0000)) >> 16;
    }
    /**
     * @function alpha
     * @param pixel
     * @return dataT
     *
     * Return pixel with only alpha component.
     */
    static constexpr inline dataT alpha(dataT pixel) {
        return (pixel & static_cast<dataT>(0xFF000000)) >> 24;
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
    Canvas(int w, int h, const std::string& owner) : Data(static_cast<unsigned>(w * h), CanvasId, owner,
                                                         {0, 0, static_cast<dataT>(w), static_cast<dataT>(h)}),
        width(w),
        height(h) {}
    friend class CanvasElement;
};
/**
 * @scopeend
 */

using CanvasPtr = std::shared_ptr<Canvas>;

/**
 * @class CanvasElement
 *
 * The CanvasElement class
 */
class TELEX_EX CanvasElement : public Element {
    static constexpr auto TileWidth = 64;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes
    static constexpr auto TileHeight = 63; // as there are some header info
public:
    ~CanvasElement();
    /**
     * @function CanvasElement
     * @param other
     *
     * Copy
     */
    CanvasElement(const CanvasElement& other) : Element(other) {m_tile = other.m_tile;}
    /**
     * @function CanvasElement
     * @param other
     *
     * Move
     */
    CanvasElement(CanvasElement&& other) : Element(std::move(other)) {m_tile = std::move(other.m_tile);}
    /**
     * @function CanvasElement
     * @param ui
     * @param id
     *
     * Access an existing HTML Canvas element.
     */
    CanvasElement(Ui& ui, const std::string& id) : Element(ui, id) {}
    /**
     * @brief CanvasElement
     * @param ui
     * @param id
     * @param parent
     *
     * Createa a new HTML Canvas element.
     */
    CanvasElement(Ui& ui, const std::string& id, Element& parent) : Element(ui, id, "canvas", parent) {}
    CanvasElement& operator=(const CanvasElement& other) = default;
    CanvasElement& operator=(CanvasElement&& other) = default;
    /**
     * @brief makeCanvas
     * @param width
     * @param height
     * @return CanvasPtr
     *
     * Pointer to bitmap. You normally call this class directly, but create a Graphics compositor (below).
     */
    CanvasPtr makeCanvas(int width, int height);
    /**
     * @brief paint
     * @param canvas
     *
     * Draw a given bitmap.
     */
    void paint(const CanvasPtr& canvas);
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
      * @function paintImage
      * @param imageId
      * @param x
      * @param y
      * @param clippingRect
      *
      * Paint an image.
      */
    void paintImage(const std::string imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0});
    /**
      * @function paintImage
      * @param imageId
      * @param targetRect
      * @param clippingRect
      *
      * Paint an image.
      */
    void paintImage(const std::string imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0});
private:
    CanvasPtr m_tile;
};
/**
 * @scopeend
 */
/**
 * @class Graphics
 * The Graphics class, a simple compositor to clone and merge Canvases. Besides a simple bitmap manipulation the a rect drawing
 * primitive is provided.
 */
class TELEX_EX Graphics {
public:
    using Color = Telex::Data::dataT;
    /**
     * @function Graphics
     * @param element
     * @param width
     * @param height
     *
     * Construct a Graphics and create a Canvas.
     */
    Graphics(const Telex::CanvasElement& element, int width, int height) : m_element(element), m_canvas(m_element.makeCanvas(width, height)) {
    }
    /**
     * @function Graphics
     * @param element
     *
     * Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
     */
    Graphics(const Telex::CanvasElement& element) : m_element(element) {
    }
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
    Graphics clone() const {
        Graphics other(m_element);
        other.create(m_canvas->width, m_canvas->height);
        std::copy(m_canvas->begin(), m_canvas->end(), other.m_canvas->data());
        return other;
    }
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
    static constexpr Color pix(Color r, Color g, Color b, Color a = 0xFF) {return Telex::Canvas::rgba(r, g, b, a);}
    static constexpr Color Black = Telex::Canvas::rgba(0, 0, 0, 0xFF);
    static constexpr Color White = Telex::Canvas::rgba(0xFF, 0xFF, 0xFF, 0xFF);
    static constexpr Color Red = Telex::Canvas::rgba(0xFF, 0, 0, 0xFF);
    static constexpr Color Green = Telex::Canvas::rgba(0, 0xFF, 0, 0xFF);
    static constexpr Color Blue = Telex::Canvas::rgba(0, 0xFF, 0, 0xFF);
    /**
     * @function setPixel
     * @param x
     * @param y
     * @param color
     *
     * Set a pixel.
     */
    void setPixel(int x, int y, Color color) {m_canvas->put(x, y, color);}
    /**
     * @function setAlpha
     * @param x
     * @param y
     * @param alpha
     *
     * Set alpha.
     */
    void setAlpha(int x, int y, Color alpha) {
        const auto c = m_canvas->get(x, y);
        m_canvas->put(x, y, pix(Telex::Canvas::r(c), Telex::Canvas::g(c), Telex::Canvas::b(c), alpha));
    }
    /**
     * @function width
     * @return int
     *
     */
    int width() const {return m_canvas->width;}
    /**
     * @function height
     * @return int
     */
    int height() const {return m_canvas->height;}
    /**
     * @function drawRect
     * @param rect
     * @param color
     *
     * Draw a rect.
     */
    void drawRect(const Element::Rect& rect, Color color);
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
private:
    Telex::CanvasElement m_element;
    Telex::CanvasPtr m_canvas;
};
/**
 * @scopeend
 */
}
/**
 * @scopeend
 */
#endif // TELEX_GRAPHICS_H
