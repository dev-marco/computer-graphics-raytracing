#include "filemanip.h"

namespace FileManip {

    std::string nextLine (std::istream &in, bool &ok, const char comment) {
        std::string line;
        do {
            std::getline(in, line);
            line = line.substr(0, line.find_first_of(comment));
        } while (!trim(line).size() && in.good());

        ok = line.size() > 0;

        return line;
    }

    void readLights (std::istream &input, Pigment::Color &ambient, std::vector<Light::Light *> &lights) {

        unsigned num_lights;
        Geometry::Vec<3> ignore, position;
        Pigment::Color color(0.0, 0.0, 0.0);
        float_max_t constant, linear, quadratic;

        ambient[3] = 1.0;
        input >> num_lights >> ignore >> ambient[0] >> ambient[1] >> ambient[2] >> ignore;

        lights.resize(--num_lights);
        for (unsigned i = 0; i < num_lights; ++i) {
            input >> position >> color[0] >> color[1] >> color[2] >> constant >> linear >> quadratic;
            lights[i] = new Light::Light(position, color, constant, linear, quadratic);
        }
    }

    Pigment::Solid *makeSolid (std::istream &input) {

        Pigment::Color solid_color(0.0, 0.0, 0.0, 1.0);
        input >> solid_color[0] >> solid_color[1] >> solid_color[2];

        return new Pigment::Solid(solid_color);
    }

    Pigment::Procedural *makeChecker (std::istream &input) {

        Pigment::Color avg_checker, checker_color_1(0.0, 0.0, 0.0), checker_color_2(0.0, 0.0, 0.0);
        float_max_t checker_size;

        input >> checker_color_1[0] >> checker_color_1[1] >> checker_color_1[2]
              >> checker_color_2[0] >> checker_color_2[1] >> checker_color_2[2]
              >> checker_size;

        checker_size *= 2.0;

        avg_checker = Pigment::Color((checker_color_1 + checker_color_2) * 0.5);

        return new Pigment::Procedural([ checker_color_1, checker_color_2, avg_checker ] (const Geometry::Vec<2> &param) {

            const float_max_t
                s = Geometry::fract(param[0]),
                t = Geometry::fract(param[1]);

            if (Geometry::closeToZero(s) || Geometry::closeTo(s, 0.5) || Geometry::closeTo(s, 1.0) ||
                Geometry::closeToZero(t) || Geometry::closeTo(t, 0.5) || Geometry::closeTo(t, 1.0)) {
                return avg_checker;
            }

            const bool right = std::round(s) > 0.5, top = std::round(t) > 0.5;

            if ((top && right) || !(top || right)) {
                return checker_color_1;
            }

            return checker_color_2;

        }, checker_size, checker_size);
    }


    // return { (1 + std::sin(param[0])) * 0.5, (1.0 + std::sin(param[0])) * 0.5, (1 + std::sin(param[0])) * 0.5 };
    Pigment::Procedural *makeMoisture (std::istream &input) {

        Pigment::Color
            moisture_color_1(0.0, 0.0, 0.0),
            moisture_color_2(0.0, 0.0, 0.0);

        Pigment::PerlinNoise noise;
        unsigned seed;
        float_max_t moisture_size;

        input >> seed >> moisture_color_1[0] >> moisture_color_1[1] >> moisture_color_1[2]
              >> moisture_color_2[0] >> moisture_color_2[1] >> moisture_color_2[2] >> moisture_size;

        noise.shuffle(seed);

        return new Pigment::Procedural([ moisture_color_1, moisture_color_2, noise ] (const Geometry::Vec<2> &param) mutable -> Pigment::Color {

            float_max_t
                s = std::fmod(param[0], 500.0) + 500.0,
                t = std::fmod(param[1], 500.0) + 500.0,
                value = (1.0 + std::sin((s + noise.at(s * 5.0, t * 5.0, 0.0) * 0.5) * 50.0)) * 0.5;

            return value * moisture_color_1 + (-value + 1.0) * moisture_color_2;

        }, moisture_size, moisture_size);
    }

    Pigment::TexMap<Pigment::Bitmap> *makeTexMapBitmap (std::istream &input, const std::string &texture_dir) {
        std::string bitmap;
        Geometry::Vec<4> P0, P1;

        input >> bitmap >> P0 >> P1;

        return new Pigment::TexMap<Pigment::Bitmap>(P0, P1, texture_dir + bitmap);
    }

    Pigment::Bitmap *makeBitmap (std::istream &input, const std::string &texture_dir) {
        std::string bitmap;
        float_max_t width, height;

        input >> bitmap >> width >> height;

        return new Pigment::Bitmap(texture_dir + bitmap, width, height);
    }

    void readPigments (
        std::istream &input,
        const std::string &texture_dir,
        std::vector<Pigment::Texture *> &pigments
    ) {

        unsigned num_pigments;

        input >> num_pigments;
        pigments.resize(num_pigments);

        for (unsigned i = 0; i < num_pigments; ++i) {

            std::string pigment_type;
            input >> pigment_type;

            if (pigment_type == "solid") {

                pigments[i] = makeSolid(input);

            } else if (pigment_type == "checker") {

                pigments[i] = makeChecker(input);

            } else if (pigment_type == "moisture") {

                pigments[i] = makeMoisture(input);

            } else if (pigment_type == "texmap") {

                pigments[i] = makeTexMapBitmap(input, texture_dir);

            } else if (pigment_type == "bitmap") {

                pigments[i] = makeBitmap(input, texture_dir);

            } else {
                pigments[i] = nullptr;
            }
        }
    }

    void readSurfaces (
        std::istream &input,
        std::vector<Light::Surface *> &surfaces
    ) {

        unsigned num_surfaces;
        float_max_t ambient, diffuse, specular, alpha, reflect, transmit, ior;

        input >> num_surfaces;
        surfaces.resize(num_surfaces);

        for (unsigned i = 0; i < num_surfaces; ++i) {
            input >> ambient >> diffuse >> specular >> alpha >> reflect >> transmit >> ior;
            surfaces[i] = new Light::Surface(
                new Light::Solid<1>(ambient),
                new Light::Solid<1>(diffuse),
                new Light::Solid<1>(specular),
                new Light::Solid<1>(alpha),
                new Light::Solid<1>(reflect),
                new Light::Solid<1>(transmit),
                new Light::Solid<1>(ior),
                new Light::Solid<3>({ 0.0, 0.0, 0.0 })
            );
        }

    }

    Shape::Sphere *readSphere (
        std::istream &input,
        Pigment::Texture *pigment,
        Light::Surface *surface
    ) {

        Geometry::Vec<3> sphere_center;
        float_max_t sphere_radius;

        input >> sphere_center >> sphere_radius;

        return new Shape::Sphere(sphere_center, sphere_radius, pigment, surface);
    }

    Shape::Polyhedron *readPolyhedron (
        std::istream &input,
        Pigment::Texture *pigment,
        Light::Surface *surface
    ) {

        unsigned num_faces;
        Geometry::Vec<3> plane_normal;
        float_max_t plane_d;

        input >> num_faces;

        std::vector<Geometry::Plane> faces(num_faces);

        for (unsigned i = 0; i < num_faces; ++i) {
            input >> plane_normal >> plane_d;
            faces[i] = Geometry::Plane(plane_normal, -plane_d);
        }

        return new Shape::Polyhedron(faces, pigment, surface);
    }

    Shape::Cylinder *readCylinder (
        std::istream &input,
        Pigment::Texture *pigment,
        Light::Surface *surface
    ) {

        Geometry::Vec<3> cylinder_bottom, cylinder_top;
        float_max_t cylinder_radius;

        input >> cylinder_bottom >> cylinder_top >> cylinder_radius;

        return new Shape::Cylinder(cylinder_bottom, cylinder_top, cylinder_radius, pigment, surface);
    }

    Shape::Box *readBox (
        std::istream &input,
        Pigment::Texture *pigment,
        Light::Surface *surface
    ) {

        Geometry::Vec<3> box_min, box_max;

        input >> box_min >> box_max;

        return new Shape::Box(box_min, box_max, pigment, surface);
    }

    Shape::CSGTree *readCSGTree (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        std::string type;
        Shape::CSGTree::Type operation;
        Shape::Shape *shape_first, *shape_second;

        input >> type;

        if (type == "union") {
            operation = Shape::CSGTree::UNION;
        } else if (type == "intersection") {
            operation = Shape::CSGTree::INTERSECTION;
        } else if (type == "subtraction") {
            operation = Shape::CSGTree::SUBTRACTION;
        } else {
            return nullptr;
        }

        shape_first = readShape(input, shapes, pigments, surfaces);
        shape_second = readShape(input, shapes, pigments, surfaces);

        return new Shape::CSGTree(shape_first, operation, shape_second);
    }

    Shape::Shape *nextUnion (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces,
        unsigned size
    ) {
        if (size > 1) {
            return new Shape::CSGTree(readShape(input, shapes, pigments, surfaces), Shape::CSGTree::UNION, nextUnion(input, shapes, pigments, surfaces, size - 1));
        }
        return readShape(input, shapes, pigments, surfaces);
    }

    Shape::Shape *readUnion (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        unsigned size;

        input >> size;

        return nextUnion(input, shapes, pigments, surfaces, size);
    }

    void readTransform (std::istream &input, Shape::Transformed *shape) {

        std::string type;

        input >> type;

        if (type == "translate") {
            Geometry::Vec<3> translation;
            input >> translation;
            shape->translate(translation);
        } else if (type == "rotate") {
            Geometry::Quaternion rotation;
            input >> rotation;
            shape->rotate(rotation);
        } else if (type == "scale") {
            float_max_t sx, sy, sz;
            input >> sx >> sy >> sz;
            shape->scale(sx, sy, sz);
        } else if (type == "shear") {
            float_max_t sxy, sxz, syx, syz, szx, szy;
            input >> sxy >> sxz >> syx >> syz >> szx >> szy;
            shape->shear(sxy, sxz, syx, syz, szx, szy);
        }
    }

    Shape::Transformed *readTransformedShape (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        Geometry::Vec<3> pivot;
        unsigned num_transforms;
        Shape::Transformed *transformed;

        input >> pivot >> num_transforms;

        transformed = new Shape::Transformed(nullptr, pivot);

        for (unsigned i = 0; i < num_transforms; ++i) {
            readTransform(input, transformed);
        }

        transformed->setShape(readShape(input, shapes, pigments, surfaces));

        return transformed;
    }

    Shape::Shape *readShape (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        std::string shape_type;
        unsigned pigment, surface;

        input >> pigment >> surface >> shape_type;

        if (shape_type == "sphere") {

            Shape::Shape *sphe = readSphere(input, pigments[pigment], surfaces[surface]);
            return sphe;

        } else if (shape_type == "polyhedron") {

            return readPolyhedron(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "cylinder") {

            return readCylinder(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "box") {

            return readBox(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "csg_tree") {

            return readCSGTree(input, shapes, pigments, surfaces);

        } else if (shape_type == "union") {

            return readUnion(input, shapes, pigments, surfaces);

        } else if (shape_type == "transform") {

            return readTransformedShape(input, shapes, pigments, surfaces);

        }

        return nullptr;
    }

    void readShapes (
        std::istream &input,
        std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        unsigned num_shapes;

        input >> num_shapes;
        shapes.resize(num_shapes);

        for (unsigned i = 0; i < num_shapes; ++i) {
            shapes[i] = readShape(input, shapes, pigments, surfaces);
        }

    }

    bool readFile (
        const std::string &name,
        const std::string &texture_dir,
        Geometry::Camera &camera,
        Pigment::Color &ambient,
        std::vector<Light::Light *> &lights,
        std::vector<Pigment::Texture *> &pigments,
        std::vector<Light::Surface *> &surfaces,
        std::vector<Shape::Shape *> &shapes
    ) {
        std::ifstream input(name);
        if (input.is_open()) {

            camera = readCamera(input);
            readLights(input, ambient, lights);
            readPigments(input, texture_dir, pigments);
            readSurfaces(input, surfaces);
            readShapes(input, shapes, pigments, surfaces);

            input.close();
            return true;
        }
        return false;
    }

};
