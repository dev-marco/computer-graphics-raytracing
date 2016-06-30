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

        bool best_min, inside_min, inside_max;
        float_max_t t_min, t_max;
        Geometry::Vec<3> normal_min, normal_max;
        Pigment::Color color_min, color_max;
        Light::Material material_min, material_max;
        const Shape::Shape *best = nullptr;

        for (const Shape::Shape *shape : shapes) {

            if (shape->intersectLine(line, t_min, t_max, false, normal_min, normal_max, inside_min, inside_max, color_min, color_max, material_min, material_max)) {
                if (t_min > 0.0) {
                    if (t_min < distance) {
                        distance = t_min;
                        best = shape;
                        best_min = true;
                    }
                } else if (t_max > 0.0) {
                    if (t_max < distance) {
                        distance = t_max;
                        best = shape;
                        best_min = false;
                    }
                }
            }
        }

        if (best != nullptr) {
            if (get_info) {
                if (best_min) {
                    best->intersectLine(line, t_min, t_max, true, normal, normal_max, inside, inside_max, pigment, color_max, material, material_max);
                } else {
                    best->intersectLine(line, t_min, t_max, true, normal_min, normal, inside_min, inside, color_min, pigment, material_min, material);
                }
            }
            return true;
        }

        return false;
    }

    Pigment::Color Trace (
        const Geometry::Line &line,
        const std::vector<Shape::Shape *> &shapes,
        Pigment::Color ambient,
        const std::vector<Light::Light *> &lights,
        const std::vector<Geometry::Vec<2>> &light_deviations,
        const std::vector<std::pair<Geometry::Vec<2>, float_max_t>> &reflect_deviations,
        const std::vector<std::pair<Geometry::Vec<2>, float_max_t>> &transmit_deviations,
        Pigment::Color color,
        unsigned jumps
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
                    const Geometry::Vec<3>
                        reflect = (-2.0 * normal.dot(line.getDirection()) * normal + line.getDirection()).normalized(),
                        up_dir = reflect.perpendicular().normalized(),
                        right_dir = reflect.cross(up_dir).normalized(),
                        hit_point = point + reflect * 5.0;

                    Pigment::Color reflect_accumulated(0.0, 0.0, 0.0);

                    float_max_t total_weight = 0.0;

                    for (const auto &deviation : reflect_deviations) {
                        const Geometry::Vec<3> dir = ((hit_point + deviation.first[0] * right_dir + deviation.first[1] * up_dir) - point).normalized();
        			    reflect_accumulated += Trace(
                            Geometry::Line(point + dir * Geometry::EPSILON, dir),
                            shapes, ambient, lights,
                            light_deviations, reflect_deviations, transmit_deviations,
                            color, jumps - 1
                        ) * deviation.second;
                        total_weight += deviation.second;
                    }

                    reflected += (reflect_accumulated / total_weight) * material.getReflect();
                }

                if (material.getTransmit() > Geometry::EPSILON) {
                    const float_max_t
                        nr = inside ? material.getIOR() : (1.0 / material.getIOR()),
                        ndl = normal.dot(-line.getDirection()),
                        root = 1.0 - (nr * nr) * (1.0 - (ndl * ndl));
                    if (root >= 0.0) {
                        const Geometry::Vec<3>
                            transmit = ((nr * ndl - std::sqrt(root)) * normal - nr * (-line.getDirection())).normalized(),
                            up_dir = transmit.perpendicular().normalized(),
                            right_dir = transmit.cross(up_dir).normalized(),
                            hit_point = point + transmit * 5.0;

                        Pigment::Color transmit_accumulated(0.0, 0.0, 0.0);

                        float_max_t total_weight = 0.0;

                        for (const auto &deviation : transmit_deviations) {
                            const Geometry::Vec<3> dir = ((hit_point + deviation.first[0] * right_dir + deviation.first[1] * up_dir) - point).normalized();
            			    transmit_accumulated += Trace(
                                Geometry::Line(point + dir * Geometry::EPSILON, dir),
                                shapes, ambient, lights,
                                light_deviations, reflect_deviations, transmit_deviations,
                                color, jumps - 1
                            ) * deviation.second;
                            total_weight += deviation.second;
                        }

                        transmitted += (transmit_accumulated / total_weight) * material.getTransmit();
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

                    Pigment::Color light_accumulated(0.0, 0.0, 0.0);

                    const Geometry::Vec<3> delta = light->getPosition() - point;
                    const float_max_t light_distance = delta.length();
                    const Geometry::Vec<3>
                        direction = delta / light_distance,
                        up_dir = direction.perpendicular().normalized(),
                        right_dir = direction.cross(up_dir).normalized();

                    for (const auto &deviation : light_deviations) {
                        const Geometry::Vec<3> dir = ((light->getPosition() + deviation[0] * right_dir + deviation[1] * up_dir) - point).normalized();

                        float_max_t obstacle_distance = light_distance;

                        bool collides = Collision(
                            Geometry::Line(point + dir * Geometry::EPSILON, dir),
                            shapes,
                            obstacle_distance,
                            false,
                            normal_ignore,
                            inside_ignore,
                            pigment_ignore,
                            material_ignore
                        );

                        if (!collides) {
                            const Geometry::Vec<3> h = ((dir - line.getDirection()) / 2).normalized();
                            const float_max_t attenuation = 1.0 / (
                                light->getConstantAttenuation() +
                                light_distance * light->getLinearAttenuation() +
                                light_distance * light_distance * light->getQuadraticAttenuation()
                            );

                            Pigment::Color
                                diffuse = (std::max(normal.dot(dir), 0.0) * material.getDiffuse()) * pigment * light->getColor(),
                                specular = (std::pow(normal.dot(h), material.getAlpha()) * material.getSpecular()) * light->getColor();

                            light_accumulated += (diffuse + specular) * attenuation;
                        }
                    }

                    accumulated += light_accumulated / light_deviations.size();
                }
            }

            color = reflected + ambient + accumulated + transmitted;

        }

        return color;
    }
}
