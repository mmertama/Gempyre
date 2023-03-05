#include <iostream>
#include <cmath>

#include <unordered_map>

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
    Gempyre::Ui ui({{"/snowflakes.html", Snowflakeshtml}}, "snowflakes.html");
    Gempyre::CanvasElement canvas(ui, "canvas");
    Gempyre::Element flakes_count(ui, "flakes_count");
    Gempyre::Element flakes_label(ui, "flakes_label");

    Gempyre::Element::Rect rect;
    std::unordered_map<int, std::vector<Koch::point>> m_cache;



    const auto draw_a_flake = [&canvas, &rect, &m_cache](int iterations) {
        if(m_cache.find(iterations) == m_cache.end()) {
            m_cache.emplace(iterations, Koch::koch_points(Size, iterations));
            GempyreUtils::log(GempyreUtils::LogLevel::Info,
                "snow flake with",
                m_cache[iterations].size(),
                "vertices on", iterations,
                "iterations.");
        }
        Flake f(m_cache[iterations], Size);
        f.setPos(rect.width / 2, rect.height / 2);
        Gempyre::FrameComposer fc;
        fc.clear_rect(rect);
        f.draw(fc);
        canvas.draw(fc);
    };

    const auto update_a_label = [&flakes_label](int iterations) {
          flakes_label.set_html("Iterations: " + std::to_string(iterations));
    };

    flakes_count.subscribe("change", [&draw_a_flake, &update_a_label](const Gempyre::Event& ev) {
        const auto v = GempyreUtils::convert<int>(ev.properties.at("value"));
        update_a_label(v);
        draw_a_flake(v);
    }, {"value"});

    ui.on_open([&flakes_count, &draw_a_flake, &update_a_label, &canvas, &rect]() {
        rect = *canvas.rect();
        const auto v = GempyreUtils::convert<int>(flakes_count.values()->at("value"));
        update_a_label(v);
        draw_a_flake(v);
    });

    ui.run();
}
