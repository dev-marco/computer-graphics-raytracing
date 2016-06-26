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
        Pigment::Color color(0.0, 0.0, 0.0, 1.0);
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

        Pigment::Color avg_checker, checker_color_1(0.0, 0.0, 0.0, 1.0), checker_color_2(0.0, 0.0, 0.0, 1.0);
        float_max_t checker_size;

        input >> checker_color_1[0] >> checker_color_1[1] >> checker_color_1[2]
              >> checker_color_2[0] >> checker_color_2[1] >> checker_color_2[2]
              >> checker_size;

        avg_checker = Pigment::Color((checker_color_1 + checker_color_2) * 0.5);

        return new Pigment::Procedural([ checker_color_1, checker_color_2, avg_checker ] (const Geometry::Vec<2> &param) {

            const float_max_t
                s = Geometry::fract(param[0]),
                t = Geometry::fract(param[1]);

            if (Geometry::closeTo(s, 0.5) || Geometry::closeTo(t, 0.5)) {
                return avg_checker;
            }

            const bool right = std::round(s) > 0.5, top = std::round(t) > 0.5;

            if ((top && right) || !(top || right)) {
                return checker_color_1;
            }

            return checker_color_2;

        }, checker_size, checker_size);
    }

    Pigment::TexMap<Pigment::Bitmap> *makeTexMapBitmap (std::istream &input) {
        std::string bitmap;
        Geometry::Vec<4> P0, P1;

        input >> bitmap >> P0 >> P1;

        return new Pigment::TexMap<Pigment::Bitmap>(P0, P1, bitmap);
    }

    void readPigments (
        std::istream &input,
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

            } else if (pigment_type == "texmap") {

                pigments[i] = makeTexMapBitmap(input);

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

    void readTransform (std::istream &input, Shape::TransformedShape *shape) {

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

    Shape::TransformedShape *readTransformedShape (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    ) {

        Geometry::Vec<3> pivot;
        unsigned num_transforms;
        Shape::TransformedShape *transformed_shape;

        input >> pivot >> num_transforms;

        transformed_shape = new Shape::TransformedShape(nullptr, pivot);

        for (unsigned i = 0; i < num_transforms; ++i) {
            readTransform(input, transformed_shape);
        }

        transformed_shape->setShape(readShape(input, shapes, pigments, surfaces));

        return transformed_shape;
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

            return readSphere(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "polyhedron") {

            return readPolyhedron(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "cylinder") {

            return readCylinder(input, pigments[pigment], surfaces[surface]);

        } else if (shape_type == "csg_tree") {

            return readCSGTree(input, shapes, pigments, surfaces);

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
            readPigments(input, pigments);
            readSurfaces(input, surfaces);
            readShapes(input, shapes, pigments, surfaces);

            input.close();
            return true;
        }
        return false;
    }

};
