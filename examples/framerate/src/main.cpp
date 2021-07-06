#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "framerate_resource.h"
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <random>
#include <list>

using namespace std::chrono_literals;

constexpr auto Speed = 20ms;

class Flake {
static constexpr int size = 20;
public:
    Flake(int pos, int at = 0) : m_x(pos), m_y(at) {}
    void draw(Gempyre::FrameComposer& fc) const {
        fc.drawImage("flakes",
                     {0, 0, 200, 200},
                     {m_x, m_y, size, size}
                     );
    }

    void setPos(int x, int y) {
        m_x = x;
        m_y = y;
    }

    void fall(const int height) {
        ++m_y;
        if(m_y >= height)
            m_y = -size;
    }

private:
    int m_x = 0;
    int m_y = 0;};

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::setDebug(Gempyre::DebugLevel::Info);
    Gempyre::Ui ui(Framerate_resourceh, "framerate.html");
    Gempyre::CanvasElement canvas(ui, "canvas");
    Gempyre::Element flakes_count(ui, "flakes_count");
    Gempyre::Element flakes_label(ui, "flakes_label");
    Gempyre::Element counter(ui, "counter");
    Gempyre::Element::Rect rect;
    std::list<Flake> flakes;
    std::default_random_engine generator;
    auto start = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration;
    unsigned frame_count = 0;
    unsigned tick_count = 0;

    const auto draw_flakes = [&canvas, &rect, &flakes, &frame_count]() {
        Gempyre::FrameComposer fc;
        fc.clearRect({0, 0, rect.width, rect.height});
        for(const auto& f : flakes) {
            f.draw(fc);
        }
        canvas.draw(fc);
        ++frame_count;
    };

    canvas.drawCompleted(draw_flakes);

    const auto update_a_label = [&flakes_label](int iterations) {
          flakes_label.setHTML("Flakes: " + std::to_string(iterations));
    };

    flakes_count.subscribe("change",  [&update_a_label, &flakes, &rect, &generator](const Gempyre::Event& ev) {
        const auto v = GempyreUtils::to<unsigned>(ev.properties.at("value"));
        update_a_label(v);

        while(v < flakes.size()) {
            std::uniform_int_distribution<int> distribution(0, flakes.size() - 1);
            const auto pos = distribution(generator);
            auto it = flakes.begin();
            std::advance(it, pos);
            flakes.erase(it);
        }

        while(v > flakes.size()) {
            std::uniform_int_distribution<int> distribution(0, rect.width);
            const auto pos = distribution(generator);
            std::uniform_int_distribution<int> distribution_y(0, rect.height);
            const auto at = distribution_y(generator);
            flakes.emplace_back(pos, at);
        }

    }, {"value"});

    ui.onOpen([&flakes_count, &draw_flakes, &update_a_label, &canvas, &rect, &flakes, &generator]() {
        rect = *canvas.rect();
        const auto v = GempyreUtils::to<int>(flakes_count.values()->at("value"));
        update_a_label(v);

        std::uniform_int_distribution<int> distribution(0, rect.width);

        for(int i = 0; i < v; i++) {
            std::uniform_int_distribution<int> distribution_x(0, rect.width);
            const auto pos = distribution_x(generator);
            std::uniform_int_distribution<int> distribution_y(0, rect.height);
            const auto at = distribution_y(generator);
            flakes.emplace_back(pos, at);
        }

        draw_flakes(); // must be called only once, otherwise requests will duplicate
    });

    ui.startTimer(1s, false, [&start, &duration, &frame_count, &counter, &tick_count]() {
        const auto end = std::chrono::steady_clock::now();
        duration = end - start;
        start = end;
        const auto expected = duration / Speed;
        const auto deviation_persentage = (expected / tick_count) * 100.;
        const auto fps = static_cast<double>(frame_count) / duration.count();
        frame_count = 0;
        tick_count = 0;
        counter.setHTML("FPS:" + std::to_string(fps) + ", deviation:" + std::to_string(deviation_persentage) + "%");
    });

    ui.startTimer(Speed, false, [&flakes, &rect, &tick_count]() {
        for(auto& f : flakes) {
            f.fall(rect.height);
        }
        ++tick_count;
    });

    ui.run();
}
