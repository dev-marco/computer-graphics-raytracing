#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <omp.h>
#include <opencv2/opencv.hpp>
#include "raytrace.h"
#include "graphics/graphics.h"
#include "filemanip.h"

int main (int argc, const char *argv[]) {

    Geometry::Camera camera;
    Pigment::Color ambient;
    std::vector<Light::Light *> lights;
    std::vector<Pigment::Texture *> pigments;
    std::vector<Light::Surface *> surfaces;
    std::vector<Shape::Shape *> shapes;
    std::string input_file, texture_dir, output_file = "output.png";

    bool
        use_poisson = false,
        use_super_sampling = false,
        use_orthogonal = false,
        use_light_distr = false,
        use_reflect_distr = false,
        use_transmit_distr = false,
        debug_mode = false;

    float_max_t
        poisson_distance = 0.3,
        light_side = 1.0;

    unsigned
        over_samples = 2,
        light_rays = 4,
        reflect_rays = 2,
        transmit_rays = 2,
        recursion_levels = 10,
        image_width = 800,
        image_height = 600;

    for (int i = 1; i < argc; ++i) {

        std::string value, arg = argv[i];

        auto equal = arg.find_first_of('=');

        if (equal != std::string::npos) {
            value = arg.substr(equal + 1);
            arg = arg.substr(0, equal);
        }

        if (arg == "-i") {
            input_file = argv[++i];
        } else if (arg == "-o") {
            output_file = argv[++i];
        } else if (arg == "--width") {
            if (!value.empty()) {
                image_width = std::stoi(value);
            }
        } else if (arg == "--height") {
            if (!value.empty()) {
                image_height = std::stoi(value);
            }
        } else if (arg == "--poisson") {
            use_poisson = true;
            use_super_sampling = false;
            if (!value.empty()) {
                poisson_distance = std::stod(value);
            }
        } else if (arg == "--super-sample") {
            use_super_sampling = true;
            use_poisson = false;
            if (!value.empty()) {
                over_samples = std::stoi(value);
            }
        } else if (arg == "--orthogonal") {
            use_orthogonal = true;
        } else if (arg == "--light-rays") {
            if (!value.empty()) {
                light_rays = std::stoi(value);
                use_light_distr = light_side > 0.0 && light_rays > 0;
            }
        } else if (arg == "--light-area") {
            if (!value.empty()) {
                light_side = std::sqrt(std::stod(value));
                use_light_distr = light_side > 0.0 && light_rays > 0;
            }
        } else if (arg == "--reflect-rays") {
            if (!value.empty()) {
                reflect_rays = std::stoi(value);
                use_reflect_distr = reflect_rays > 0;
            }
        } else if (arg == "--transmit-rays") {
            if (!value.empty()) {
                transmit_rays = std::stoi(value);
                use_transmit_distr = transmit_rays > 0;
            }
        } else if (arg == "--recurse") {
            if (!value.empty()) {
                recursion_levels = std::stoi(value);
            }
        } else if (arg == "--debug") {
            debug_mode = true;
        } else {
            std::cerr << "Unknown option '" << arg << "'. Ignoring." << std::endl;
        }
    }

    if (input_file.empty()) {
        std::cout
            << "Execution:" << std::endl
            << "$ bin/raytracing -i <INPUT_FILE> [ -o OUTPUT_FILE = output.png ] [ OPTIONS ]" << std::endl
            << "OPTIONS:" << std::endl
            << "--width=WID        : Resulting image width in pixels. Default: WID = 800" << std::endl
            << "--height=HEI       : Resulting image height in pixels. Default: HEI = 600" << std::endl
            << "--poison=POI       : Minimum distance between ray samples (anti-aliasing). Disables super-sampling. Default: POI = 0.4" << std::endl
            << "--super-sample=SS  : Square root of ray amount samples to take (anti-aliasing). Disables poisson. Default: SS = 2" << std::endl
            << "--light-rays=LR    : Square root of rays amount to cast in lights, excluding the central (distributed ray-tracing). Default: LR = 4" << std::endl
            << "--light-area=LA    : Area of every light. Enables light rays. Default: LA = 1.0" << std::endl
            << "--reflect-rays=RR  : Square root of rays amount to cast after reflection, excluding the central (distributed ray-tracing). Default: RR = 2" << std::endl
            << "--transmit-rays=TR : Square root of rays amount to cast after transmission, excluding the central (distributed ray-tracing). Default: TR = 2" << std::endl
            << "--recurse=REC      : Amount of levels of recursion levels to use. Default: REC = 10" << std::endl
            << "--orthogonal       : Use orthogonal projection (may lead to unexpected results). Default: DISABLED" << std::endl
            << "--debug            : Enable debug mode (prints image line). Default: DISABLED" << std::endl;
        return 1;
    }

    FileManip::readFile(input_file, camera, ambient, lights, pigments, surfaces, shapes);

    constexpr float_max_t
        reflect_side = 1.0,
        half_reflect_size = reflect_side * 0.5,
        transmit_side = 1.0,
        half_transmit_size = transmit_side * 0.5;

    const float_max_t
        half_light_side = light_side * 0.5,
        light_step = light_side / light_rays,
        transmit_step = transmit_side / transmit_rays,
        transmit_diag = std::sqrt(transmit_side + transmit_side),
        reflect_step = reflect_side / reflect_rays,
        reflect_diag = std::sqrt(reflect_side + reflect_side);

    const float_max_t
        inv_image_width = 1.0 / static_cast<float_max_t>(image_width),
        inv_image_height = 1.0 / static_cast<float_max_t>(image_height),
        aspect_ratio = static_cast<float_max_t>(image_width) * inv_image_height,
        scale = std::tan(camera.getFieldOfView() * 0.5);
    const Geometry::Vec<3>
        eye_pos = camera.getPosition(),
        up_dir = camera.getUpDirection(),
        camera_direction = camera.getDirection(),
        camera_right = camera_direction.cross(up_dir).normalized(),
        camera_up = camera_right.cross(camera_direction),
        camera_offset = eye_pos + camera_direction,
        x_ratio = (scale * aspect_ratio) * camera_right,
        y_ratio = scale * camera_up;
    std::vector<Geometry::Vec<2>>
        deviations, light_deviations = { { 0.0, 0.0 } };
    std::vector<std::pair<Geometry::Vec<2>, float_max_t>>
        reflect_deviations = { { { 0.0, 0.0 }, reflect_diag } },
        transmit_deviations = { { { 0.0, 0.0 }, transmit_diag } };

    if (use_light_distr) {

        for (unsigned i = 0; i < light_rays; ++i) {
            const float_max_t y_samples = (light_step * i) - half_light_side;
            for (unsigned j = 0; j < light_rays; ++j) {
                light_deviations.push_back({ y_samples, (light_step * j) - half_light_side });
            }
        }
    }

    if (use_reflect_distr) {

        for (unsigned i = 0; i < reflect_rays; ++i) {
            const float_max_t y_samples = (reflect_step * i) - half_reflect_size;
            for (unsigned j = 0; j < reflect_rays; ++j) {
                const Geometry::Vec<2> point = { y_samples, (reflect_step * j) - half_reflect_size };
                reflect_deviations.push_back({ point, reflect_diag - point.distance(0.0) });
            }
        }
    }

    if (use_transmit_distr) {

        for (unsigned i = 0; i < transmit_rays; ++i) {
            const float_max_t y_samples = (transmit_step * i) - half_transmit_size;
            for (unsigned j = 0; j < transmit_rays; ++j) {
                const Geometry::Vec<2> point = { y_samples, (transmit_step * j) - half_transmit_size };
                transmit_deviations.push_back({ point, transmit_diag - point.distance(0.0) });
            }
        }
    }

    cv::Mat img = cv::Mat::zeros(image_height, image_width, CV_8UC3);

    if (use_poisson) {
        Geometry::PoissonDisc poisson(poisson_distance);
        deviations = poisson.allPoints();
    } else if (use_super_sampling) {
        float_max_t step = 1.0 / over_samples;
        for (unsigned i = 0; i < over_samples; ++i) {
            float_max_t y_samples = step * i;
            for (unsigned j = 0; j < over_samples; ++j) {
                deviations.push_back({ y_samples, step * j });
            }
        }
    } else {
        deviations = { { 0.5, 0.5 } };
    }

    const unsigned size = deviations.size();

    std::vector<std::vector<Geometry::Vec<3>>>
        pixel_x_cache(image_width, std::vector<Geometry::Vec<3>>(size)),
        pixel_y_cache(image_height, std::vector<Geometry::Vec<3>>(size));

    for (unsigned i = 0; i < size; ++i) {
        for (unsigned pixel_y = 0; pixel_y < image_height; ++pixel_y) {
            pixel_y_cache[pixel_y][i] = (1.0 - (pixel_y + deviations[i][1]) * (2.0 * inv_image_height)) * y_ratio + camera_offset;
        }
        for (unsigned pixel_x = 0; pixel_x < image_width; ++pixel_x) {
            pixel_x_cache[pixel_x][i] = ((pixel_x + deviations[i][0]) * (2.0 * inv_image_width) - 1.0) * x_ratio;
        }
    }

    #pragma omp parallel for schedule(dynamic, 1) collapse(2)
    for (unsigned pixel_y = 0; pixel_y < image_height; ++pixel_y) {
        for (unsigned pixel_x = 0; pixel_x < image_width; ++pixel_x) {
            Pigment::Color accumulated(0.0, 0.0, 0.0);

            if (debug_mode && pixel_x == 0) {
                std::cout << "Line: " << pixel_y << std::endl;
            }

            for (unsigned i = 0; i < size; ++i) {

                const Geometry::Vec<3> position = pixel_x_cache[pixel_x][i] + pixel_y_cache[pixel_y][i];

                accumulated += RayTrace::Trace(
                    Geometry::Line(position, use_orthogonal ? camera_direction.normalized() : (position - eye_pos).normalized()),
                    shapes, ambient, lights,
                    light_deviations, reflect_deviations, transmit_deviations, { 0.5, 0.5, 0.5, 0.0 }, recursion_levels
                );
            }

            img.at<cv::Vec3b>(pixel_y, pixel_x) = static_cast<Pigment::Color>(accumulated / size).intervalFixed();
        }
    }

    cv::imwrite(output_file, img);

    return 0;
}
