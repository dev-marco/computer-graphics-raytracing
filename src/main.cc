#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "graphics/graphics.h"
#include "filemanip.h"

// [ noise ] (const Geometry::Vec<2> &param) -> Pigment::Color {
//
//     // return { (1 + std::sin(param[0])) * 0.5, (1.0 + std::sin(param[0])) * 0.5, (1 + std::sin(param[0])) * 0.5 };
//
//     float_max_t
//         s = std::fmod(param[0], 500.0) + 500.0,
//         t = std::fmod(param[1], 500.0) + 500.0,
//         value = (1.0 + std::sin((s + noise.at(s * 5.0, t * 5.0, 0.0) * 0.5) * 50.0)) * 0.5;
//
//     // return { noise.at(s, t), noise.at(s, t), noise.at(s, t) };
//
//     // return { value, value, value };
//
//     s = Geometry::fract(param[0]), t = Geometry::fract(param[1]);
//     if ((0.49 < s && s < 0.51) || (0.49 < t && t < 0.51)) {
//         return { 0.5, 0.5, 0.5 };
//     }
//     bool right = std::round(s) > 0.5, top = std::round(t) > 0.5;
//
//     if ((top && right) || !(top || right)) {
//         return { 1.0, 1.0, 1.0 };
//     }
//     return { 0.0, 0.0, 0.0 };
// }

int main (int argc, const char *argv[]) {

    const unsigned
        image_width = 800,
        image_height = 600;
    const bool use_projection = true;

    Geometry::Camera camera;
    Pigment::Color ambient;
    std::vector<Light::Light *> lights;
    std::vector<Pigment::Texture *> pigments;
    std::vector<Light::Surface *> surfaces;
    std::vector<Shape::Shape *> shapes;

    FileManip::readFile(argv[1], camera, ambient, lights, pigments, surfaces, shapes);

    const float_max_t
        inv_image_width = 1.0 / static_cast<float_max_t>(image_width),
        inv_image_height = 1.0 / static_cast<float_max_t>(image_height),
        aspect_ratio = static_cast<float_max_t>(image_width) * inv_image_height,
        scale = std::tan(camera.getFieldOfView() * 0.5);
    const Geometry::Vec<3>
        look_at = camera.getLookAt(),
        eye_pos = camera.getPosition(),
        up_dir = camera.getUpDirection(),
        camera_direction = (look_at - eye_pos).normalized(),
        camera_right = camera_direction.cross(up_dir),
        camera_up = camera_right.cross(camera_direction);

    std::vector<float_max_t> pixel_x_cache(image_width);

    cv::Mat img = cv::Mat::zeros(image_height, image_width, CV_8UC3);

    float_max_t t_min, t_max;
    Geometry::Vec<3> normal_min, normal_max;
    Pigment::Color color_min, color_max;
    Light::Material material_min, material_max;

    for (unsigned pixel_y = 0; pixel_y < image_height; ++pixel_y) {
        const float_max_t world_y = (1.0 - static_cast<float_max_t>((pixel_y << 1) + 1) * inv_image_height) * scale;
        for (unsigned pixel_x = 0; pixel_x < image_width; ++pixel_x) {

            if (pixel_y == 0) {
                pixel_x_cache[pixel_x] = (static_cast<float_max_t>((pixel_x << 1) + 1) * inv_image_width - 1.0) * aspect_ratio * scale;
            }

            const Geometry::Vec<3> position =
                pixel_x_cache[pixel_x] * camera_right + world_y * camera_up + eye_pos + camera_direction;

            const Geometry::Line line(position, use_projection ? (position - eye_pos).normalized() : camera_direction);
            float_max_t global_min = std::numeric_limits<float_max_t>::infinity();

            for (const auto &shape : shapes) {

                if (shape->intersectLine(line, normal_min, normal_max, color_min, color_max, material_min, material_max, t_min, t_max, true)) {
                    if (t_min > 0.0) {
                        if (t_min < global_min) {
                            global_min = t_min;
                            img.at<cv::Vec3b>(pixel_y, pixel_x) = color_min;
                        }
                    } else if (t_max > 0.0) {
                        if (t_max < global_min) {
                            global_min = t_max;
                            img.at<cv::Vec3b>(pixel_y, pixel_x) = color_max;
                        }
                    }
                }
            }
        }
    }

    cv::imwrite(argv[2], img);

    return 0;
}
