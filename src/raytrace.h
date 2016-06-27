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
        Pigment::Color color = Pigment::Color::rgb(127, 127, 127),
        unsigned jumps = 10,
        float_max_t travelled = 100.0
    );

};
