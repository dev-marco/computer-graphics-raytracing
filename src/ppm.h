#ifndef SRC_PPM_H_DEFINED_
#define SRC_PPM_H_DEFINED_

#include <array>
#include <fstream>
#include "spatial/spatial.h"

class PPM {

public:

    struct Pixel {
        float_max_t R, G, B;
        inline Pixel (float_max_t red = 0.0, float_max_t green = 0.0, float_max_t blue = 0.0) : R(red), G(green), B(blue) {};
    };

private:

    std::vector<std::vector<Pixel>> image;
    unsigned width, height;

public:

    PPM(unsigned _width, unsigned _height, Pixel color = Pixel(0.0, 0.0, 0.0));

    inline unsigned getHeight (void) const { return this->height; }
    inline unsigned getWidth (void) const { return this->width; }

    void write(const std::string &file);
    void colorPixel(const std::array<unsigned, 2> &position, const Pixel &color);
    void setColor(const std::array<unsigned, 2> &position, const Pixel &color);
    void normalize(void);
};

#endif
