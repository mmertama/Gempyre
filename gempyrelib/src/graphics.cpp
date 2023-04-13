#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "gempyre_internal.h"
#include "data.h"
#include <any>
#include <cassert>
#include <lodepng.h>

using namespace Gempyre;

static constexpr auto TileWidth = 640;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes (There is a issue somewhere, this not really work if something else)
static constexpr auto TileHeight = 640; // as there are some header info

class Gempyre::CanvasData  {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
    static constexpr auto NO_ID = "";
    CanvasData(int w, int h, const std::string& owner);
    CanvasData(int w, int h) : CanvasData(w, h, NO_ID) {}
    ~CanvasData() = default;
    void put(int x, int y, dataT pixel) {
        data()[x + y * m_width] = pixel;
    }
    [[nodiscard]] dataT get(int x, int y) const {
        return data()[x + y * m_width];
    }
    int width() const {return m_width;}
    int height() const {return m_height;}
    size_t size() const {return m_data->size();}


    dataT* data() {return m_data->data();}
    const dataT* data() const {return m_data->data();}
    const Data& ref() const { return *m_data; }
    Data& ref() { return *m_data; }
    DataPtr ptr() const {return m_data;}   

 #ifdef GEMPYRE_IS_DEBUG
    std::string dump() const {return m_data->dump();}
#endif
private:
    std::shared_ptr<Data> m_data;
    const int m_width;
    const int m_height;  
};


CanvasElement::~CanvasElement() {
    m_tile.reset();
}



void CanvasElement::paint(const CanvasDataPtr& canvas, int x_pos, int y_pos, bool as_draw) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "paint", x_pos, y_pos, as_draw);
    if(!canvas) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Won't paint as canvas is NULL");
        return;
    }
    if(!m_tile) {
        m_tile = std::shared_ptr<CanvasData>(new CanvasData(TileWidth, TileHeight, m_id));
    }

    if(canvas->height() <= 0 || canvas->width() <= 0 ) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Won't paint as canvas size is 0");
        return;
    }

    // This is not ok - as we dont know all extents
    // if (y_pos + canvas->height() < 0 || x_pos + canvas->width() < 0) {
    //    return; 
    //}
        
    //const auto canvas_height = y_pos < 0 ? canvas->height() + y_pos : canvas->height();
    const auto canvas_height =  canvas->height();

    const auto y = y_pos < 0 ? -y_pos : 0;
    //const auto canvas_width = x_pos < 0 ? canvas->width() + x_pos : canvas->width();
    const auto canvas_width =  canvas->width();
    const auto x = x_pos < 0 ? -x_pos : 0;

    x_pos = std::max(0, x_pos);
    y_pos = std::max(0, y_pos);

    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sending canvas data");

    bool is_last = false;

    if(y < canvas_height && x < canvas_width) {

        for(auto j = y ; j < canvas_height ; j += TileHeight) {
            const auto height = std::min(TileHeight, canvas_height - j);
            for(auto i = x ; i < canvas_width ; i += TileWidth) {
                assert(!is_last);
                is_last = (canvas_height - j <= TileHeight) && (canvas_width - i <= TileWidth);
                const auto width = std::min(TileWidth, canvas_width - i);
                const auto srcPos = canvas->data() + i + (j * canvas->width());
                GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Copy canvas frame", i, j, width, height);
                for(int h = 0; h < height; h++) {
                    const auto lineStart = srcPos + (h * canvas->width());
                    auto trgPos = m_tile->data() + width * h;
                    assert(trgPos < m_tile->data() + m_tile->width() * m_tile->height());
                    std::copy(lineStart, lineStart + width, trgPos);
                }
                m_tile->ref().writeHeader({static_cast<Gempyre::dataT>(i + x_pos),
                                    static_cast<Gempyre::dataT>(j + y_pos),
                                    static_cast<Gempyre::dataT>(width),
                                    static_cast<Gempyre::dataT>(height),
                                    (as_draw && is_last)});
                
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sending canvas frame", i, j, width, height, m_tile->size(),  (width * height + 4) * 4 + 20 + 16);
                #ifdef GEMPYRE_IS_DEBUG
                if (m_tile->size() != static_cast<size_t>((width * height + 4) * 4 + 20 + 16)) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, m_tile->dump());                
                }
            //  assert(m_tile->size() == static_cast<size_t>((width * height + 4) * 4 + 20 + 16));
                #endif         
                ref().send(m_tile->ptr());
            }
        }
    } else {
        is_last = true;
        if (as_draw) {
            m_tile->ref().writeHeader({static_cast<Gempyre::dataT>(x_pos),
                                        static_cast<Gempyre::dataT>(y_pos),
                                        static_cast<Gempyre::dataT>(0),
                                        static_cast<Gempyre::dataT>(0),
                                        static_cast<Gempyre::dataT>(is_last)});
            ref().send(m_tile->ptr());                            
        }
    }
    assert(is_last);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sent canvas data");
}

std::string CanvasElement::add_image(const std::string& url, const std::function<void (const std::string& id)> &loaded) {
    const auto name = generateId("image");
    Gempyre::Element imageElement(*m_ui, name, "IMG", /*m_ui->root()*/*this);
    if(loaded)
        imageElement.subscribe("load", [loaded, name](const Gempyre::Event&) {
            loaded(name);
        }, {"complete"});
    imageElement.set_attribute("style", "display:none");
    imageElement.set_attribute("src", url);
    return name;
}

#if 0
std::vector<std::string> CanvasElement::add_images(const std::vector<std::string>& urls, const std::function<void (const std::vector<std::string>)>& loaded) {
    std::vector<std::string> names;
    auto result = std::make_shared<std::map<std::string, bool>>();
    std::for_each(urls.begin(), urls.end(), [this, &names, loaded, &result](const auto& url){
        const auto name = add_image(url, [this, loaded, result](const std::string& id) {
            (*result)[id] = true;
            if(loaded && std::find_if(result->begin(), result->end(), [](const auto& r){return !r.second;}) == result->end()) {
                std::vector<std::string> keys;
                std::transform(result->begin(), result->end(), std::back_inserter(keys), [](const auto& it){return it.first;});
                ui().after(0ms, [loaded, keys = std::move(keys)] {
                    loaded(keys);
                 });
            }
        });
        result->emplace(name, false);
        names.push_back(name);
    });
    return names;
}
#endif

void CanvasElement::paint_image(const std::string& imageId, int x, int y, const Rect& clippingRect) const {
    auto ui = const_cast<GempyreInternal*>(&ref());
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        ui->send(*this, "paint_image",
            "image", imageId,
            "pos", std::vector<int>{x, y});
    else
        ui->send(*this, "paint_image",
            "image", imageId,
            "pos", std::vector<int>{x, y},
            "clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height});
}

void CanvasElement::paint_image(const std::string& imageId, const Rect& targetRect, const Element::Rect& clippingRect) const {
    if(targetRect.width <= 0 || targetRect.height <= 0)
        return;
    auto ui = const_cast<GempyreInternal*>(&ref());
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        ui->send(*this, "paint_image",
        "image", imageId,
        "rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height});
    else
        ui->send(*this, "paint_image", 
        "image", imageId, 
        "rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height},
        "clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height});

}


void CanvasElement::draw(const CanvasElement::CommandList &canvasCommands)  {
    if(canvasCommands.empty())
        return;
    std::vector<std::string> commandString;
    /*std::transform(canvasCommands.begin(), canvasCommands.end(), std::back_inserter(commandString), [](auto&& arg) -> std::string {
         if(const auto doubleval = std::get_if<double>(&arg))
            return std::to_string(*doubleval);
         if(const auto intval = std::get_if<int>(&arg))
            return std::to_string(*intval);
         return std::get<std::string>(arg);
    });*/
    const auto str = [](auto&& arg) -> std::string {
            if(const auto doubleval = std::get_if<double>(&arg))
               return std::to_string(*doubleval);
            if(const auto intval = std::get_if<int>(&arg))
               return std::to_string(*intval);
            return std::string(std::get<std::string>(arg));
       };
    for(auto&& cmd : canvasCommands)  {
        auto s = str(cmd);
        commandString.emplace_back(s);
    }

    ref().send_unique(*this, "canvas_draw", "commands", commandString);
}

void CanvasElement::draw(const FrameComposer& frameComposer) {
    draw(frameComposer.composed());
}


void CanvasElement::draw_completed(const DrawCallback& drawCallback, DrawNotify kick) {
    
    ref().send(*this, "event_notify",
        "name", "canvas_draw",
        "add", drawCallback != nullptr);

    if (drawCallback) {

        if(kick == DrawNotify::Kick) {
            ui().after(0ms, drawCallback);
        }

        subscribe("event_notify", [drawCallback](const Event& ev) {
            if(ev.properties.at("name") == "canvas_draw") {
                drawCallback();
            }
        });  
    }         

}

void CanvasElement::erase(bool resized) {
    if(resized || m_width <= 0 || m_height <= 0) {
        const auto rv = rect();
        if(rv) {
            m_width = rv->width;
            m_height = rv->height;
        } else {
            return;
        }
    }
    draw({"clearRect", 0, 0, m_width, m_height});
}

void CanvasElement::draw(int x, int y, const Gempyre::Bitmap& bmp) {
     if(bmp.m_canvas)
        paint(bmp.m_canvas, x, y, true);
}

Bitmap::Bitmap(int width, int height)  {
    create(width, height);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Graphics consructed", width, height);
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

void Bitmap::draw_rect(const Element::Rect& rect, Color::type color) {
    if(rect.width <= 0 || rect.width <= 0)
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

void Bitmap::merge( int x_pos, int y_pos, const Bitmap& bitmap) {
    if(bitmap.m_canvas == m_canvas)
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
    }

    if (y_pos < 0) {
        y = 0;
        b_y = (-y_pos);
        height -= b_y;
    } else {
        y = y_pos;
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
            const auto po = bitmap.m_canvas->get(i, j);
            const auto ao = Color::alpha(po);
            const auto a = Color::alpha(p);
            const auto r = Color::r(p) * (0xFF - ao);
            const auto g = Color::g(p) * (0xFF - ao);
            const auto b = Color::b(p) * (0xFF - ao);

            const auto ro = (Color::r(po) * ao);
            const auto go = (Color::g(po) * ao);
            const auto bo = (Color::b(po) * ao);

            const auto pix = Color::rgba_clamped((r + ro) / 0xFF , (g + go) / 0xFF, (b + bo) / 0xFF, a);
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

int Bitmap::width() const {
        return m_canvas->width();
    }

int Bitmap::height() const {
        return m_canvas->height();
    }

void Bitmap::swap(Bitmap& other) {
        m_canvas.swap(other.m_canvas);
    }


Bitmap Bitmap::clone() const {
        Bitmap other;
        other.create(m_canvas->width(), m_canvas->height());
        other.copy_from(*this);
        return other;
    }


void Bitmap::copy_from(const Bitmap& other) {
    assert(other.width() == width());
    assert(other.height() == height());
    if(m_canvas == other.m_canvas)
        return;
    std::copy(other.m_canvas->ptr()->begin(), other.m_canvas->ptr()->end(), m_canvas->data());
}


 CanvasData::CanvasData(int w, int h, const std::string& owner) :
    m_data{std::make_shared<Data>(
        static_cast<size_t>(w * h),
        static_cast<dataT>(CanvasId),
        owner,
        std::vector<dataT>{0, 0, static_cast<dataT>(w), static_cast<dataT>(h),
        false // no update cb
        }
        )},
    m_width{w},
    m_height{h} {}

