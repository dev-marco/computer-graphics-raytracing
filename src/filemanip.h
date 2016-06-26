#ifndef SRC_FILEMANIP_H_
#define SRC_FILEMANIP_H_

#include <algorithm>
#include <string>
#include <fstream>
#include "graphics/graphics.h"

namespace FileManip {

    inline bool not_space (int c) { return !isspace(c); }

    inline std::string &ltrim (std::string &s) { s.erase(s.begin(), find_if(s.begin(), s.end(), not_space)); return s; }
    inline std::string &rtrim (std::string &s) { s.erase(find_if(s.rbegin(), s.rend(), not_space).base(), s.end()); return s; }
    inline std::string &trim (std::string &s) { return ltrim(rtrim(s)); }

    std::string nextLine (std::istream &in, bool &ok, const char comment = '#');

    inline Geometry::Camera readCamera (std::istream &input) {
        Geometry::Vec<3> position, look_at, up_dir;
        float_max_t fov;
        input >> position >> look_at >> up_dir >> fov;
        return Geometry::Camera(position, look_at, up_dir, fov * Geometry::DEG2RAD);
    }

    void readLights (std::istream &input, Pigment::Color &ambient, std::vector<Light::Light *> &lights);

    void readPigments (std::istream &input, std::vector<Pigment::Texture *> &pigments);
    Pigment::Solid *makeSolid (std::istream &input);
    Pigment::Procedural *makeChecker (std::istream &input);
    Pigment::TexMap<Pigment::Bitmap> *makeTexMapBitmap (std::istream &input);

    void readSurfaces (std::istream &input, std::vector<Light::Surface *> &surfaces);

    void readShapes (
        std::istream &input,
        std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    );
    Shape::Shape *readShape (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    );
    Shape::Sphere *readSphere (std::istream &input, Pigment::Texture *pigment, Light::Surface *surface);
    Shape::Polyhedron *readPolyhedron (std::istream &input, Pigment::Texture *pigment, Light::Surface *surface);
    Shape::Cylinder *readCylinder (std::istream &input, Pigment::Texture *pigment, Light::Surface *surface);
    Shape::Box *readBox (std::istream &input, Pigment::Texture *pigment, Light::Surface *surface);
    Shape::CSGTree *readCSGTree (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    );

    void readTransform (std::istream &input, Shape::TransformedShape *shape);
    Shape::TransformedShape *readTransformedShape (
        std::istream &input,
        const std::vector<Shape::Shape *> &shapes,
        const std::vector<Pigment::Texture *> &pigments,
        const std::vector<Light::Surface *> &surfaces
    );

    bool readFile (
        const std::string &name,
        Geometry::Camera &camera,
        Pigment::Color &ambient,
        std::vector<Light::Light *> &lights,
        std::vector<Pigment::Texture *> &pigments,
        std::vector<Light::Surface *> &surfaces,
        std::vector<Shape::Shape *> &shapes
    );

}

#endif
