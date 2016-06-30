#include <vector>
#include "graphics/graphics.h"

namespace RayTrace {

    bool Collision (
        const Geometry::Line &line,
        const std::vector<Shape::Shape *> &shapes,
        float_max_t &distance,
        bool get_info,
        Geometry::Vec<3> &normal,
        bool &inside,
        Pigment::Color &pigment,
        Light::Material &material
    );

    Pigment::Color Trace (
        const Geometry::Line &line,
        const std::vector<Shape::Shape *> &shapes,
        Pigment::Color ambient,
        const std::vector<Light::Light *> &lights,
        const std::vector<Geometry::Vec<2>> &light_deviations = { { 0.0, 0.0 } },
        const std::vector<std::pair<Geometry::Vec<2>, float_max_t>> &reflect_deviations = { { 0.0, 0.0 } },
        const std::vector<std::pair<Geometry::Vec<2>, float_max_t>> &transmit_deviations = { { 0.0, 0.0 } },
        Pigment::Color color = Pigment::Color::rgb(127, 127, 127),
        unsigned jumps = 10
    );

};
