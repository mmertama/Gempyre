#include "telex_graphics.h"
#include "telex_utils.h"

using namespace Telex;

Canvas::~Canvas() {
}

CanvasPtr CanvasElement::makeCanvas(int width, int height) { //could be const, but is it sustainable?
    m_tile = std::shared_ptr<Canvas>(new Canvas(std::min(width, TileWidth), std::min(height, TileHeight), m_id));
    return std::shared_ptr<Canvas>(new Canvas(width, height, m_id)); //private cannot use make_...
}

CanvasElement::~CanvasElement() {
    m_tile.reset();
}

void CanvasElement::paint(const CanvasPtr& canvas) {
    for(auto j = 0 ; j < canvas->height ; j += TileHeight) {
        const auto height = std::min(TileHeight, canvas->height - j);
        for(auto i = 0 ; i < canvas->width ; i += TileWidth) {
            const auto width = std::min(TileWidth, canvas->width - i);
            const auto srcPos = canvas->data() + i + (j * canvas->width);
            for(int h = 0; h < height; h++) {
                const auto lineStart = srcPos + (h * canvas->width);
                auto trgPos = m_tile->data() + width * h;
                std::copy(lineStart, lineStart + width, trgPos);
            }
            m_tile->writeHeader({static_cast<Canvas::Data::dataT>(i),
                                 static_cast<Canvas::Data::dataT>(j),
                                 static_cast<Canvas::Data::dataT>(width),
                                 static_cast<Canvas::Data::dataT>(height)});
           send(m_tile);
        }
    }
}

std::string CanvasElement::addImage(const std::string& url, const std::function<void (const std::string& id)> &loaded) {
    const auto name = generateId("image");
    Telex::Element imageElement(*m_ui, name, "IMG", /*m_ui->root()*/*this);
    if(loaded)
        imageElement.subscribe("load", [loaded, name](const Telex::Event&) {
            loaded(name);
        });
    imageElement.setAttribute("style", "display:none");
    imageElement.setAttribute("src", url);
    return name;
}

void CanvasElement::paintImage(const std::string imageId, int x, int y, const Rect& clippingRect) {
    if(clippingRect.width <= 0 || clippingRect.height <= 0)
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                    {"pos", std::vector<int>{x, y}}});
    else
        send("paint_image", std::unordered_map<std::string, std::any>{{"image", imageId},
                                                                         {"pos", std::vector<int>{x, y}},
                                                                         {"clip", std::vector<int>{clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height}}});
}

void CanvasElement::paintImage(const std::string imageId, const Rect& targetRect, const Element::Rect& clippingRect) {
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


void Graphics::drawRect(const Element::Rect& rect, Color color) {
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
    telex_graphics_assert(other.m_canvas->size() == m_canvas->size(), "Canvas sizes must match")
    auto pos = m_canvas->data();
    const auto posOther = other.m_canvas->data();
    for(auto i = 0U; i < m_canvas->size(); i++) {
       const auto p = pos[i];
       const auto po = posOther[i];
       const auto ao = Telex::Canvas::alpha(po);
       const auto a = Telex::Canvas::alpha(p);
       const auto r = Telex::Canvas::r(p) * (0xFF - ao);
       const auto g = Telex::Canvas::g(p) * (0xFF - ao);
       const auto b = Telex::Canvas::b(p) * (0xFF - ao);


       const auto ro = (Telex::Canvas::r(po) * ao);
       const auto go = (Telex::Canvas::g(po) * ao);
       const auto bo = (Telex::Canvas::b(po) * ao);

       const auto pix = Telex::Canvas::rgbaClamped((r + ro) / 0xFF , (g + go) / 0xFF, (b + bo) / 0xFF, a);
       pos[i] = pix;
    }
}

void Graphics::update() {
    if(m_canvas)
        m_element.paint(m_canvas);
}


