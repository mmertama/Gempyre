#ifndef KOCH_H
#define KOCH_H

#include <vector>

namespace Koch {
struct point {
    double x;
    double y;
};
std::vector<point> koch_points(double size, int iterations);
}

#endif // KOCH_H
