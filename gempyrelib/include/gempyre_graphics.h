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

#define gempyre_graphics_assert(b, x) ((b) || GempyreUtils::do_fatal(x, nullptr, __FILE__, __LINE__));

namespace  Gempyre {

class FrameComposer;
class CanvasData;
class Bitmap;
using CanvasDataPtr = std::shared_ptr<CanvasData>;


class GEMPYRE_EX CanvasElement : public Element {
public:
    using Command = std::variant<std::string, double, int>;
    using CommandList = std::vector<Command>;
    using DrawCallback = std::function<void()>;
    
    /// set intial draw
    enum class DrawNotify{NoKick, Kick};
    
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

    std::string add_image(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr);
    std::vector<std::string> add_images(const std::vector<std::string>& urls, const std::function<void(const std::vector<std::string>)>&loaded = nullptr);
    void paint_image(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) const;
    void paint_image(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) const;
    void draw(const CommandList& canvasCommands);
    void draw(const FrameComposer& frameComposer);
    void draw(const Bitmap& bmp, int x, int y);
    /// Set a callback to be called after the draw, drawCompletedCallback can be nullptr
    void draw_completed(DrawCallback&& drawCompletedCallback, DrawNotify kick = DrawNotify::NoKick);
    void erase(bool resized = false);
    [[nodiscard]] bool hasCanvas() const {
        return !!m_tile;
    }
private:
    friend class Bitmap;
    void paint(const CanvasDataPtr& canvas, int x, int y, bool as_draw);
private:
    CanvasDataPtr m_tile;
    int m_width{0};
    int m_height{0};
    DrawCallback m_drawCallback{nullptr};
};

namespace  Color {
    
    using type = Gempyre::dataT;
    [[nodiscard]] static constexpr inline type rgba_clamped(type r, type g, type b, type a = 0xFF) {
        return (0xFF & r) | ((0xFF & g) << 8) | ((0xFF & b) << 16) | ((0xFF & a) << 24);
    }

    [[nodiscard, deprecated("Use snake")]] static constexpr inline type rgbaClamped(type r, type g, type b, type a = 0xFF) {return rgba_clamped(r, g, b, a);}
    [[nodiscard]] static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) {
        return r | (g << 8) | (b << 16) | (a << 24);
    }

    [[nodiscard]] static constexpr inline type r(type pixel) {
        return pixel & static_cast<type>(0xFF);
    }

    [[nodiscard]] static constexpr inline type g(type pixel) {
        return (pixel & static_cast<type>(0xFF00)) >> 8;
    }

    [[nodiscard]] static constexpr inline type b(type pixel) {
        return (pixel & static_cast<type>(0xFF0000)) >> 16;
    }

    [[nodiscard]] static constexpr inline type alpha(type pixel) {
        return (pixel & static_cast<type>(0xFF000000)) >> 24;
    }

    [[nodiscard]] static inline std::string rgba(type pixel) {
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

    [[nodiscard]] static inline std::string rgb(type pixel) {
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

    static constexpr Color::type Black      = Color::rgba(0, 0, 0, 0xFF);
    static constexpr Color::type White      = Color::rgba(0xFF, 0xFF, 0xFF, 0xFF);
    static constexpr Color::type Red        = Color::rgba(0xFF, 0, 0, 0xFF);
    static constexpr Color::type Green      = Color::rgba(0, 0xFF, 0, 0xFF);
    static constexpr Color::type Blue       = Color::rgba(0, 0, 0xFF, 0xFF);
    static constexpr Color::type Cyan       = Color::rgba(0, 0xFF, 0xFF, 0xFF);
    static constexpr Color::type Magenta    = Color::rgba(0xFF, 0, 0xFF, 0xFF);
    static constexpr Color::type Yellow     = Color::rgba(0xFF, 0xFF, 0, 0xFF);
    static constexpr Color::type Aqua       = Cyan;
    static constexpr Color::type Fuchsia    = Magenta;
    static constexpr Color::type Lime       = Green;

}


class GEMPYRE_EX Bitmap {
public:
    Bitmap(int width, int height);
    Bitmap();
    Bitmap(Bitmap&& other) = default;
    Bitmap(const Bitmap& other) = default;
    ~Bitmap();
    Bitmap(const std::vector<unsigned char>& image_data);

    Bitmap& operator=(const Bitmap& other) = default;
    Bitmap& operator=(Bitmap&& other) = default;
    void create(int width, int height);
    Bitmap clone() const;
    static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a);}
    [[deprecated("Use Color")]] static constexpr Color::type Black = Color::Black;
    [[deprecated("Use Color")]] static constexpr Color::type White = Color::White;
    [[deprecated("Use Color")]] static constexpr Color::type Red = Color::Red;
    [[deprecated("Use Color")]] static constexpr Color::type Green = Color::Green;
    [[deprecated("Use Color")]] static constexpr Color::type Blue = Color::Blue;

    void set_pixel(int x, int y, Color::type color);
    void set_alpha(int x, int y, Color::type alpha);
    Color::type pixel(int x, int y) const;
    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;
    void swap(Bitmap& other);
    void draw_rect(const Element::Rect& rect, Color::type color);
    [[deprecated("use merge(other, x, y)")]] void merge(const Bitmap& other) {merge(other, 0, 0);}
    void merge(const Bitmap& other, int x, int y);
private:
    friend class Gempyre::CanvasElement;
    Gempyre::CanvasDataPtr m_canvas;
};

 class // bw compatibility
[[deprecated("Use Bitmap")]] Graphics : public Bitmap {
public:
    Graphics(const Gempyre::CanvasElement& element, int width, int height) : Bitmap(width, height), m_element(element) {};
    Graphics(const Gempyre::CanvasElement& element) : m_element(element) {};
    void update() {m_element.draw(*this, 0, 0);}
private:
    Gempyre::CanvasElement m_element;

};

class FrameComposer {
public:
    FrameComposer() {}
    FrameComposer(Gempyre::CanvasElement::CommandList& lst) : m_composition(lst) {}
    FrameComposer(FrameComposer&& other) = default;
    FrameComposer(const FrameComposer& other) = default;
    FrameComposer stroke_rect(const Gempyre::Element::Rect& r) {return push({"strokeRect", r.x, r.y, r.width, r.height});}
    FrameComposer clear_rect(const Gempyre::Element::Rect& r) {return push({"clearRect", r.x, r.y, r.width, r.height});}
    FrameComposer fill_rect(const Gempyre::Element::Rect& r) {return push({"fillRect", r.x, r.y, r.width, r.height});}
    FrameComposer fill_text(const std::string& text, double x, double y) {return push({"fillText", text, x, y});}
    FrameComposer stroke_text(const std::string& text, double x, double y) {return push({"strokeText", text, x, y});}
    FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) {
        return push({"arc", x, y, r, sAngle, eAngle});}
    FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) {
        return push({"ellipse", x, y, radiusX, radiusY, rotation, startAngle, endAngle});}
    FrameComposer begin_path()  {return push({"beginPath"});}
    FrameComposer close_path() {return push({"closePath"});}
    FrameComposer line_to(double x, double y) {return push({"lineTo", x, y});}
    FrameComposer move_to(double x, double y)  {return push({"moveTo", x, y});}
    FrameComposer bezier_curve_to(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
        return push({"bezierCurveTo", cp1x, cp1y, cp2x, cp2y, x,  y});}
    FrameComposer quadratic_curve_to(double cpx, double cpy, double x, double y) {
        return push({"quadraticCurveTo", cpx, cpy, x, y});}
    FrameComposer arc_to(double x1, double y1, double x2, double y2, double radius) {
        return push({"arcTo", x1, y1, x2, y2, radius});}
    FrameComposer rect(const Gempyre::Element::Rect& r) {return push({"rect", r.x, r.y, r.width, r.height});}
    FrameComposer stroke() {return push({"stroke"});}
    FrameComposer fill() {return push({"fill"});}
    FrameComposer fill_style(const std::string& color) {return push({"fillStyle", color});}
    FrameComposer stroke_style(const std::string& color) {return push({"strokeStyle", color});}
    FrameComposer line_width(double width) {return push({"lineWidth", width});}
    FrameComposer font(const std::string& style) {return push({"font", style});}
    FrameComposer text_align(const std::string& align) {return push({"textAlign", align});}
    FrameComposer save() {return push({"save"});}
    FrameComposer restore() {return push({"restore"});}
    FrameComposer rotate(double angle)  {return push({"rotate", angle});}
    FrameComposer translate(double x, double y)  {return push({"translate", x, y});}
    FrameComposer scale(const double x, double y)  {return push({"scale", x, y});}
    FrameComposer draw_image(const std::string& id, double x, double y)  {return push({"drawImage", id, x, y});}
    FrameComposer draw_image(const std::string& id, const Gempyre::Element::Rect& rect)  {return push({"drawImageRect", id, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer draw_image(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {return push({"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer text_baseline(const std::string& textBaseline) {return push({"textBaseline", textBaseline});}
    [[nodiscard]] const Gempyre::CanvasElement::CommandList& composed() const {return m_composition;}
private:
    FrameComposer push(const std::initializer_list<Gempyre::CanvasElement::Command>& list) {m_composition.insert(m_composition.end(), list); return *this;}
    Gempyre::CanvasElement::CommandList m_composition;
};
}

#endif // GEMPYRE_GRAPHICS_H
