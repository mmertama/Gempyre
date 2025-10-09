#include "gempyre.h"
#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "gempyre_client.h"
#include "fire_resources.h"

#include <chrono>
#include <cmath>
#include <cassert>
#include <array>
#include <iostream>
#include <fstream>

using Time = decltype(std::chrono::steady_clock::now());

// todo add function to gempyre (needs testing, this is fishy)
static
Gempyre::Color::type hsl_to_rgb(float h, float s, float l)  {
  s /= 100.;
  l /= 100.;
  
  const auto k = [&](auto n) -> float {return std::fmod((n + h / 30.), 12.);};
  const auto a = s * std::min(l, 1 - l);
  const auto f = [&](auto n) -> float {
    return l - a * std::max(-1., std::min(k(n) - 3., std::min(9. - k(n), 1.)));
  };
  return Gempyre::Color::rgba_clamped(255 * f(0), 255 * f(8), 255 * f(4));
}


static
std::array<Gempyre::Color::type, 256> make_palette() {
    //Hue goes from 0 to 85: red to yellow
    //Saturation is always the maximum: 255
    //Lightness is 0..255 for x=0..128, and 255 for x=128..255int
    std::array<Gempyre::Color::type, 256> palette;
    for (unsigned x = 0; x < 256; x++) {
        const auto f = static_cast<float>(x);
        const auto color = hsl_to_rgb(f / 3., 100, std::min(100.f, f * 3.f));
        //set the palette to the calculated RGB value
        palette[x] = color;
    }
    return palette;
}

inline Gempyre::Color::type pixel(const std::vector<Gempyre::Color::type>& vec, int x, int y, int w) {
    return vec[y * w + x];
}

inline void set_pixel(std::vector<Gempyre::Color::type>& vec, int x, int y, int w, Gempyre::Color::type col) {
    vec[y * w + x] = col;
}

void draw_fire(Gempyre::Bitmap& target, std::vector<Gempyre::Color::type>& fire) {
    const auto h = target.height();
    const auto w = target.width();    
    for(auto y = 1; y < h - 3; y++) {
        for(auto x = 1; x < w - 1; x++) {
            const auto p1 = pixel(fire, (x - 1 + w) % w, (y + 1) % h, w);
            const auto p2 = pixel(fire, (x + 0 + 0) % w, (y + 2) % h, w);
            const auto p3 = pixel(fire, (x + 1 + 0) % w, (y + 1) % h, w);
            const auto p4 = pixel(fire, (x + 0 + 0) % w, (y + 3) % h, w);
            set_pixel(fire, x, y, w, ((p1 + p2 + p3 + p4) * 64) / 257);
        }
    }
}

void draw_frame(
    Gempyre::CanvasElement& canvas,
    Gempyre::Bitmap& target,
    std::vector<Gempyre::Color::type>& fire,
    const std::array<Gempyre::Color::type, 256>& palette) {
        const auto h = target.height();
        const auto w = target.width();    
        for(auto y = 0; y < h; y++) {
            for(auto x = 0; x < w; x++)  {
                target.set_pixel(x, y, palette[pixel(fire, x, y, w)]);
            }
        }
    canvas.draw(0, 0, target);
}

void draw_fps(Gempyre::Ui& ui, unsigned& fps_count, Time& start) {
    if (fps_count > 100) {
        auto end = std::chrono::steady_clock::now(); 
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        const auto fps = static_cast<float>(1000 * fps_count) / duration.count();
        Gempyre::Element(ui, "fps").set_html(std::to_string(fps));
        fps_count = 0;
        start = std::chrono::steady_clock::now(); 
    }
}

void amain( Gempyre::Ui& ui, Gempyre::CanvasElement& canvas_element, const Gempyre::Bitmap& bmp, unsigned& fps_count, Time& start, std::weak_ptr<Gempyre::Bitmap>& target_ref) {
    const auto rect = canvas_element.rect();
    gempyre_utils_assert(rect);
    auto palette = make_palette();
    auto fire_buffer = std::make_shared<std::vector<Gempyre::Color::type>>(rect->width * rect->height);
    auto canvas = std::make_shared<Gempyre::Bitmap>(rect->width, rect->height);
    target_ref = canvas; // bind to weak;
    canvas_element.draw_completed([canvas_element, canvas, fire_buffer, palette = std::move(palette), &fps_count]() mutable {
        draw_frame(canvas_element, *canvas, *fire_buffer, palette);
        ++fps_count;
    }, Gempyre::CanvasElement::DrawNotify::Kick);
    ui.start_periodic(50ms, [&ui, rect, fire_buffer, &bmp, canvas, &start, &fps_count]() {
        auto& fire = *fire_buffer;
        draw_fire(*canvas, fire);
        const auto left = (rect->width - bmp.width()) / 2;
        const auto top = (rect->height - bmp.height()) / 2;
        for(int y = 0; y < bmp.height(); ++y)
            for(int x = 0; x < bmp.width(); ++x) {
                const auto px = bmp.pixel(x, y);
                if (px != Gempyre::Color::Black) {
                    const auto sat = Gempyre::Color::r(px) + Gempyre::Color::g(px) + Gempyre::Color::b(px); 
                    set_pixel(fire, left + x, top + y, rect->width,
                     std::abs(32768 + std::rand()) % (sat / 3));
                }
            }

        draw_fps(ui, fps_count, start);
        });
}

int main(int /*argc*/, char** /*argv*/) {
    GempyreUtils::set_log_level(GempyreUtils::LogLevel::Warning);
    Gempyre::Ui ui{Fire_resourcesh, "fire.html", "Fire", 620, 660};
    Gempyre::CanvasElement canvas_element{ui, "canvas"};
    const auto image_data = ui.resource("/grunge-skull.png");
    assert(image_data);
    Gempyre::Bitmap skull(*image_data);
    unsigned fps_count = 0;
    auto start = std::chrono::steady_clock::now();

    std::weak_ptr<Gempyre::Bitmap> target_ref; // refer to target bitmap

    Gempyre::Element(ui, "do_save").subscribe(Gempyre::Event::CLICK, [&](auto) {
        if(target_ref.use_count() == 0)  // do we have image?
            return;
        const Gempyre::Dialog::Filter image_filter ({{"Image", {"*.png"}}}); // setup a dialog filter
        const auto file = Gempyre::Dialog::save_file_dialog("Save file", "", image_filter);
        if(file) {
            std::ofstream out(*file, std::ios::binary | std::ios::out);
            if(!out.is_open()) {
                std::cerr << "Cannot write to " << *file;
                return; 
            }
            const auto png_data = target_ref.lock()->png_image();   // get data from weak pointer referenced bitmap
            out.write(reinterpret_cast<const char*>(png_data.data()), png_data.size()); // write data
        }
    });
    ui.on_open([&](){amain(ui, canvas_element, skull, fps_count, start, target_ref);});
    ui.run();
}