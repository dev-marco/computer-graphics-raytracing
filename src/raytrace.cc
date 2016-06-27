#include "raytrace.h"

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
    ) {

        bool inside_min, inside_max, found = false;
        float_max_t t_min, t_max;
        Geometry::Vec<3> normal_min, normal_max;
        Pigment::Color color_min, color_max;
        Light::Material material_min, material_max;

        for (const auto &shape : shapes) {

            if (shape->intersectLine(line, t_min, t_max, get_info, normal_min, normal_max, inside_min, inside_max, color_min, color_max, material_min, material_max)) {
                if (t_min > 0.0) {
                    if (t_min < distance) {
                        distance = t_min;
                        normal = normal_min;
                        inside = inside_min;
                        pigment = color_min;
                        material = material_min;
                        found = true;
                    }
                } else if (t_max > 0.0) {
                    if (t_max < distance) {
                        distance = t_max;
                        normal = normal_max;
                        inside = inside_max;
                        pigment = color_max;
                        material = material_max;
                        found = true;
                    }
                }
            }
        }

        return found;
    }

    Pigment::Color Trace (
        const Geometry::Line &line,
        const std::vector<Shape::Shape *> &shapes,
        Pigment::Color ambient,
        const std::vector<Light::Light *> &lights,
        Pigment::Color color,
        unsigned jumps,
        float_max_t travelled
    ) {

        float_max_t distance = std::numeric_limits<float_max_t>::infinity();
        Geometry::Vec<3> normal;
        Pigment::Color pigment;
        Light::Material material;
        bool inside;

        if (Collision(line, shapes, distance, true, normal, inside, pigment, material)) {

            const Geometry::Vec<3> &point = line.at(distance);

            Pigment::Color
                reflected(0.0, 0.0, 0.0),
                transmitted(0.0, 0.0, 0.0),
                accumulated(0.0, 0.0, 0.0);

            normal += material.getNormal();

            if (jumps > 0) {

                if (material.getReflect() > Geometry::EPSILON) {
                    const Geometry::Vec<3> reflect = (-2.0 * normal.dot(line.getDirection()) * normal + line.getDirection()).normalized();
        			reflected = Trace(Geometry::Line(point + reflect * Geometry::EPSILON, reflect), shapes, ambient, lights, color, jumps - 1, travelled - distance) * material.getReflect();
                }

                if (material.getTransmit() > Geometry::EPSILON) {
                    const float_max_t
                        nr = inside ? material.getIOR() : (1.0 / material.getIOR()),
                        ndl = normal.dot(-line.getDirection()),
                        root = 1.0 - (nr * nr) * (1.0 - (ndl * ndl));
                    if (root >= 0.0) {
                        const Geometry::Vec<3> transmit = ((nr * ndl - std::sqrt(root)) * normal - nr * (-line.getDirection())).normalized();
                        transmitted = Trace(Geometry::Line(point + transmit * Geometry::EPSILON, transmit), shapes, ambient, lights, color, jumps - 1, travelled - distance) * material.getTransmit();
                    }
                }
            }

            ambient *= material.getAmbient() * pigment;

            if (material.getSpecular() > Geometry::EPSILON || material.getDiffuse() > Geometry::EPSILON) {
                for (const auto &light : lights) {

                    static Geometry::Vec<3> normal_ignore;
                    static Pigment::Color pigment_ignore;
                    static Light::Material material_ignore;
                    bool inside_ignore;

                    const Geometry::Vec<3> delta = light->getPosition() - point;
                    const float_max_t light_distance = delta.length();
                    const Geometry::Vec<3> direction = delta / light_distance;
                    float_max_t obstacle_distance = light_distance;

                    bool collides = Collision(Geometry::Line(point + direction * Geometry::EPSILON, direction), shapes, obstacle_distance, false, normal_ignore, inside_ignore, pigment_ignore, material_ignore);

                    if (!collides) {
                        const Geometry::Vec<3> h = ((direction - line.getDirection()) / 2).normalized();
                        const float_max_t attenuation = 1.0 / (
                            light->getConstantAttenuation() +
                            light_distance * light->getLinearAttenuation() +
                            light_distance * light_distance * light->getQuadraticAttenuation()
                        );

                        Pigment::Color
                            diffuse = pigment * std::max(normal.dot(direction), 0.0) * material.getDiffuse() * light->getColor(),
                            specular = std::pow(normal.dot(h), material.getAlpha()) * material.getSpecular() * light->getColor();

                        accumulated += (diffuse + specular) * attenuation;
                    }
                }
            }

            color = reflected + ambient + accumulated + transmitted;


        }

        return color;
    }
}
