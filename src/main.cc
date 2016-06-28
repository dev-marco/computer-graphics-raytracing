#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "raytrace.h"
#include "graphics/graphics.h"
#include "filemanip.h"

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
        eye_pos = camera.getPosition(),
        up_dir = camera.getUpDirection(),
        camera_direction = camera.getDirection(),
        camera_right = camera_direction.cross(up_dir),
        camera_up = camera_right.cross(camera_direction);

    std::vector<float_max_t> pixel_x_cache(image_width);

    cv::Mat img = cv::Mat::zeros(image_height, image_width, CV_8UC3);

    for (unsigned pixel_y = 0; pixel_y < image_height; ++pixel_y) {
        const float_max_t world_y = (1.0 - static_cast<float_max_t>((pixel_y << 1) + 1) * inv_image_height) * scale;
        for (unsigned pixel_x = 0; pixel_x < image_width; ++pixel_x) {

            if (pixel_y == 0) {
                pixel_x_cache[pixel_x] = (static_cast<float_max_t>((pixel_x << 1) + 1) * inv_image_width - 1.0) * aspect_ratio * scale;
            }

            const Geometry::Vec<3> position =
                pixel_x_cache[pixel_x] * camera_right + world_y * camera_up + eye_pos + camera_direction;

            img.at<cv::Vec3b>(pixel_y, pixel_x) = RayTrace::Trace(
                Geometry::Line(position, use_projection ? (position - eye_pos).normalized() : camera_direction.normalized()),
                shapes, ambient, lights
            ).intervalFixed();
        }
    }

    cv::imwrite(argv[2], img);

    return 0;
}
