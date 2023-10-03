#ifndef GEMPYRE_GRAPHICS_H
#define GEMPYRE_GRAPHICS_H

#include <gempyre.h>
#include <initializer_list>
#include <variant>
		
/**
  * @file
  * 
  * ![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&v=4)
  *
  * gempyre_graphics.h API for drawing on HTML canvas
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

/// @brief 
class FrameComposer;
class CanvasData;
class Bitmap;
using CanvasDataPtr = std::shared_ptr<CanvasData>;

/// @brief Graphics element
class GEMPYRE_EX CanvasElement : public Element {
public:
    /// @brief Canvas draw command type.
    using Command = std::variant<std::string, double, int>;
    /// @brief List of Canvas draw commands.
    using CommandList = std::vector<Command>;
    /// @brief Function type for draw notifies. @see CanvasElement::draw_completed and @see DrawNotify.
    using DrawCallback = std::function<void()>;
    
    /// set initial draw, @see CanvasElement::draw_completed()
    enum class DrawNotify{NoKick, Kick};
    
    /// Destructor.
    ~CanvasElement();
    
    /// Copy constructor. 
    CanvasElement(const CanvasElement& other);

    /// Move constructor. 
    CanvasElement(CanvasElement&& other);
    
    /// @brief Constructor of canvas id.
    /// @param ui ref.
    /// @param id HTML id, should be canvas. 
    CanvasElement(Ui& ui, const std::string& id);
    
    /// @brief Constructor to create a new CanvasElement
    /// @param ui ref
    /// @param id HTML id
    /// @param parent parent element
    CanvasElement(Ui& ui, const std::string& id, const Element& parent);

    /// @brief Constructor to create a new CanvasElement
    /// @param ui ref
    /// @param parent parent element
    CanvasElement(Ui& ui, const Element& parent);

    /// Copy operator. 
    CanvasElement& operator=(const CanvasElement& other);

    /// Move operator.
    CanvasElement& operator=(CanvasElement&& other);

    /// @brief Add an image into HTML DOM tree.
    /// @param url address of image, can be either a resource or any http url.
    /// @param loaded callback called when image is loaded.
    /// @return image id.
    std::string add_image(const std::string& url, const std::function<void (const std::string& id)>& loaded = nullptr);
    
    /// @brief Draw image at position.
    /// @param imageId image id.
    /// @param x image x coordinate.
    /// @param y image y coordinate.
    /// @param clippingRect optional imake clipping rectangle.
    /// @details paint image does not call draw_completed callback. 
    void paint_image(const std::string& imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) const;
    
    /// @brief Draw image in rectangle.
    /// @param imageId image id.
    /// @param targetRect image is resized in the rectangle.
    /// @param clippingRect optional imake clipping rectangle.
    /// @details paint image does not call draw_completed callback. 
    void paint_image(const std::string& imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) const;
    
    /// @brief Draw command list - please prefer @see draw(FrameComposer) 
    /// @param canvasCommands 
    void draw(const CommandList& canvasCommands);

    /// @brief Draw Frame Composer
    /// @param frameComposer 
    void draw(const FrameComposer& frameComposer);

    /// @brief Draw bitmap
    /// @param bmp 
    void draw(const Bitmap& bmp) {draw(0, 0, bmp);}
    
    /// @brief Draw bitmap at position
    /// @param x 
    /// @param y 
    /// @param bmp 
    void draw(int x, int y, const Bitmap& bmp); 

    /// @brief Set a callback to be called after the draw
    /// @param drawCompletedCallback - function called after draw.
    /// @param kick - optional whether callback is called 1st time automatically.
    /// @details - When doing animation or frequent drawing, please do no call draw functions inside timer function.
    /// The preferred way is to to do animation in timer function, but use draw_completed to do the actual drawing. 
    /// draw_completed do drawing in the optimal frequency (as fast the system can do it without consuming all CPU).
    /// @note
    /// @code{.cpp}
    /// canvas_element.draw_completed([this]() {draw_frame();}, Gempyre::CanvasElement::DrawNotify::Kick);
    /// ui.start_periodic(50ms, [this]() {animate();});
    /// @endcode
    void draw_completed(const DrawCallback& drawCompletedCallback, DrawNotify kick = DrawNotify::NoKick);
    
    /// @brief erase bitmap
    /// @param resized - make an explicit query to ask canvas current size
    void erase(bool resized = false);
private:
    friend class Bitmap;
    void paint(const CanvasDataPtr& canvas, int x, int y, bool as_draw);
private:
    CanvasDataPtr m_tile{};
    int m_width{0};
    int m_height{0};
};

/// @brief RGB handling
namespace  Color {
    /// @brief pixel type
    using type = Gempyre::dataT;

    /// Pack r,g b and a components by saturating value range. 
    [[nodiscard]] static constexpr inline type rgba_clamped(type r, type g, type b, type a = 0xFF) {
        const type FF = 0xFF;
        return std::min(FF, r) | (std::min(FF, g) << 8) | (std::min(FF, b) << 16) | (std::min(FF, a) << 24);
    }

    /// Pack r,g b, and a components into pixel. Each component is expected to be less than 256. 
    [[nodiscard]] static constexpr inline type rgba(type r, type g, type b, type a = 0xFF) {
        return r | (g << 8) | (b << 16) | (a << 24);
    }

    /// Pack r,g and b components into pixel. 
    [[nodiscard]] static constexpr inline type rgb(type r, type g, type b) {
        return r | (g << 8) | (b << 16) | (static_cast<type>(0xFF) << 24);
    }

    /// Get red component.
    [[nodiscard]] static constexpr inline type r(type pixel) {
        return pixel & static_cast<type>(0xFF);
    }

    /// Get green component.
    [[nodiscard]] static constexpr inline type g(type pixel) {
        return (pixel & static_cast<type>(0xFF00)) >> 8;
    }

    /// Get blue compoennt.
    [[nodiscard]] static constexpr inline type b(type pixel) {
        return (pixel & static_cast<type>(0xFF0000)) >> 16;
    }

    /// Get alpha component.
    [[nodiscard]] static constexpr inline type alpha(type pixel) {
        return (pixel & 0xFF000000) >> 24;
    }

    /// Get pixel as a HTML string.
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

    /// Get pixel as a HTML string.
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

    /// Get components as a HTML string 
    [[nodiscard]] static inline std::string to_string(type r, type g, type b, type a = 0xFF) {
        return a == 0xFF ? Gempyre::Color::rgb(Gempyre::Color::rgb(r, g, b)) : Gempyre::Color::rgba(Gempyre::Color::rgba(r, g, b, a));
    }

    /// Get color as a HYML string 
    [[nodiscard]] static inline std::string to_string(Gempyre::Color::type color) {
        return Gempyre::Color::to_string(
            Gempyre::Color::r(color),
            Gempyre::Color::g(color),
            Gempyre::Color::b(color),
            Gempyre::Color::alpha(color));
    }

    /// @brief Black
    static constexpr Color::type Black      = Color::rgba(0, 0, 0, 0xFF);
    /// @brief White
    static constexpr Color::type White      = Color::rgba(0xFF, 0xFF, 0xFF, 0xFF);
    /// @brief Red
    static constexpr Color::type Red        = Color::rgba(0xFF, 0, 0, 0xFF);
    /// @brief Green
    static constexpr Color::type Green      = Color::rgba(0, 0xFF, 0, 0xFF);
    /// @brief Blue
    static constexpr Color::type Blue       = Color::rgba(0, 0, 0xFF, 0xFF);
    /// @brief Cyan
    static constexpr Color::type Cyan       = Color::rgba(0, 0xFF, 0xFF, 0xFF);
    /// @brief  Magenta
    static constexpr Color::type Magenta    = Color::rgba(0xFF, 0, 0xFF, 0xFF);
    /// @brief Yellow
    static constexpr Color::type Yellow     = Color::rgba(0xFF, 0xFF, 0, 0xFF);
    /// @brief Cyan
    static constexpr Color::type Aqua       = Cyan;
    /// @brief Fuchsia
    static constexpr Color::type Fuchsia    = Magenta;
    /// @brief Lime
    static constexpr Color::type Lime       = Green;

}

/// @brief Bitmap for Gempyre Graphics
class GEMPYRE_EX Bitmap {
public:
    /// @brief Constructor - unitialialized data
    /// @param width 
    /// @param height 
    Bitmap(int width, int height);

    /// @brief Constructor - with a single color data
    /// @param width 
    /// @param height 
    /// @param color 
    Bitmap(int width, int height, Gempyre::Color::type color);
    
    /// @brief Constructor - zero size, use @see create() to create the actual bitmap.
    Bitmap();
    
    /// @brief Move constructor. 
    Bitmap(Bitmap&& other) = default;
    
    /// Copy constructor - does not copy the data, for deep copy @see clone() 
    Bitmap(const Bitmap& other) = default;
    
    /// Destructor
    ~Bitmap();

    /// @brief Create bitmap from byte array. 
    /// @param image_data PNG image in bytes.
    /// @note
    /// @code{.cpp}
    /// const auto bytes = GempyreUtils::slurp<uint8_t>("image.png");
    /// Bitmap bmp(bytes);
    /// @endcode
    Bitmap(const std::vector<uint8_t>& image_data);

    /// @brief Convert a bitmap to PNG
    /// @return PNG bytes
    const std::vector<uint8_t> png_image() const;

    /// Copy operator does only shallow copy, for deep copy @see clone() 
    Bitmap& operator=(const Bitmap& other) = default;
    
    /// Move operator. 
    Bitmap& operator=(Bitmap&& other) = default;

    /// @brief  Create bitmap bytes.
    /// @param width 
    /// @param height 
    void create(int width, int height);

    /// Deep copy bitmap bytes.
    Bitmap clone() const;

    /// Components to pixel type. 
    static constexpr Color::type pix(Color::type r, Color::type g, Color::type b, Color::type a = 0xFF) {return Color::rgba(r, g, b, a);}
    
    /// Set a single pixel.
    void set_pixel(int x, int y, Color::type color);

    /// Set a singe pixel's alpha value. 
    void set_alpha(int x, int y, Color::type alpha);

    /// Get a single pixel.
    Color::type pixel(int x, int y) const;

    /// Get width.
    [[nodiscard]] int width() const;

    /// Get height.
    [[nodiscard]] int height() const;

    /// Swap bitmap data with another. 
    void swap(Bitmap& other);

    /// Draw a rect with a color in bitmap.
    void draw_rect(const Element::Rect& rect, Color::type color);

    /// Draw a Bitmap on this bitmap - merge alpha.  
    void merge(int x, int y, const Bitmap& other);

    /// Draw a Bitmap on this bitmap - merge alpha.  
    void merge(const Bitmap& other) {merge(0, 0, other);}

     /// Draw a Bitmap on this bitmap - replace area.  
    void tile(int x, int y, const Bitmap& other);

    /// Create a new bitmap from part of bitmap
    Bitmap clip(const Element::Rect& rect) const;

    /// return true if there is not data  
    bool empty() const;
protected:
    /// @cond INTERNAL
    void copy_from(const Bitmap& other);
    /// @endcond
private:
    friend class Gempyre::CanvasElement;
    Gempyre::CanvasDataPtr m_canvas{};
};


/// @brief - wrap up Javascript draw commands.
class FrameComposer {
public:
    /// @brief Constructor.
    FrameComposer() {}
    /// @brief Construct from CommandList. 
    FrameComposer(Gempyre::CanvasElement::CommandList& lst) : m_composition(lst) {}
    /// @brief Move constructor. 
    FrameComposer(FrameComposer&& other) = default;
    /// @brief Copy constructor. 
    FrameComposer(const FrameComposer& other) = default;
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer stroke_rect(const Gempyre::Element::Rect& r) {return push({"strokeRect", r.x, r.y, r.width, r.height});}
    FrameComposer stroke_rect(double x, double y, double w, double h) {return push({"strokeRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer clear_rect(const Gempyre::Element::Rect& r) {return push({"clearRect", r.x, r.y, r.width, r.height});}
    FrameComposer clear_rect(double x, double y, double w, double h) {return push({"clearRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer fill_rect(const Gempyre::Element::Rect& r) {return push({"fillRect", r.x, r.y, r.width, r.height});}
    FrameComposer fill_rect(double x, double y, double w, double h) {return push({"fillRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer fill_text(const std::string& text, double x, double y) {return push({"fillText", text, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer stroke_text(const std::string& text, double x, double y) {return push({"strokeText", text, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) {
        return push({"arc", x, y, r, sAngle, eAngle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) {
        return push({"ellipse", x, y, radiusX, radiusY, rotation, startAngle, endAngle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer begin_path()  {return push({"beginPath"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer close_path() {return push({"closePath"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer line_to(double x, double y) {return push({"lineTo", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer move_to(double x, double y)  {return push({"moveTo", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer bezier_curve_to(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
        return push({"bezierCurveTo", cp1x, cp1y, cp2x, cp2y, x,  y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer quadratic_curve_to(double cpx, double cpy, double x, double y) {
        return push({"quadraticCurveTo", cpx, cpy, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer arc_to(double x1, double y1, double x2, double y2, double radius) {
        return push({"arcTo", x1, y1, x2, y2, radius});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer rect(const Gempyre::Element::Rect& r) {return push({"rect", r.x, r.y, r.width, r.height});}
    FrameComposer rect(double x, double y, double w, double h) {return push({"rect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer stroke() {return push({"stroke"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer fill() {return push({"fill"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer fill_style(const std::string& color) {return push({"fillStyle", color});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer stroke_style(const std::string& color) {return push({"strokeStyle", color});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer line_width(double width) {return push({"lineWidth", width});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer font(const std::string& style) {return push({"font", style});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer text_align(const std::string& align) {return push({"textAlign", align});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer save() {return push({"save"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer restore() {return push({"restore"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer rotate(double angle)  {return push({"rotate", angle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer translate(double x, double y)  {return push({"translate", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer scale(const double x, double y)  {return push({"scale", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer draw_image(const std::string& id, double x, double y)  {return push({"drawImage", id, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer draw_image(const std::string& id, const Gempyre::Element::Rect& rect)  {return push({"drawImageRect", id, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer draw_image(const std::string& id, double x, double y, double w, double h)  {return push({"drawImageRect", id, x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer draw_image(const std::string& id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {return push({"drawImageClip", id, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height});}
    FrameComposer draw_image(const std::string& id, double cx, double cy, double cw, double ch, double x, double y, double w, double h) {return push({"drawImageClip", id, cx, cy, cw, ch, x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozialla documentation</a>
    FrameComposer text_baseline(const std::string& textBaseline) {return push({"textBaseline", textBaseline});}
    /// @brief Get command list composed.
    [[nodiscard]] const Gempyre::CanvasElement::CommandList& composed() const {return m_composition;}
private:
    FrameComposer push(const std::initializer_list<Gempyre::CanvasElement::Command>& list) {m_composition.insert(m_composition.end(), list); return *this;}
    Gempyre::CanvasElement::CommandList m_composition{};
};
}

#endif // GEMPYRE_GRAPHICS_H
