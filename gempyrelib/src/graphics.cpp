#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "data.h"
#include <any>
#include <cassert>


using namespace Gempyre;


static constexpr auto TileWidth = 640;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes (There is a issue somewhere, this not really work if something else)
static constexpr auto TileHeight = 640; // as there are some header info

class Gempyre::CanvasData  {
private:
    enum DataTypes : dataT {
      CanvasId = 0xAAA
    };
public:
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
 #ifdef GEMPYRE_IS_DEBUG
    std::string dump() const {return m_data->dump();}
#endif
private:
    std::shared_ptr<Data> m_data;
    const int m_width;
    const int m_height;

private:
   
    dataT* data() {return m_data->data();}
    const dataT* data() const {return m_data->data();}
    Data& ref() { return *m_data; }
    DataPtr ptr() const {return m_data;}    
private:
    CanvasData(int w, int h, const std::string& owner);
    friend class CanvasElement;
    friend class Bitmap;
};



CanvasDataPtr CanvasElement::make_canvas(int width, int height) { //could be const, but is it sustainable?
    gempyre_graphics_assert(width > 0 && height > 0, "Graphics size is expected be more than zero");
    m_tile = std::shared_ptr<CanvasData>(new CanvasData(/*std::min(width,*/ TileWidth/*)*/, /*td::min(height,*/ TileHeight/*)*/, m_id));
    return std::shared_ptr<CanvasData>(new CanvasData(width, height, m_id)); //private cannot use make_...
}

CanvasElement::~CanvasElement() {
    m_tile.reset();
}



void CanvasElement::paint(const CanvasDataPtr& canvas, bool as_draw) {
    if(!canvas) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Won't paint as canvas is NULL");
        return;
    }
    if(!m_tile) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Won't paint as buffer is NULL");
        return;
    }
    if(canvas->height() <= 0 || canvas->width() <= 0 ) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Won't paint as canvas size is 0");
        return;
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sending canvas data");

    bool is_last = false;

    for(auto j = 0 ; j < canvas->height() ; j += TileHeight) {
        const auto height = std::min(TileHeight, canvas->height() - j);
        for(auto i = 0 ; i < canvas->width() ; i += TileWidth) {
            assert(!is_last);
            is_last = (canvas->height() - j < TileHeight) && (canvas->width() - i < TileWidth);
            const auto width = std::min(TileWidth, canvas->width() - i);
            const auto srcPos = canvas->data() + i + (j * canvas->width());
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Copy canvas frame", i, j, width, height);
            for(int h = 0; h < height; h++) {
                const auto lineStart = srcPos + (h * canvas->width());
                auto trgPos = m_tile->data() + width * h;
                assert(trgPos < m_tile->data() + m_tile->width() * m_tile->height());
                std::copy(lineStart, lineStart + width, trgPos);
            }
            m_tile->ref().writeHeader({static_cast<Gempyre::dataT>(i),
                                 static_cast<Gempyre::dataT>(j),
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
            send(m_tile->ptr());
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
        });
    imageElement.set_attribute("style", "display:none");
    imageElement.set_attribute("src", url);
    return name;
}

std::vector<std::string> CanvasElement::add_images(const std::vector<std::string>& urls, const std::function<void (const std::vector<std::string>)>& loaded) {
    std::vector<std::string> names;
    auto result = std::make_shared<std::map<std::string, bool>>();
    std::for_each(urls.begin(), urls.end(), [this, &names, loaded, &result](const auto& url){
        const auto name = add_image(url, [loaded, result](const std::string& id) {
            (*result)[id] = true;
            if(loaded && std::find_if(result->begin(), result->end(), [](const auto& r){return !r.second;}) == result->end()) {
                std::vector<std::string> keys;
                std::transform(result->begin(), result->end(), std::back_inserter(keys), [](const auto& it){return it.first;});
                loaded(keys);
            }
        });
        result->emplace(name, false);
        names.push_back(name);
    });
    return names;
}

void CanvasElement::paint_image(const std::string& imageId, int x, int y, const Rect& clippingRect) const {
    auto This = const_cast<CanvasElement*>(this);
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        This->send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                    {"pos", std::vector<int>{x, y}}});
    else
        This->send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                         {"pos", std::vector<int>{x, y}},
                                                                         {"clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height}}});
}

void CanvasElement::paint_image(const std::string& imageId, const Rect& targetRect, const Element::Rect& clippingRect) const {
    if(targetRect.width <= 0 || targetRect.height <= 0)
        return;
    auto This = const_cast<CanvasElement*>(this);
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        This->send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                     {"rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height}}});
    else
        This->send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                         {"rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height}},
                                                                         {"clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height}}});

}


void CanvasElement::draw(const CanvasElement::CommandList &canvasCommands) const {
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
    auto This = const_cast<CanvasElement*>(this);
    This->send("canvas_draw", std::unordered_map<std::string, std::any>{
                   {"commands", commandString}}, true);
}

void CanvasElement::draw(const FrameComposer& frameComposer) const {
    draw(frameComposer.composed());
}


void CanvasElement::draw_completed(DrawCallback&& drawCompletedCallback, DrawNotify kick) {
    subscribe("event_notify", [this](const Event& ev) {
        if(m_drawCallback && ev.properties.at("name") == "canvas_draw") {
            m_drawCallback();
        }
    });
    m_drawCallback = std::move(drawCompletedCallback);
    send("event_notify", std::unordered_map<std::string, std::any>{
                   {"name", "canvas_draw"},
                   {"add", m_drawCallback != nullptr}
               });
    if(kick == DrawNotify::Kick) {
        ui().after(0ms, m_drawCallback);
    }

}

void CanvasElement::erase(bool resized) const {
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

Bitmap::Bitmap(const Gempyre::CanvasElement& element, int width, int height) : m_element(element), m_canvas(m_element.make_canvas(width, height)) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Graphics consructed", width, height);
}
/**
 * @function Graphics
 * @param element
 *
 * Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
 */
Bitmap::Bitmap(const Gempyre::CanvasElement& element) : m_element(element) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Bitmap without canvas created, create() must be called");
}


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

void Bitmap::merge(const Bitmap& other) {
    if(other.m_canvas == m_canvas)
        return;
    gempyre_graphics_assert(other.m_canvas->ref().size() == m_canvas->ref().size(), "Canvas sizes must match")
    auto pos = m_canvas->data();
    const auto posOther = other.m_canvas->data();
    for(auto i = 0U; i < m_canvas->ref().elements(); i++) {
       const auto p = pos[i];
       const auto po = posOther[i];
       const auto ao = Color::alpha(po);
       const auto a = Color::alpha(p);
       const auto r = Color::r(p) * (0xFF - ao);
       const auto g = Color::g(p) * (0xFF - ao);
       const auto b = Color::b(p) * (0xFF - ao);


       const auto ro = (Color::r(po) * ao);
       const auto go = (Color::g(po) * ao);
       const auto bo = (Color::b(po) * ao);

       const auto pix = Color::rgba_clamped((r + ro) / 0xFF , (g + go) / 0xFF, (b + bo) / 0xFF, a);
       pos[i] = pix;
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
        Bitmap other(m_element);
        other.create(m_canvas->width(), m_canvas->height());
        std::copy(m_canvas->ptr()->begin(), m_canvas->ptr()->end(), other.m_canvas->data());
        return other;
    }

void Bitmap::update() {
    if(m_canvas)
        m_element.paint(m_canvas, true);
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

