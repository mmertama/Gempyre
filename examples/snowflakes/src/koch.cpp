// See https://en.wikipedia.org/wiki/Koch_snowflake
#include "koch.h"


using namespace Koch;


constexpr double sqrt3_2 = 0.86602540378444; // sqrt(3)/2

static std::vector<point> koch_next(const std::vector<point>& points) {
    const auto size = points.size();
    std::vector<point> output( 4 * (size - 1) + 1);
    double x0, y0, x1, y1;
    size_t j = 0;
    for (auto i = 0U; i < size - 1U; ++i) {
        x0 = points[i].x;
        y0 = points[i].y;
        x1 = points[i + 1].x;
        y1 = points[i + 1].y;
        const auto dy = y1 - y0;
        const auto dx = x1 - x0;
        output[j++] = {x0, y0};
        output[j++] = {x0 + dx / 3, y0 + dy / 3};
        output[j++] = {x0 + dx/2 - dy * sqrt3_2 / 3, y0 + dy / 2 + dx * sqrt3_2 / 3};
        output[j++] = {x0 + 2 * dx / 3, y0 + 2 * dy / 3};
    }
    output[j] = {x1, y1};
    return output;
}

std::vector<point> Koch::koch_points(double size, int iterations) {
    const auto length = size * sqrt3_2 * 0.95;
    const auto x = (size - length) / 2.;
    const auto y = size / 2. - length * sqrt3_2 / 3;
    std::vector<point> points {
        {x, y},
        {x + length / 2., y + length * sqrt3_2},
        {x + length, y},
        {x, y}
    };
    for (auto i = 0; i < iterations; ++i)
        points = koch_next(points);
    return points;
}

