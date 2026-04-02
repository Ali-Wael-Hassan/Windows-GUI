#include <Windows.h>

namespace custom {
    struct Color {
        unsigned char r, g, b, a;
        Color(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 255)
            : r(r), g(g), b(b), a(a) {}
        COLORREF toNative() const { return RGB(r, g, b); }
    };

    struct Paint {
        Color color1;
        Color color2;
        bool isGradient;
        bool isVertical;

        Paint(Color solid) 
            : color1(solid), color2(solid), isGradient(false), isVertical(true) {}

        Paint(Color start, Color end, bool vertical = true) 
            : color1(start), color2(end), isGradient(true), isVertical(vertical) {}
    };
}