#include "ppm.h"

PPM::PPM (unsigned _width, unsigned _height, Pixel color) : width(_width), height(_height) {
    this->image.resize(_height);
    for (unsigned i = 0; i < _height; ++i) {
        this->image[i].resize(_width);
        std::fill(std::begin(this->image[i]), std::end(this->image[i]), color);
    }
}

void PPM::write (const std::string &file) {
    std::ofstream out(file);
    out << "P3" << std::endl << this->getWidth() << ' ' << this->getHeight() << std::endl << "255" << std::endl;
    for (const auto &line : image) {
        for (const auto &pixel : line) {
            const unsigned
                R = std::min(255.0, std::max(0.0, pixel.R * 255.0)),
                G = std::min(255.0, std::max(0.0, pixel.G * 255.0)),
                B = std::min(255.0, std::max(0.0, pixel.B * 255.0));
            out << R << ' ' << G << ' ' << B << ' ';
        }
        out << std::endl;
    }
    out.close();
}

void PPM::colorPixel (const std::array<unsigned, 2> &position, const Pixel &color) {
    if (
        position[0] >= 0 && position[0] < this->getWidth() &&
        position[1] >= 0 && position[1] < this->getHeight()
    ) {
        this->setColor(position, color);
    }
}

void PPM::setColor (const std::array<unsigned, 2> &position, const Pixel &color) {
    this->image[position[1]][position[0]] = color;
}

void PPM::normalize (void) {
    float_max_t max_val = 0.0;
    for (unsigned y = 0, height = this->getHeight(), width = this->getWidth(); y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            const Pixel &pixel = this->image[y][x];
            if (pixel.R > max_val) {
                max_val = pixel.R;
            }
            if (pixel.G > max_val) {
                max_val = pixel.G;
            }
            if (pixel.B > max_val) {
                max_val = pixel.B;
            }
        }
    }
    if (max_val > 0.0) {
        for (unsigned y = 0, height = this->getHeight(), width = this->getWidth(); y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                Pixel &pixel = this->image[y][x];
                pixel.R /= max_val;
                pixel.G /= max_val;
                pixel.B /= max_val;
            }
        }
    }
}
