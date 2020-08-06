#include "gempyre_graphics.h"
#include "gempyre_utils.h"

using namespace Gempyre;

CanvasData::~CanvasData() {
}

CanvasDataPtr CanvasElement::makeCanvas(int width, int height) { //could be const, but is it sustainable?
    gempyre_graphics_assert(width > 0 && height > 0, "Graphics size is expected be more than zero");
    m_tile = std::shared_ptr<CanvasData>(new CanvasData(/*std::min(width,*/ TileWidth/*)*/, /*td::min(height,*/ TileHeight/*)*/, m_id));
    return std::shared_ptr<CanvasData>(new CanvasData(width, height, m_id)); //private cannot use make_...
}

CanvasElement::~CanvasElement() {
    m_tile.reset();
}

void CanvasElement::paint(const CanvasDataPtr& canvas) {
    if(!canvas) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Won't paint as canvas is NULL");
        return;
    }
    if(!m_tile) {
        GempyreUtils::log(GempyreUtils::LogLevel::Error, "Won't paint as buffer is NULL");
        return;
    }
    if(canvas->height <= 0 || canvas->width <= 0 ) {
        GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Won't paint as canvas size is 0");
        return;
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sending canvas data");

    for(auto j = 0 ; j < canvas->height ; j += TileHeight) {
        const auto height = std::min(TileHeight, canvas->height - j);
        for(auto i = 0 ; i < canvas->width ; i += TileWidth) {
            const auto width = std::min(TileWidth, canvas->width - i);
            const auto srcPos = canvas->data() + i + (j * canvas->width);
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Copy canvas frame", i, j, width, height);
            for(int h = 0; h < height; h++) {
                const auto lineStart = srcPos + (h * canvas->width);
                auto trgPos = m_tile->data() + width * h;
                std::copy(lineStart, lineStart + width, trgPos);
            }
            GempyreUtils::log(GempyreUtils::LogLevel::Debug_Trace, "Sending canvas frame", i, j, width, height);
            m_tile->writeHeader({static_cast<CanvasData::Data::dataT>(i),
                                 static_cast<CanvasData::Data::dataT>(j),
                                 static_cast<CanvasData::Data::dataT>(width),
                                 static_cast<CanvasData::Data::dataT>(height)});
           send(m_tile);
        }
    }
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Sent canvas data");
}

std::string CanvasElement::addImage(const std::string& url, const std::function<void (const std::string& id)> &loaded) {
    const auto name = generateId("image");
    Gempyre::Element imageElement(*m_ui, name, "IMG", /*m_ui->root()*/*this);
    if(loaded)
        imageElement.subscribe("load", [loaded, name](const Gempyre::Event&) {
            loaded(name);
        });
    imageElement.setAttribute("style", "display:none");
    imageElement.setAttribute("src", url);
    return name;
}

std::vector<std::string> CanvasElement::addImages(const std::vector<std::string>& urls, const std::function<void (const std::vector<std::string>)>& loaded) {
    std::vector<std::string> names;
    auto result = std::make_shared<std::map<std::string, bool>>();
    std::for_each(urls.begin(), urls.end(), [this, &names, loaded, &result](const auto& url){
        const auto name = addImage(url, [loaded, result](const std::string& id) {
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

void CanvasElement::paintImage(const std::string& imageId, int x, int y, const Rect& clippingRect) {
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                    {"pos", std::vector<int>{x, y}}});
    else
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                         {"pos", std::vector<int>{x, y}},
                                                                         {"clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height}}});
}

void CanvasElement::paintImage(const std::string& imageId, const Rect& targetRect, const Element::Rect& clippingRect) {
    if(targetRect.width <= 0 || targetRect.height <= 0)
        return;
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                     {"rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height}}});
    else
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                         {"rect", std::vector<int>{targetRect.x, targetRect.y, targetRect.width, targetRect.height}},
                                                                         {"clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height}}});

}


void CanvasElement::draw(const CanvasElement::CommandList &canvasCommands) {
    if(canvasCommands.empty())
        return;
    std::vector<std::string> commandString;
    std::transform(canvasCommands.begin(), canvasCommands.end(), std::back_inserter(commandString), [](auto&& arg) -> std::string {
         if(const auto doubleval = std::get_if<double>(&arg))
            return std::to_string(*doubleval);
         if(const auto intval = std::get_if<int>(&arg))
            return std::to_string(*intval);
         return std::get<std::string>(arg);
    });
    send("canvas_draw", std::unordered_map<std::string, std::any>{{"commands", commandString}});
}

void CanvasElement::draw(const FrameComposer& frameComposer) {
    draw(frameComposer.composed());
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

Graphics::Graphics(const Gempyre::CanvasElement& element, int width, int height) : m_element(element), m_canvas(m_element.makeCanvas(width, height)) {
    GempyreUtils::log(GempyreUtils::LogLevel::Debug, "Graphics consructed", width, height);
}
/**
 * @function Graphics
 * @param element
 *
 * Creates a Graphics without a Canvas, call `create` to construct an actual Canvas.
 */
Graphics::Graphics(const Gempyre::CanvasElement& element) : m_element(element) {
    GempyreUtils::log(GempyreUtils::LogLevel::Info, "Graphics without canvas created, create() must be called");
}

void Graphics::drawRect(const Element::Rect& rect, Color::type color) {
    if(rect.width <= 0 || rect.width <= 0)
        return;
    const auto x = std::max(0, rect.x);
    const auto y = std::max(0, rect.y);
    const auto width = (x + rect.width >= m_canvas->width) ?  m_canvas->width - rect.x : rect.width;
    const auto height = (y + rect.height >= m_canvas->height) ? m_canvas->height - rect.y : rect.height;
    auto pos = m_canvas->data() + (x + y * m_canvas->width);
    for(int j = 0; j < height; j++) {
        std::fill(pos, pos + width, color);
        pos += m_canvas->width;
    }
}

void Graphics::merge(const Graphics& other) {
    if(other.m_canvas == m_canvas)
        return;
    gempyre_graphics_assert(other.m_canvas->size() == m_canvas->size(), "Canvas sizes must match")
    auto pos = m_canvas->data();
    const auto posOther = other.m_canvas->data();
    for(auto i = 0U; i < m_canvas->size(); i++) {
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

       const auto pix = Color::rgbaClamped((r + ro) / 0xFF , (g + go) / 0xFF, (b + bo) / 0xFF, a);
       pos[i] = pix;
    }
}


Graphics Graphics::clone() const {
        Graphics other(m_element);
        other.create(m_canvas->width, m_canvas->height);
        std::copy(m_canvas->begin(), m_canvas->end(), other.m_canvas->data());
        return other;
    }

void Graphics::update() {
    if(m_canvas)
        m_element.paint(m_canvas);
}

