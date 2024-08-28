#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "data.h"
#include "canvas_data.h"
#include "gempyre_internal.h"
#include "gempyre_bitmap.h"
#include <any>
#include <cassert>


using namespace Gempyre;

static constexpr auto TileWidth = 640;  // used for server spesific stuff - bigger than a limit (16384) causes random crashes (There is a issue somewhere, this not really work if something else)
static constexpr auto TileHeight = 640; // as there are some header info


 CanvasElement::CanvasElement(const CanvasElement& other)
        : Element{other},
          m_tile{other.m_tile},
          m_width{other.m_width},
          m_height{other.m_height}{
    }

CanvasElement::CanvasElement(CanvasElement&& other)
        : Element{std::move(other)},
            m_tile{std::move(other.m_tile)},
            m_width{other.m_width},
            m_height{other.m_height}{
    }

CanvasElement::CanvasElement(Ui& ui, std::string_view id)
        : Element(ui, id) {}
    
CanvasElement::CanvasElement(Ui& ui, std::string_view id, const Element& parent)
        : Element(ui, id, "canvas", parent) {}

CanvasElement::CanvasElement(Ui& ui, const Element& parent)
        : Element(ui, "canvas", parent) {}
        

// Copy operator. 
CanvasElement& CanvasElement::operator=(const CanvasElement& other) {
    m_tile = other.m_tile;
    m_width = other.m_width;
    m_height = other.m_height;
    return *this;
}

/// Move operator.
CanvasElement& CanvasElement::operator=(CanvasElement&& other) {
    m_tile = std::move(other.m_tile);
    m_width = other.m_width;
    m_height = other.m_height;
    return *this;
}

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
                                    static_cast<Gempyre::dataT>(as_draw && is_last)});
                
                GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sending canvas frame", i, j, width, height, m_tile->size(),  (width * height + 4) * 4 + 20 + 16);
                #ifdef GEMPYRE_IS_DEBUG
                if (m_tile->size() != static_cast<size_t>((width * height + 4) * 4 + 20 + 16)) {
                    GempyreUtils::log(GempyreUtils::LogLevel::Debug, m_tile->dump());                
                }
            //  assert(m_tile->size() == static_cast<size_t>((width * height + 4) * 4 + 20 + 16));
                #endif         
                ref().send(m_tile->ptr(), is_last); // last is not droppable
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
            ref().send(m_tile->ptr(), is_last);   // last is not droppable                         
        }
    }
    assert(is_last);
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sent canvas data");
}

std::string CanvasElement::add_image(std::string_view url, const std::function<void (const std::string& id)> &loaded) {
    const auto name = generateId("image");
    Gempyre::Element imageElement(*m_ui, name, "IMG", /*m_ui->root()*/*this);
    if(loaded)
        imageElement.subscribe(Gempyre::Event::LOAD, [loaded, name](const Gempyre::Event&) {
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

void CanvasElement::paint_image(std::string_view imageId, int x, int y, const Rect& clippingRect) const {
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

void CanvasElement::paint_image(std::string_view imageId, const Rect& targetRect, const Element::Rect& clippingRect) const {
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


// TODO: This function has issues
// 1) it HAS to be called if there is any drawing +10 fps, otherwise network may be mumbled
// i.e. why then not to set it always on (but being lazy init is trikier and that is why not done now)
// 2) If called multiple times there can be some unwanted size effecs
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
    FrameComposer fc;
    fc.begin_path();
    if(resized)
        fc.clear_rect(0, 0, m_width, m_height); 
    else
        fc.clear_rect(0, 0, 0x4000, 0x4000);
    fc.close_path();
    draw(fc);
}

void CanvasElement::draw(int x, int y, const Gempyre::Bitmap& bmp) {
     if(bmp.m_canvas)
        paint(bmp.m_canvas, x, y, true);
}
