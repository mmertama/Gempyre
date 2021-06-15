#ifndef GEMPYRE_GRAPHICS_H
#define GEMPYRE_GRAPHICS_H

#include <gempyre.h>
#include <initializer_list>
		

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
  *
  */

#ifdef WINDOWS_EXPORT
    #ifndef GEMPYRE_EX
        #define GGEMPYRE_EX __declspec( dllexport )
    //#else
    //    #define GEMPYRE_EX
    #endif
#endif

#define gempyre_graphics_assert(b, x) ((b) || GempyreUtils::doFatal(x, nullptr, __FILE__, __LINE__));

namespace  Gempyre {

class FrameComposer;

class GEMPYRE_EX CanvasData : public Data {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
    ~CanvasData();
    void put(int x, int y, dataT pixel) {
        data()[x + y * width] = pixel;
    }
    dataT get(int x, int y) const {
        return data()[x + y * width];
    }
    const int width;
    const int height;
private:
    CanvasData(int w, int h, const std::string& owner) : Data(static_cast<unsigned>(w * h), CanvasId, owner,
                                                         {0, 0, static_cast<dataT>(w), static_cast<dataT>(h)}),
        width(w),
        height(h) {}
    friend class CanvasElement;
};


using CanvasDataPtr = std::shared_ptr<CanvasData>;


class GEMPYRE_EX CanvasElement : public Element {
    static constexpr auto TileWidth = 64;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes (There is a issue somewhere, this not really work if something else)
    static constexpr auto TileHeight = 63; // as there are some header info
public:
    using Command = std::variant<std::string, double, int>;
    using CommandList = std::vector<Command>;
    ~CanvasElement();
    CanvasElement(const CanvasElement& other)
        : Element(other) {
        m_tile = other.m_tile;
        m_width = other.m_width;
        m_height = other.m_height;
    }
    CanvasElement(CanvasElement&& other)
        : Element(std::move(other)) {
        m_tile = std::move(other.m_tile);
        m_width = other.m_width;
        m_height = other.m_height;
    }
    CanvasElement(Ui& ui, const std::string& id)
        : Element(ui, id) {}
    CanvasElement(Ui& ui, const std::string& id, const Element& parent)
        : Element(ui, id, "canvas", parent) {}
    CanvasElement& operator=(const CanvasElement& other) = default;
    CanvasElement& operator=(CanvasElement&& other) = default;

    CanvasDataPtr makeCanvas(int width, int height);
    std::string addImage(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr);
    std::vector<std::string> addImages(const std::vector<std::string>& urls, const std::function<void(const std::vector<std::string>)>&loaded = nullptr);
    void paintImage(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) const;
    void paintImage(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) const;
    void draw(const CommandList& canvasCommands) const;
    void draw(const FrameComposer& frameComposer) const;
    void erase(bool resized = false) const;
    bool hasCanvas() const {
        return !!m_tile;
    }
private:
    friend class Graphics;
    void paint(const CanvasDataPtr& canvas);
private:
    CanvasDataPtr m_tile;
    mutable int m_width = 0;
    mutable int m_height = 0;
};

namespace  Color {
using type = Gempyre::Data::dataT;
static constexpr inline type rgbaClamped(type r, type g, type b, type a = 0xFF) {
    return (0xFF & r) | ((0xFF & g) << 8) | ((0xFF & b) << 16) | ((0xFF & a) << 24);
}
static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) {
    return r | (g << 8) | (b << 16) | (a << 24);
}
static constexpr inline type r(type pixel) {
    return pixel & static_cast<type>(0xFF);
}
static constexpr inline type g(type pixel) {
    return (pixel & static_cast<type>(0xFF00)) >> 8;
}
static constexpr inline type b(type pixel) {
    return (pixel & static_cast<type>(0xFF0000)) >> 16;
}
static constexpr inline type alpha(type pixel) {
    return (pixel & static_cast<type>(0xFF000000)) >> 24;
}

static inline std::string rgba(type pixel) {
    constexpr auto c = "0123456789ABCDEF";
    std::string v("#RRGGBBAA");
    v[1] =  c[r(pixel) >> 4];
    v[2] =  c[r(pixel) & 0xF];
    v[3] =  c[g(pixel) >> 4];
    v[4] =  c[g(pixel) & 0xF];
    v[5] =  c[b(pixel) >> 4];
    v[6] =  c[b(pixel) & 0xF];
    v[7] =  c[alpha(pixel) >> 4];
    v[8] =  c[alpha(pixel) & 0xF];
    return v;
}

static inline std::string rgb(type pixel) {
    constexpr auto c = "0123456789ABCDEF";
    std::string v("#RRGGBB");
    v[1] =  c[r(pixel) >> 4];
    v[2] =  c[r(pixel) & 0xF];
    v[3] =  c[g(pixel) >> 4];
    v[4] =  c[g(pixel) & 0xF];
    v[5] =  c[b(pixel) >> 4];
    v[6] =  c[b(pixel) & 0xF];
    return v;
}

}


class GEMPYRE_EX Graphics {
public:
    Graphics(const Gempyre::CanvasElement& element, int width, int height);
    Graphics(const Gempyre::CanvasElement& element);
    Graphics(Graphics&& other) = default;
    Graphics(const Graphics& other) = default;
    Graphics& operator=(const Graphics& other) = default;
    Graphics& operator=(Graphics&& other) = default;
    void create(int width, int height) {
        m_canvas = m_element.makeCanvas(width, height);
    }
    Graphics clone() const;
    static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a);}
    static constexpr Color::type Black = Color::rgba(0, 0, 0, 0xFF);
    static constexpr Color::type White = Color::rgba(0xFF, 0xFF, 0xFF, 0xFF);
    static constexpr Color::type Red = Color::rgba(0xFF, 0, 0, 0xFF);
    static constexpr Color::type Green = Color::rgba(0, 0xFF, 0, 0xFF);
    static constexpr Color::type Blue = Color::rgba(0, 0xFF, 0, 0xFF);

    void setPixel(int x, int y, Color::type color) {
        m_canvas->put(x, y, color);
    }

    void setAlpha(int x, int y, Color::type alpha) {
        const auto c = m_canvas->get(x, y);
        m_canvas->put(x, y, pix(Color::r(c), Color::g(c), Color::b(c), alpha));
    }
    int width() const {
        return m_canvas->width;
    }
    int height() const {
        return m_canvas->height;
    }
    void drawRect(const Element::Rect& rect, Color::type color);
    void merge(const Graphics& other);
    void swap(Graphics& other) {
        m_canvas.swap(other.m_canvas);
    }
    void update();
    CanvasDataPtr ptr() {
        return m_canvas;
    }

private:
    Gempyre::CanvasElement m_element;
    Gempyre::CanvasDataPtr m_canvas;
};


class FrameComposer {
public:
    FrameComposer() {}
    FrameComposer(Gempyre::CanvasElement::CommandList& lst) : m_composition(lst) {}
    FrameComposer(FrameComposer&& other) = default;
    FrameComposer(const FrameComposer& other) = default;
    FrameComposer strokeRect(const Gempyre::Element::Rect& r) {return push({"strokeRect", r.x, r.y, r.width, r.height});}
    FrameComposer clearRect(const Gempyre::Element::Rect& r) {return push({"clearRect", r.x, r.y, r.width, r.height});}
    FrameComposer fillRect(const Gempyre::Element::Rect& r) {return push({"fillRect", r.x, r.y, r.width, r.height});}
    FrameComposer fillText(const std::string& text, double x, double y) {return push({"fillText", text, x, y});}
    FrameComposer strokeText(const std::string& text, double x, double y) {return push({"strokeText", text, x, y});}
    FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) {
        return push({"arc", x, y, r, sAngle, eAngle});}
    FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) {
        return push({"ellipse", x, y, radiusX, radiusY, rotation, startAngle, endAngle});}
    FrameComposer beginPath()  {return push({"beginPath"});}
    FrameComposer closePath() {return push({"closePath"});}
    FrameComposer lineTo(double x, double y) {return push({"lineTo", x, y});}
    FrameComposer moveTo(double x, double y)  {return push({"moveTo", x, y});}
    FrameComposer bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
        return push({"bezierCurveTo", cp1x, cp1y, cp2x, cp2y, x,  y});}
    FrameComposer quadraticCurveTo(double cpx, double cpy, double x, double y) {
        return push({"quadraticCurveTo", cpx, cpy, x, y});}
    FrameComposer arcTo(double x1, double y1, double x2, double y2, double radius) {
        return push({"arcTo", x1, y1, x2, y2, radius});}
    FrameComposer rect(const Gempyre::Element::Rect& r) {return push({"rect", r.x, r.y, r.width, r.height});}
    FrameComposer stroke() {return push({"stroke"});}
    FrameComposer fill() {return push({"fill"});}
    FrameComposer fillStyle(const std::string& color) {return push({"fillStyle", color});}
    FrameComposer strokeStyle(const std::string& color) {return push({"strokeStyle", color});}
    FrameComposer lineWidth(double width) {return push({"lineWidth", width});}
    FrameComposer font(const std::string& style) {return push({"font", style});}
    FrameComposer textAlign(const std::string& align) {return push({"textAlign", align});}
    FrameComposer save() {return push({"save"});}
    FrameComposer restore() {return push({"restore"});}
    FrameComposer rotate(double angle)  {return push({"rotate", angle});}
    FrameComposer translate(double x, double y)  {return push({"translate", x, y});}
    FrameComposer scale(const double x, double y)  {return push({"scale", x, y});}
    FrameComposer drawImage(const std::string& id, double x, double y)  {return push({"drawImage", id, x, y});}
    FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& rect)  {return push({"drawImageRect", id, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer drawImage(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {return push({"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer textBaseline(const std::string& textBaseline) {return push({"textBaseline", textBaseline});}
    const Gempyre::CanvasElement::CommandList& composed() const {return m_composition;}
private:
    FrameComposer push(const std::initializer_list<Gempyre::CanvasElement::Command>& list) {m_composition.insert(m_composition.end(), list); return *this;}
    Gempyre::CanvasElement::CommandList m_composition;
};
}

#endif // GEMPYRE_GRAPHICS_H
