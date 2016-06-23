#include <memory>
#include <iostream>
#include "spatial/spatial.h"
#include "ppm.h"

int main (int argc, const char *argv[]) {

    const Spatial::Vec<3>
        eye_pos = { 0.0, 0.0, 5.0 },
        look_at = { 0.0, 0.0, 0.0 },
        up_dir = { 0.0, 1.0, 0.0 };
    const float_max_t fov = 90.0 * Spatial::DEG2RAD;
    const unsigned
        image_width = 800,
        image_height = 600;
    const bool use_projection = true;

    std::vector<std::unique_ptr<Spatial::Shape>> shapes;

    // transf->shear(1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    // transf->rotate( Spatial::Quaternion::axisAngle(Spatial::Vec<3>::axisZ, Spatial::DEG45) );
    // transf->scale( 1.0, 2.0, 1.0 );
    // transf->translate( -1.0, -1.0, -1.0 );

    shapes.push_back(std::make_unique<Spatial::CSGTree>(
        Spatial::CSGTree(new Spatial::Sphere({ 0.0, 0.0, 0.0 }, 1.0), Spatial::CSGTree::SUBTRACTION, new Spatial::Cylinder({ 1.0, 0.0, -2.0 }, { 1.0, 0.0, 2.0 }, 1.0))
    ));
    // shapes.push_back(std::make_unique<Spatial::Box>(Spatial::Box({ 3.0, 3.0, 3.0 }, { 5.0, 5.0, 5.0 })));
    // shapes.push_back(std::make_unique<Spatial::Cylinder>(Spatial::Cylinder({ -1.0, -1.0, -0.0 }, { 1.0, 1.0, 0.0 }, 1.0)));
    // shapes.push_back(std::make_unique<Spatial::Polyhedron>(Spatial::Polyhedron({
    //     Spatial::Plane({ 0.0, 0.0, 1.0 }, 0.0),
    //     Spatial::Plane({ 0.0, 1.0, 0.0 }, 0.0),
    //     Spatial::Plane({ 1.0, 0.0, 0.0 }, 0.0)
    // })));

    // polyhedrons.push_back({
    //     std::make_pair(Spatial::Vec<3>({ 0.0, 0.0, -1.0 }), 0.0),
    //     std::make_pair(Spatial::Vec<3>({ 0.0, 0.0, 0.0 }), 0.0),
    //     std::make_pair(Spatial::Vec<3>({ 1.0, 1.0, 0.0 }), 0.0)
    // });
    //

    const float_max_t
        inv_image_width = 1.0 / static_cast<float_max_t>(image_width),
        inv_image_height = 1.0 / static_cast<float_max_t>(image_height),
        aspect_ratio = static_cast<float_max_t>(image_width) * inv_image_height,
        scale = std::tan(fov * 0.5);
    const Spatial::Vec<3>
        camera_direction = (look_at - eye_pos).normalized(),
        camera_right = camera_direction.cross(up_dir),
        camera_up = camera_right.cross(camera_direction);

    std::vector<float_max_t> pixel_x_cache(image_width);

    PPM img(image_width, image_height, PPM::Pixel(0.0, 0.0, 0.0));

    for (unsigned pixel_y = 0; pixel_y < image_height; ++pixel_y) {
        const float_max_t world_y = (1.0 - static_cast<float_max_t>((pixel_y << 1) + 1) * inv_image_height) * scale;
        for (unsigned pixel_x = 0; pixel_x < image_width; ++pixel_x) {

            if (pixel_y == 0) {
                pixel_x_cache[pixel_x] = (static_cast<float_max_t>((pixel_x << 1) + 1) * inv_image_width - 1.0) * aspect_ratio * scale;
            }

            const Spatial::Vec<3> position =
                pixel_x_cache[pixel_x] * camera_right + world_y * camera_up + eye_pos + camera_direction;

            const Spatial::Line line(position, use_projection ? (position - eye_pos).normalized() : camera_direction);

            for (const auto &shape : shapes) {

                float_max_t t_min, t_max;
                Spatial::Vec<3> normal_min, normal_max;

                if (shape->intersectLine(line, normal_min, normal_max, t_min, t_max, true)) {
                    if (t_min > 0.0) {
                        img.setColor({ pixel_x, pixel_y }, PPM::Pixel(t_min, t_min, t_min));
                    } else if (t_max > 0.0) {
                        img.setColor({ pixel_x, pixel_y }, PPM::Pixel(t_max, t_max, t_max));
                    }
                }
            }
        }
    }

    img.normalize();
    img.write(argv[1]);

    return 0;
}
