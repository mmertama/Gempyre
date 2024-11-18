#include <iostream>
#include <cmath>
#include <array>
#include <cassert>

#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "snowflakes_resource.h"

#include "koch.h"

using namespace std::chrono_literals;

class Flake {
public:
    using Vertices = std::vector<Koch::point>;
    Flake(const Vertices& vertices, int size) : m_vertices(vertices), m_size(size) {
    }
    void draw(Gempyre::FrameComposer& f) const {
        f.save();
        f.translate(m_x - m_size / 2, m_y - m_size / 2);
        f.begin_path();
        auto it = m_vertices.begin();
        f.move_to(it->x, it->y);
        ++it;
        for(; it != m_vertices.end(); ++it)
            f.line_to(it->x, it->y);
        f.stroke();
        f.restore();
    }
    void setPos(int x, int y) {
        m_x = x;
        m_y = y;
    }
private:
    int m_x = 0;
    int m_y = 0;
    const Vertices& m_vertices;
    const int m_size;
};

constexpr int Size = 400;

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::set_debug(true);
    Gempyre::Ui ui({{"/snowflakes.html", Snowflakeshtml}}, "snowflakes.html", "Koch flake", 620, 620);
    Gempyre::CanvasElement canvas(ui, "canvas");
    Gempyre::Element flakes_count(ui, "flakes_count");
    Gempyre::Element flakes_info(ui, "flakes_info");

    Gempyre::Element::Rect rect;
    std::array<std::pair<std::vector<Koch::point>, std::chrono::milliseconds>, 10> m_kochs;



    const auto draw_a_flake = [&](int iterations) {
        if(m_kochs[iterations].first.empty()) {
            flakes_info.set_html("Calculating....");
            ui.flush(); // since next operation is slow we ensure that this get handled
            const auto start = std::chrono::steady_clock::now();
            auto points =  Koch::koch_points(Size, iterations);
            const auto end = std::chrono::steady_clock::now(); 
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            m_kochs[iterations] = {std::move(points), duration};
        }
        const auto start = std::chrono::steady_clock::now();
        Flake f(m_kochs[iterations].first, Size);
        f.setPos(rect.width / 2, rect.height / 2);
        Gempyre::FrameComposer fc;
        fc.clear_rect(rect);
        f.draw(fc);
        canvas.draw(fc);
        const auto end = std::chrono::steady_clock::now(); 
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::stringstream info;
        info << "Iterations: "<< iterations << ", Size: " << (m_kochs[iterations].first.size() - 1) << ", Calc:" << m_kochs[iterations].second.count() << "ms, Draw:" << duration.count() << "ms";
        flakes_info.set_html(info.str());
    };

    flakes_count.subscribe(Gempyre::Event::CHANGE, [&draw_a_flake](const Gempyre::Event& ev) {
        const auto v = GempyreUtils::convert<unsigned>(ev.properties.at("value"));
        assert(v <= 10);
        draw_a_flake(v);
    }, {"value"});

    ui.on_open([&flakes_count, &draw_a_flake, &canvas, &rect]() {
        rect = *canvas.rect();
        const auto v = GempyreUtils::convert<unsigned>(flakes_count.values()->at("value"));
        draw_a_flake(v);
    });

    ui.run();
}
