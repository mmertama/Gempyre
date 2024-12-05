#ifndef GEMPYRE_GRAPHICS_H
#define GEMPYRE_GRAPHICS_H

#include <initializer_list>
#include <variant>
#include <array>
#include <string_view>
		
#include <gempyre.h>
#include <gempyre_bitmap.h> // for compatibility, not really needed, fwd declaration is sufficient

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
    CanvasElement(Ui& ui, std::string_view id);
    
    /// @brief Constructor to create a new CanvasElement
    /// @param ui ref
    /// @param id HTML id
    /// @param parent parent element
    CanvasElement(Ui& ui, std::string_view id, const Element& parent);

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
    std::string add_image(std::string_view url, const std::function<void (std::string_view id)>& loaded = nullptr);
    
    /// @brief Draw image at position.
    /// @param imageId image id.
    /// @param x image x coordinate.
    /// @param y image y coordinate.
    /// @param clippingRect optional image clipping rectangle.
    /// @details paint image does not call draw_completed callback. 
    void paint_image(std::string_view imageId, int x, int y, const Element::Rect& clippingRect  = {0, 0, 0, 0}) const;
    
    /// @brief Draw image in rectangle.
    /// @param imageId image id.
    /// @param targetRect image is resized in the rectangle.
    /// @param clippingRect optional image clipping rectangle.
    /// @details paint image does not call draw_completed callback. 
    void paint_image(std::string_view imageId, const Element::Rect& targetRect, const Element::Rect& clippingRect = {0, 0, 0, 0}) const;
    
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
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer stroke_rect(const Gempyre::Element::Rect& r) {return push({"strokeRect", r.x, r.y, r.width, r.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer stroke_rect(double x, double y, double w, double h) {return push({"strokeRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer clear_rect(const Gempyre::Element::Rect& r) {return push({"clearRect", r.x, r.y, r.width, r.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer clear_rect(double x, double y, double w, double h) {return push({"clearRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer fill_rect(const Gempyre::Element::Rect& r) {return push({"fillRect", r.x, r.y, r.width, r.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer fill_rect(double x, double y, double w, double h) {return push({"fillRect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer fill_text(std::string_view text, double x, double y) {return push({"fillText", std::string{text}, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer stroke_text(std::string_view text, double x, double y) {return push({"strokeText", std::string{text}, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer arc(double x, double y, double r, double sAngle, double eAngle) {
        return push({"arc", x, y, r, sAngle, eAngle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle) {
        return push({"ellipse", x, y, radiusX, radiusY, rotation, startAngle, endAngle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer begin_path()  {return push({"beginPath"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer close_path() {return push({"closePath"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer line_to(double x, double y) {return push({"lineTo", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer move_to(double x, double y)  {return push({"moveTo", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer bezier_curve_to(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
        return push({"bezierCurveTo", cp1x, cp1y, cp2x, cp2y, x,  y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer quadratic_curve_to(double cpx, double cpy, double x, double y) {
        return push({"quadraticCurveTo", cpx, cpy, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer arc_to(double x1, double y1, double x2, double y2, double radius) {
        return push({"arcTo", x1, y1, x2, y2, radius});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer rect(const Gempyre::Element::Rect& r) {return push({"rect", r.x, r.y, r.width, r.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer rect(double x, double y, double w, double h) {return push({"rect", x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer stroke() {return push({"stroke"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer fill() {return push({"fill"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer fill_style(std::string_view color) {return push({"fillStyle", std::string{color}});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer stroke_style(std::string_view color) {return push({"strokeStyle", std::string{color}});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer line_width(double width) {return push({"lineWidth", width});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer font(std::string_view style) {return push({"font", std::string{style}});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer text_align(std::string_view align) {return push({"textAlign", std::string{align}});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer save() {return push({"save"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer restore() {return push({"restore"});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer rotate(double angle)  {return push({"rotate", angle});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer translate(double x, double y)  {return push({"translate", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer scale(const double x, double y)  {return push({"scale", x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer draw_image(std::string_view id, double x, double y)  {return push({"drawImage", std::string{id}, x, y});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer draw_image(std::string_view id, const Gempyre::Element::Rect& rect)  {return push({"drawImageRect", std::string{id}, rect.x, rect.y, rect.width, rect.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer draw_image(std::string_view id, double x, double y, double w, double h)  {return push({"drawImageRect", std::string{id}, x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer draw_image(std::string_view id, const Gempyre::Element::Rect& clip, const Gempyre::Element::Rect& rect) {return push({"drawImageClip", std::string{id}, clip.x, clip.y, clip.width, clip.height, rect.x, rect.y, rect.width, rect.height});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer draw_image(std::string_view id, double cx, double cy, double cw, double ch, double x, double y, double w, double h) {return push({"drawImageClip", std::string{id}, cx, cy, cw, ch, x, y, w, h});}
    /// @brief Visit the <a href="https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial/Drawing_shapes">Mozilla documentation</a>
    FrameComposer text_baseline(std::string_view textBaseline) {return push({"textBaseline", std::string{textBaseline}});}
    /// @brief Get command list composed.
    [[nodiscard]] const Gempyre::CanvasElement::CommandList& composed() const {return m_composition;}
private:
    FrameComposer push(const std::initializer_list<Gempyre::CanvasElement::Command>& list) {m_composition.insert(m_composition.end(), list); return *this;}
    Gempyre::CanvasElement::CommandList m_composition{};
};

/// @brief HTML colors
constexpr std::array<std::pair<std::string_view, uint32_t>, 147> html_colors = {{
    {"AliceBlue", 0xF0F8FF}, {"AntiqueWhite", 0xFAEBD7}, {"Aqua", 0x00FFFF},
    {"Aquamarine", 0x7FFFD4}, {"Azure", 0xF0FFFF}, {"Beige", 0xF5F5DC},
    {"Bisque", 0xFFE4C4}, {"Black", 0x000000}, {"BlanchedAlmond", 0xFFEBCD},
    {"Blue", 0x0000FF}, {"BlueViolet", 0x8A2BE2}, {"Brown", 0xA52A2A},
    {"BurlyWood", 0xDEB887}, {"CadetBlue", 0x5F9EA0}, {"Chartreuse", 0x7FFF00},
    {"Chocolate", 0xD2691E}, {"Coral", 0xFF7F50}, {"CornflowerBlue", 0x6495ED},
    {"Cornsilk", 0xFFF8DC}, {"Crimson", 0xDC143C}, {"Cyan", 0x00FFFF},
    {"DarkBlue", 0x00008B}, {"DarkCyan", 0x008B8B}, {"DarkGoldenRod", 0xB8860B},
    {"DarkGray", 0xA9A9A9}, {"DarkGreen", 0x006400}, {"DarkKhaki", 0xBDB76B},
    {"DarkMagenta", 0x8B008B}, {"DarkOliveGreen", 0x556B2F}, {"DarkOrange", 0xFF8C00},
    {"DarkOrchid", 0x9932CC}, {"DarkRed", 0x8B0000}, {"DarkSalmon", 0xE9967A},
    {"DarkSeaGreen", 0x8FBC8F}, {"DarkSlateBlue", 0x483D8B}, {"DarkSlateGray", 0x2F4F4F},
    {"DarkTurquoise", 0x00CED1}, {"DarkViolet", 0x9400D3}, {"DeepPink", 0xFF1493},
    {"DeepSkyBlue", 0x00BFFF}, {"DimGray", 0x696969}, {"DodgerBlue", 0x1E90FF},
    {"FireBrick", 0xB22222}, {"FloralWhite", 0xFFFAF0}, {"ForestGreen", 0x228B22},
    {"Fuchsia", 0xFF00FF}, {"Gainsboro", 0xDCDCDC}, {"GhostWhite", 0xF8F8FF},
    {"Gold", 0xFFD700}, {"GoldenRod", 0xDAA520}, {"Gray", 0x808080},
    {"Green", 0x008000}, {"GreenYellow", 0xADFF2F}, {"HoneyDew", 0xF0FFF0},
    {"HotPink", 0xFF69B4}, {"IndianRed", 0xCD5C5C}, {"Indigo", 0x4B0082},
    {"Ivory", 0xFFFFF0}, {"Khaki", 0xF0E68C}, {"Lavender", 0xE6E6FA},
    {"LavenderBlush", 0xFFF0F5}, {"LawnGreen", 0x7CFC00}, {"LemonChiffon", 0xFFFACD},
    {"LightBlue", 0xADD8E6}, {"LightCoral", 0xF08080}, {"LightCyan", 0xE0FFFF},
    {"LightGoldenRodYellow", 0xFAFAD2}, {"LightGray", 0xD3D3D3}, {"LightGreen", 0x90EE90},
    {"LightPink", 0xFFB6C1}, {"LightSalmon", 0xFFA07A}, {"LightSeaGreen", 0x20B2AA},
    {"LightSkyBlue", 0x87CEFA}, {"LightSlateGray", 0x778899}, {"LightSteelBlue", 0xB0C4DE},
    {"LightYellow", 0xFFFFE0}, {"Lime", 0x00FF00}, {"LimeGreen", 0x32CD32},
    {"Linen", 0xFAF0E6}, {"Magenta", 0xFF00FF}, {"Maroon", 0x800000},
    {"MediumAquaMarine", 0x66CDAA}, {"MediumBlue", 0x0000CD}, {"MediumOrchid", 0xBA55D3},
    {"MediumPurple", 0x9370DB}, {"MediumSeaGreen", 0x3CB371}, {"MediumSlateBlue", 0x7B68EE},
    {"MediumSpringGreen", 0x00FA9A}, {"MediumTurquoise", 0x48D1CC}, {"MediumVioletRed", 0xC71585},
    {"MidnightBlue", 0x191970}, {"MintCream", 0xF5FFFA}, {"MistyRose", 0xFFE4E1},
    {"Moccasin", 0xFFE4B5}, {"NavajoWhite", 0xFFDEAD}, {"Navy", 0x000080},
    {"OldLace", 0xFDF5E6}, {"Olive", 0x808000}, {"OliveDrab", 0x6B8E23},
    {"Orange", 0xFFA500}, {"OrangeRed", 0xFF4500}, {"Orchid", 0xDA70D6},
    {"PaleGoldenRod", 0xEEE8AA}, {"PaleGreen", 0x98FB98}, {"PaleTurquoise", 0xAFEEEE},
    {"PaleVioletRed", 0xDB7093}, {"PapayaWhip", 0xFFEFD5}, {"PeachPuff", 0xFFDAB9},
    {"Peru", 0xCD853F}, {"Pink", 0xFFC0CB}, {"Plum", 0xDDA0DD},
    {"PowderBlue", 0xB0E0E6}, {"Purple", 0x800080}, {"RebeccaPurple", 0x663399},
    {"Red", 0xFF0000}, {"RosyBrown", 0xBC8F8F}, {"RoyalBlue", 0x4169E1},
    {"SaddleBrown", 0x8B4513}, {"Salmon", 0xFA8072}, {"SandyBrown", 0xF4A460},
    {"SeaGreen", 0x2E8B57}, {"SeaShell", 0xFFF5EE}, {"Sienna", 0xA0522D},
    {"Silver", 0xC0C0C0}, {"SkyBlue", 0x87CEEB}, {"SlateBlue", 0x6A5ACD},
    {"SlateGray", 0x708090}, {"Snow", 0xFFFAFA}, {"SpringGreen", 0x00FF7F},
    {"SteelBlue", 0x4682B4}, {"Tan", 0xD2B48C}, {"Teal", 0x008080},
    {"Thistle", 0xD8BFD8}, {"Tomato", 0xFF6347}, {"Turquoise", 0x40E0D0},
    {"Violet", 0xEE82EE}, {"Wheat", 0xF5DEB3}, {"White", 0xFFFFFF},
    {"WhiteSmoke", 0xF5F5F5}, {"Yellow", 0xFFFF00}, {"YellowGreen", 0x9ACD32}
}};


}

#endif // GEMPYRE_GRAPHICS_H
