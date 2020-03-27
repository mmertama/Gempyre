#ifndef MANDELBROTDRAW_H
#define MANDELBROTDRAW_H

#include "mandelbrot.h"
#include <telex_graphics.h>
#include <telex_utils.h>

#include <array>
#include <cmath>

#include <future>
#include <mutex>

using namespace std::chrono_literals;

class MandelbrotDraw {
public:
    using Color = Telex::Color::type;
    MandelbrotDraw(Telex::Graphics& g, const Mandelbrot::Number& left, const Mandelbrot::Number& top, const Mandelbrot::Number& right, const Mandelbrot::Number& bottom, int iterations) :
        m_g(g), m_height(static_cast<Mandelbrot::Number>(g.height())), m_left(left), m_right(right), m_top(top), m_bottom(bottom), m_iterations(iterations) {
        makeLut();
    }

    inline Mandelbrot::Number real(const Mandelbrot::Number& r) const {return coord(m_left, m_right, r);}
    inline Mandelbrot::Number img(const Mandelbrot::Number& i) const {return coord(m_top, m_bottom, i);}

    void set(const Mandelbrot::Number& left, const Mandelbrot::Number& top, const Mandelbrot::Number& right, const Mandelbrot::Number& bottom) {
        cancel();
        m_left = left; m_right = right; m_top = top; m_bottom = bottom;
    }

    void setRect(int x, int y, int width, int height) {
        const auto l = real(x);
        const auto t = img(y);
        const auto r = real(x + width);
        const auto b = img(y + height);
        set(l, t, r, b);
    }

    void setIterations(int iterations) {
        cancel();
        m_iterations = iterations;
        makeLut();
    }

    Mandelbrot::Number radius() const {
        return Mandelbrot::sqrt(Mandelbrot::Complex((m_right - m_left) / Mandelbrot::Number(2), (m_bottom - m_top) / Mandelbrot::Number(2)).abs2());
    }

    void setColors(int colors) {
        cancel();
        m_colorCycles = colors;
        makeLut();
    }

    std::array<Mandelbrot::Number, 4> coords() const {
        return {m_left, m_top, m_right, m_bottom};
    }

    void update(std::function<void (int, int)> onComplete) {
        m_results.clear();
        m_updates = 0;
        m_cancel = false;
        const auto threads = 11;
        const auto updater = [this, onComplete, threads](int hstart, int hend) { //yes it is needed for MSVC :-(
           // const auto [minpri, maxpri] = TelexUtils::getPriorityLevels();
           // (void) maxpri;
           //  TelexUtils::setPriority(20);
            for(auto y = hstart; y < hend; y++) {
                if(m_cancel)
                    return;
                for(auto x = 0; x < m_g.width(); x++) {
                    const Mandelbrot::Complex c (real(static_cast<double>(x)),
                                                 img(static_cast<double>(y)));
                    const auto it = Mandelbrot::calculate(c, m_iterations);
                    if(it < m_iterations)
                        m_g.setPixel(x, y, m_colorlut[static_cast<unsigned>(it)]);
                    else
                        m_g.setPixel(x, y, Telex::Graphics::Black);
                    std::this_thread::yield(); //let the other thread run
                }
                 std::this_thread::sleep_for(100ms); //make others happen
            }
            ++m_updates;
            std::lock_guard<std::mutex> lock(m_mutex);
            onComplete(m_updates, threads + 1);
        };
        int linesInThread = m_g.height() / threads;
        int lastLinesInThread = m_g.height() % threads;
        for(auto n = 0; n < threads; n++)
            m_results.push_back(std::async(std::launch::async, updater, n * linesInThread, (n + 1) * linesInThread));
        m_results.push_back(std::async(std::launch::async, updater, m_g.height() - lastLinesInThread , m_g.height()));
        onComplete(0, threads);
    }
    void blend(Color colorStart, Color colorEnd) {
        cancel();
        m_colorStart = colorStart;
        m_colorEnd = colorEnd;
        makeLut();
    }
private:
    inline Mandelbrot::Number coord(const Mandelbrot::Number& start, const Mandelbrot::Number& end, const Mandelbrot::Number& screenPos) const {
        return start + (screenPos / m_height) * (end - start);
    }
    void makeLut() {
        m_colorlut.resize(static_cast<size_t>(m_iterations));
        const auto r0 = static_cast<double>(Telex::Color::r(m_colorStart));
        const auto g0 = static_cast<double>(Telex::Color::g(m_colorStart));
        const auto b0 = static_cast<double>(Telex::Color::b(m_colorStart));
        const auto r1 = static_cast<double>(Telex::Color::r(m_colorEnd));
        const auto g1 = static_cast<double>(Telex::Color::g(m_colorEnd));
        const auto b1 = static_cast<double>(Telex::Color::b(m_colorEnd));
        auto r = r0;
        auto g = g0;
        auto b = b0;
        auto lutpos = 0U;

        const auto lutcycle = static_cast<double>(m_iterations) / static_cast<double>(m_colorCycles);
        int cycle = 0;
        for(; cycle < m_colorCycles; ++cycle) {
            const bool odd = cycle & 0x1;
            const auto dr = (odd ? (r0 - r1) : (r1 - r0)) / lutcycle;
            const auto dg = (odd ? (g0 - g1) : (g1 - g0)) / lutcycle;
            const auto db = (odd ? (b0 - b1) : (b1 - b0)) / lutcycle;
            for(int i= 0; i < static_cast<int>(lutcycle); ++i) {
                m_colorlut[lutpos] = Telex::Graphics::pix(static_cast<unsigned>(r), static_cast<unsigned>(g), static_cast<unsigned>(b));
                r += dr;
                g += dg;
                b += db;
                ++lutpos;
            }
        }
        const bool odd = cycle & 0x1;
        const auto dr = (odd ? (r0 - r1) : (r1 - r0)) / lutcycle;
        const auto dg = (odd ? (g0 - g1) : (g1 - g0)) / lutcycle;
        const auto db = (odd ? (b0 - b1) : (b1 - b0)) / lutcycle;
        for(; lutpos < static_cast<unsigned>(m_iterations); ++lutpos) {
            m_colorlut[lutpos] = Telex::Graphics::pix(static_cast<unsigned>(r), static_cast<unsigned>(g), static_cast<unsigned>(b));
            r += dr;
            g += dg;
            b += db;
        }
    }

    void cancel() {
        m_cancel = true;
        std::for_each(m_results.begin(), m_results.end(), [](auto& f){f.wait_for(5s);});
    }

    Telex::Graphics& m_g;
    const Mandelbrot::Number m_height;
    Mandelbrot::Number m_left, m_right, m_top, m_bottom;
    int m_iterations;
    Color m_colorStart = Telex::Graphics::Red;
    Color m_colorEnd = Telex::Graphics::Blue;
    int m_colorCycles = 1;
    std::vector<Color> m_colorlut;
    std::vector<std::future<void>> m_results;
    std::atomic_bool m_cancel = false;
    std::atomic_int m_updates = 0;
    std::mutex m_mutex;
};


#endif // MANDELBROTDRAW_H
