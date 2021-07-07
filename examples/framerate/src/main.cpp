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

constexpr int Width = 156;
constexpr int Height = 175;

constexpr std::array<int, 5> XCoords =  {0, 216, 434, 626, 826};
constexpr std::array<int, 3> YCoords = {0, 292, 531};

class Flake {
public:
    Flake(const std::tuple<int, int, int, double, int>& p) :
        ix(XCoords[std::get<0>(p) % 5]),
        iy(YCoords[std::get<0>(p) / 5]),
        m_x(std::get<1>(p)),
        m_y(std::get<2>(p)),
        m_fallspeed(std::get<3>(p)),
        m_size(std::get<4>(p)),
        m_d(static_cast<double>(m_y)) {
    }
    void draw(Gempyre::FrameComposer& fc) const {
        fc.drawImage("flakes",
                     {ix, iy, Width, Height},
                     {m_x, m_y, m_size, m_size}
                     );
    }

    void setPos(int x, int y) {
        m_x = x;
        m_y = y;
        m_d = static_cast<double>(m_y);
    }

    void fall(const int height) {
        m_d += m_fallspeed;
        m_y = static_cast<int>(m_d);
        if(m_y >= height) {
            m_y = -m_size;
            m_d = static_cast<double>(m_y);
        }
    }

private:
    const int ix;
    const int iy;
    int m_x = 0;
    int m_y = 0;
    const double m_fallspeed;
    const int m_size;
    double m_d = 0;
};

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

    const auto flake_params = [&generator, &rect](){
        std::uniform_int_distribution<int> distribution_s(0, 15);
        const auto s = distribution_s(generator);


        std::uniform_int_distribution<int> distribution_x(0, rect.width);
        const auto pos = distribution_x(generator);
        std::uniform_int_distribution<int> distribution_y(0, rect.height);
        const auto at = distribution_y(generator);

        std::uniform_int_distribution<int> distribution_f(800, 1200);
        const auto f = static_cast<double>(distribution_f(generator)) / 1000.;

        std::uniform_int_distribution<int> distribution_sz(40, 60);
        const auto sz = distribution_sz(generator);

        return std::make_tuple(s, pos, at, f, sz);
    };

    flakes_count.subscribe("change",  [&](const Gempyre::Event& ev) {
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
            flakes.emplace_back(flake_params());
        }

    }, {"value"});

    ui.onOpen([&]() {
        rect = *canvas.rect();
        const auto v = GempyreUtils::to<int>(flakes_count.values()->at("value"));
        update_a_label(v);

        std::uniform_int_distribution<int> distribution(0, rect.width);

        for(int i = 0; i < v; i++) {
            flakes.emplace_back(flake_params());
        }

        draw_flakes(); // must be called only once, otherwise requests will duplicate
    });

    ui.startPeriodic(1s, [&start, &duration, &frame_count, &counter, &tick_count]() {
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

    ui.startPeriodic(Speed, [&flakes, &rect, &tick_count]() {
        for(auto& f : flakes) {
            f.fall(rect.height);
        }
        ++tick_count;
    });

    ui.run();
}
