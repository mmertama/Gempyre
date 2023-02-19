#include "gempyre.h"
#include "gempyre_graphics.h"
#include "gempyre_utils.h"
#include "fire_resources.h"

#include <cmath>

/*
static 
float hue_to_rgb(float v1, float v2, float vH) {
	if (vH < 0)
		vH += 1.;

	if (vH > 1)
		vH -= 1.;

	if ((6. * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2. * vH) < 1.)
		return v2;

	if ((3. * vH) < 2.)
		return (v1 + (v2 - v1) * ((2.0f / 3.) - vH) * 6.);

	return v1;
}

struct HSL {
    float H, S, L;
};


static
Gempyre::Color::type hsl_to_rgb(const HSL& hsl) {
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	if (hsl.S == 0) {
		r = g = b = static_cast<uint8_t>(hsl.L * 255.);
	} else {
		const auto hue = static_cast<float>(hsl.H / 360.);
		const auto v2 = (hsl.L < 0.5f) ? (hsl.L * (1. + hsl.S)) : ((hsl.L + hsl.S) - (hsl.L * hsl.S));
		const auto v1 = 2. * hsl.L - v2;

		r = static_cast<unsigned char>(255. * hue_to_rgb(v1, v2, hue + (1.0f / 3.)));
		g = static_cast<unsigned char>(255. * hue_to_rgb(v1, v2, hue));
		b = static_cast<unsigned char>(255. * hue_to_rgb(v1, v2, hue - (1.0f / 3.)));
	}

	return Gempyre::Color::rgba(r, g, b);
}
*/

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
};


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

void draw_frame(Gempyre::Bitmap& canvas,
    std::vector<Gempyre::Color::type>& fire,
    const std::array<Gempyre::Color::type, 256>& palette) {
    const auto h = canvas.height();
    const auto w = canvas.width();    
    for(auto y = 1; y < h - 3; y++) {
        for(auto x = 1; x < w - 1; x++) {
            const auto p1 = pixel(fire, (x - 1 + w) % w, (y + 1) % h, w);
            const auto p2 = pixel(fire, (x + 0 + 0) % w, (y + 2) % h, w);
            const auto p3 = pixel(fire, (x + 1 + 0) % w, (y + 1) % h, w);
            const auto p4 = pixel(fire, (x + 0 + 0) % w, (y + 3) % h, w);
            set_pixel(fire, x, y, w, ((p1 + p2 + p3 + p4) * 64) / 257);
        }
    }

    for(auto y = 0; y < h; y++)

        for(auto x = 0; x < w; x++)  {
            canvas.set_pixel(x, y, palette[pixel(fire, x, y, w)]);
    }
    canvas.update();
}

void amain( Gempyre::Ui& ui, Gempyre::CanvasElement& canvas_element) {
    const auto rect = canvas_element.rect();
    gempyre_utils_assert(rect);
    auto palette = make_palette();
    auto fire_buffer = std::make_shared<std::vector<Gempyre::Color::type>>(rect->width * rect->height);
    auto canvas = std::make_shared<Gempyre::Bitmap>(canvas_element, rect->width, rect->height);
    canvas_element.draw_completed([canvas, fire_buffer, palette = std::move(palette)]() {
        draw_frame(*canvas, *fire_buffer, palette);
    }, Gempyre::CanvasElement::DrawNotify::Kick);
    ui.start_periodic(50ms, [rect, fire_buffer]() {
        //randomize the bottom row of the fire buffer
        auto& fire = *fire_buffer;
        for(int x = 0; x < rect->width; x++)
            set_pixel(fire, x, rect->height - 1, rect->width, std::abs(32768 + std::rand()) % 256);
        });
}

int main(int /*argc*/, char** /*argv*/) {
    Gempyre::Ui ui{Fire_resourcesh, "fire.html"};
    Gempyre::CanvasElement canvas_element{ui, "canvas"};
    ui.on_open([&ui, &canvas_element](){amain(ui, canvas_element);});
    ui.run();
}