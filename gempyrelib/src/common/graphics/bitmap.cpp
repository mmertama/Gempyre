#include "gempyre_bitmap.h"
#include "gempyre_utils.h"
#include "data.h"
#include "canvas_data.h"
#include <string>
#include <lodepng.h>
#include <cassert>


using namespace Gempyre;

// may need C++20 to be constexpr (TODO)
std::optional<Color::type> Color::from_html_name(std::string_view name) {
        const auto it = std::find_if(std::begin(html_colors), std::end(html_colors), [&](const auto& p) {
            return GempyreUtils::iequals(p.first, name);
            });
        return it != std::end(html_colors) ? std::make_optional(rgba(
            (it->second >> 16) & 0xFF,
            (it->second >> 8) & 0xFF,
            (it->second) & 0xFF, 
            0xFF
        )) : std::nullopt;
    }

 std::optional<Color::type> Color::get_color(std::string_view color) {
        if (color.empty())
            return std::nullopt;    
        if (color[0] == '#') {
            if (color.length() == 7) {// #RRGGBB
                const auto v = GempyreUtils::parse<uint32_t>(color.substr(1), 16);
                return v ? std::make_optional(rgb_value(*v)) : std::nullopt;
            }    
            if (color.length() == 9) {// #RRGGBBAA
                const auto v = GempyreUtils::parse<uint32_t>(color.substr(1), 16);
                return v ? std::make_optional(rgba_value(*v)) : std::nullopt;
            }    
        }
        if (color[0] == '0') {
             if (color.length() == 8) {// 0xRRGGBB
                const auto v = GempyreUtils::parse<uint32_t>(color.substr(2), 16);
                return v ? std::make_optional(rgb_value(*v)) : std::nullopt;
            }    
            if (color.length() == 10) {// 0xRRGGBBAA
                const auto v = GempyreUtils::parse<uint32_t>(color.substr(2), 16);
                return v ? std::make_optional(rgba_value(*v)) : std::nullopt;
            }    
        } else {   
            const auto named = from_html_name(color);
            if (named)
                return named;
        }
        return std::nullopt;
    }

Bitmap::Bitmap(int width, int height)  {
    if(width > 0 && height > 0)
        create(width, height);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Graphics constructed", width, height);
}


Bitmap::Bitmap(int width, int height, Gempyre::Color::type color) : Bitmap(width, height) {
    if(width > 0 && height > 0)
        draw_rect({0, 0, width, height}, color);
}

Bitmap::Bitmap(const std::vector<unsigned char>& image_data)  {
    unsigned width, height;
    std::vector<unsigned char> image;
    const auto error = lodepng::decode(image, width, height, image_data, LCT_RGBA, 8);

    if(error) {
        throw std::runtime_error(lodepng_error_text(error)); // or use return value as exceptions not used? TODO: Think
        }

    create(static_cast<int>(width), static_cast<int>(height));
    assert(m_canvas);
    auto ptr = reinterpret_cast<unsigned char*>(m_canvas->data());
    std::copy(image.begin(), image.end(), ptr);    
}

std::vector<uint8_t> Bitmap::png_image() const {
    std::vector<uint8_t> out;
    lodepng::State state;
    lodepng::encode(out, reinterpret_cast<const unsigned char*>(m_canvas->data()),
        static_cast<unsigned>(width()),
        static_cast<unsigned>(height()), state);
    return out;
}

void Bitmap::create(int width, int height) {
        assert(width > 0);
        assert(height > 0);
        m_canvas = std::shared_ptr<CanvasData>(new CanvasData(width, height)); 
    }

/**
 * @function Graphics
 * @param element
 *
 * Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
 */
Bitmap::Bitmap() {}


Bitmap::~Bitmap() {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Bitmap gone");
}

void Bitmap::draw_rect(const Gempyre::Rect& rect, Color::type color) {
    if(empty())
        return;
    const auto x = std::max(0, rect.x);
    const auto y = std::max(0, rect.y);
    const auto width = (x + rect.width >= m_canvas->width()) ?  m_canvas->width ()- rect.x : rect.width;
    const auto height = (y + rect.height >= m_canvas->height()) ? m_canvas->height() - rect.y : rect.height;
    auto pos = m_canvas->data() + (x + y * m_canvas->width());
    for(int j = 0; j < height; j++) {
        std::fill(pos, pos + width, color);
        pos += m_canvas->width();
    }
}

void Bitmap::tile(int x_pos, int y_pos, const Bitmap& bitmap, int r_width, int r_height) {
    tile(x_pos, y_pos, bitmap, 0, 0, r_width, r_height);
}

void Bitmap::tile(int x_pos, int y_pos, const Bitmap& bitmap) {
    tile(x_pos, y_pos, bitmap, bitmap.width(), bitmap.height());
}

void Bitmap::tile(int x_pos, int y_pos, const Bitmap& bitmap, int rx_pos, int ry_pos, int r_width, int r_height) {
    if(bitmap.m_canvas == m_canvas)
        return;

    if(empty() || bitmap.empty())
        return;    

    auto width = std::min(r_width, bitmap.width()) - rx_pos;
    auto height = std::min(r_height, bitmap.height()) - ry_pos;

    rx_pos = std::max(0, rx_pos);
    ry_pos = std::max(0, ry_pos);
        
    if (width <= 0 || x_pos >= this->width() || x_pos + width < 0)
        return;

    if (height <= 0 || y_pos >= this->height() || y_pos + height < 0)
        return;
        
    int x, y, b_x, b_y;

    if (x_pos < 0) {              // if -10 and width 100
        x = 0;                  // set 0  
        b_x = -x_pos + rx_pos;// (-10) => 10
        width -= b_x;           // 100 + (-10) => 90 
    } else {
        x = x_pos;
        b_x = rx_pos;
    }

    if (y_pos < 0) {
        y = 0;
        b_y = -y_pos + ry_pos;
        height -= b_y;
    } else {
        y = y_pos;
        b_y = ry_pos;
    }

    if  (x + width >= this->width()) {
        width = this->width() - x;
    }

    if  (y + height >= this->height()) {
        height = this->height() - y;
    }

    assert(width <= this->width());
    assert(height <= this->height());
    assert(width <= bitmap.width());
    assert(height <= bitmap.height());
   

    for (auto j = 0; j < height; ++j) {
        const auto target = m_canvas->data() + (x + (y + j) * m_canvas->width());
        const auto source = bitmap.m_canvas->data() + (b_x + (b_y + j) * bitmap.m_canvas->width());
        std::memcpy(target, source, sizeof(dataT) * static_cast<size_t>(width));
        }
}

void Bitmap::merge( int x_pos, int y_pos, const Bitmap& bitmap) {
    if(bitmap.m_canvas == m_canvas)
        return;

    if(empty() || bitmap.empty())
        return;    

    auto width = bitmap.width();
    auto height = bitmap.height();
        
    if (x_pos >= this->width() || x_pos + bitmap.width() < 0)
        return;

    if (y_pos >= this->height() || y_pos + bitmap.height() < 0)
            return;
        
    int x, y, b_x, b_y;

    if (x_pos < 0) {              // if -10 and width 100
        x = 0;                  // set 0  
        b_x = (-x_pos);// (-10) => 10
        width -= b_x;           // 100 + (-10) => 90 
    } else {
        x = x_pos;
        b_x = 0;
    }

    if (y_pos < 0) {
        y = 0;
        b_y = (-y_pos);
        height -= b_y;
    } else {
        y = y_pos;
        b_y = 0;
    }

    if  (x + width >= this->width()) {
        width = this->width() - x;
    }

    if  (y + height >= this->height()) {
        height = this->height() - y;
    }

    assert(width <= this->width());
    assert(height <= this->height());
    assert(width <= bitmap.width());
    assert(height <= bitmap.height());
   

    for (auto j = 0; j < height; ++j) {
        for (auto i = 0; i < width; ++i) {
            assert(x + i < this->width());
            assert(y + j < this->height());
            const auto p = m_canvas->get(x + i, y + j);

            const auto po = bitmap.m_canvas->get(b_x + i, b_y + j);
            const auto ao = Color::alpha(po);
            const auto a = Color::alpha(p);
            
            const auto r = Color::r(p) * (0xFF - ao);
            const auto g = Color::g(p) * (0xFF - ao);
            const auto b = Color::b(p) * (0xFF - ao);

            const auto ro = (Color::r(po) * ao);
            const auto go = (Color::g(po) * ao);
            const auto bo = (Color::b(po) * ao);

            const auto pix = Color::rgba_clamped((r + ro) / 0xFF , (g + go) / 0xFF, (b + bo) / 0xFF, a + ao);
            m_canvas->put(x + i, y + j, pix);
        }
    }
}


void Bitmap::set_pixel(int x, int y, Color::type color) {
    m_canvas->put(x, y, color);
    }

void Bitmap::set_alpha(int x, int y, Color::type alpha) {
    const auto c = m_canvas->get(x, y);
    m_canvas->put(x, y, pix(Color::r(c), Color::g(c), Color::b(c), alpha));
    }

  Color::type Bitmap::pixel(int x, int y) const {
    return m_canvas->get(x, y);
  }   


// add this test in bitmap was long thinking - yes it may make it slower
// but makes many things easier elsewhere (no need to call empty)
// trust your compiler :-D
int Bitmap::width() const {
        return m_canvas ? m_canvas->width() : 0;
    }

int Bitmap::height() const {
        return m_canvas ? m_canvas->height() : 0; 
    }

void Bitmap::swap(Bitmap& other) {
        m_canvas.swap(other.m_canvas);
    }


Bitmap Bitmap::clone() const {
        Bitmap other;
        if(!empty()) {
            other.create(m_canvas->width(), m_canvas->height());
            other.copy_from(*this);
        }
        return other;
    }


void Bitmap::copy_from(const Bitmap& other) {
    assert(other.width() == width());
    assert(other.height() == height());
    if(m_canvas == other.m_canvas)
        return;
    std::copy(other.m_canvas->ptr()->begin(), other.m_canvas->ptr()->end(), m_canvas->data());
}

Bitmap Bitmap::clip(const Gempyre::Rect& rect) const {
    const auto x = std::max(0, rect.x);
    const auto y = std::max(0, rect.y);
    const auto w = std::min(rect.width - (x - rect.x), width() - x);
    const auto h = std::min(rect.height - (y - rect.y), height() - y);
    Bitmap bmp(w, h);
    for (auto row = 0; row < h; ++row) {
        for(auto col = 0; col < w; ++col) {
            bmp.set_pixel(col, row, pixel(col + x, row + y)); // this could be a scanline copy, but really? show me the benefit
        }
    }
    return bmp;
}

bool Bitmap::empty() const {
    return !m_canvas || m_canvas->width() <= 0 || m_canvas->height() <= 0;
}

const uint8_t* Bitmap::const_data() const {
    return reinterpret_cast <const uint8_t*>(m_canvas->data());
}

Color::type* Bitmap::inner_data() {
    return m_canvas->data();
}

std::size_t Bitmap::size() const {
    return m_canvas->size();
}