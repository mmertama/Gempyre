#ifndef TELEX_GRAPHICS_H
#define TELEX_GRAPHICS_H

#include <telex.h>

#ifdef WINDOWS_EXPORT
    #ifndef TELEX_EX
        #define TELEX_EX __declspec( dllexport )
    #else
        #define TELEX_EX
    #endif
#endif

#define telex_graphics_assert(b, x) (b || TelexUtils::doFatal(x, nullptr, __FILE__, __LINE__));

namespace  Telex {

/**
 * @class The Canvas class
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
     */
    void put(int x, int y, dataT p) {
        data()[x + y * width] = p;
    }
    /**
     * @function get
     * @param x
     * @param y
     * @return
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
     * @return
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
     * @return
     */
    static constexpr inline dataT rgba(dataT r, dataT g, dataT b, dataT a = 0xFF) {
        return r | (g << 8) | (b << 16) | (a << 24);
    }
    /**
     * @function r
     * @param p
     * @return
     */
    static constexpr inline dataT r(dataT p) {
        return p & static_cast<dataT>(0xFF);
    }
    /**
     * @function g
     * @param p
     * @return
     */
    static constexpr inline dataT g(dataT p) {
        return (p & static_cast<dataT>(0xFF00)) >> 8;
    }
    /**
     * @function b
     * @param p
     * @return
     */
    static constexpr inline dataT b(dataT p) {
        return (p & static_cast<dataT>(0xFF0000)) >> 16;
    }
    /**
     * @function alpha
     * @param p
     * @return
     */
    static constexpr inline dataT alpha(dataT p) {
        return (p & static_cast<dataT>(0xFF000000)) >> 24;
    }
    const int width;
    const int height;
private:
    Canvas(int w, int h, const std::string& owner) : Data(static_cast<unsigned>(w * h), CanvasId, owner,
                                                         {0, 0, static_cast<dataT>(w), static_cast<dataT>(h)}),
        width(w),
        height(h) {}
    friend class CanvasElement;
};

using CanvasPtr = std::shared_ptr<Canvas>;

/**
 * @class The CanvasElement class
 */
class TELEX_EX CanvasElement : public Element {
    static constexpr auto TileWidth = 64;  // used server spesific stuff - bigger than a limit causes random crashes
    static constexpr auto TileHeight = 63; // < 16384 as there are some header info
public:
    ~CanvasElement();
    CanvasElement(const CanvasElement& other) : Element(other) {m_tile = other.m_tile;}
    CanvasElement(CanvasElement&& other) : Element(std::move(other)) {m_tile = std::move(other.m_tile);}
    CanvasElement(Ui& ui, const std::string& id) : Element(ui, id) {}
    CanvasElement(Ui& ui, const std::string& id, Element& parent) : Element(ui, id, "canvas", parent) {}
    CanvasElement& operator=(const CanvasElement& other) = default;
    CanvasElement& operator=(CanvasElement&& other) = default;
    /**
     * @brief makeCanvas
     * @param width
     * @param height
     * @return
     */
    CanvasPtr makeCanvas(int width, int height);
    /**
     * @brief paint
     * @param canvas
     */
    void paint(const CanvasPtr& canvas);
    /**
     * @function addImage
     * @param url
     * @param loaded
     * @return
     */
    std::string addImage(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr);
    /**
      * @function paintImage
      */
    void paintImage(const std::string imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0});
    /**
      * @function paintImage
      */
    void paintImage(const std::string imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0});
private:
    CanvasPtr m_tile;
};

/**
 * @class The Graphics class
 */
class TELEX_EX Graphics {
public:
    using Color = Telex::Data::dataT;
    Graphics(const Telex::CanvasElement& element, int width, int height) : m_element(element), m_canvas(m_element.makeCanvas(width, height)) {
    }
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
     */
    void create(int width, int height) {
        m_canvas = m_element.makeCanvas(width, height);
    }
    /**
     * @function clone
     * @return
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
     * @return
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
     */
    void setPixel(int x, int y, Color color) {m_canvas->put(x, y, color);}
    /**
     * @function setAlpha
     * @param x
     * @param y
     * @param alpha
     */
    void setAlpha(int x, int y, Color alpha) {
        const auto c = m_canvas->get(x, y);
        m_canvas->put(x, y, pix(Telex::Canvas::r(c), Telex::Canvas::g(c), Telex::Canvas::b(c), alpha));
    }
    /**
     * @function width
     * @return
     */
    int width() const {return m_canvas->width;}
    /**
     * @function height
     * @return
     */
    int height() const {return m_canvas->height;}
    /**
     * @function drawRect
     * @param rect
     * @param color
     */
    void drawRect(const Element::Rect& rect, Color color);
    /**
     * @function merge
     * @param other
     */
    void merge(const Graphics& other);
    /**
     * @function swap
     * @param other
     */
    void swap(Graphics& other) {
        m_canvas.swap(other.m_canvas);
    }
    /**
     * @function update
     */
    void update();
private:
    Telex::CanvasElement m_element;
    Telex::CanvasPtr m_canvas;
};

}

#endif // TELEX_GRAPHICS_H
