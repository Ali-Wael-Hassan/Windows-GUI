#pragma once
#include <Windows.h>

namespace custom {

    struct alignas(64) Color {
        union {
            struct { uint8_t r, g, b, a; };
            uint32_t raw;
        };

        Color() : raw(0) { a = 255; }

        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            : r(r), g(g), b(b), a(a) {}

        inline COLORREF toNative() const { 
            return (COLORREF)(r | (g << 8) | (b << 16)); 
        }
    };

    struct alignas(64) Paint {
        Color color1;
        Color color2;

        uint8_t isGradient : 1;
        uint8_t isVertical : 1;
        uint8_t padding    : 6; 

        Paint() : isGradient(0), isVertical(1), padding(0) {}

        Paint(Color solid) 
            : color1(solid), color2(solid), isGradient(0), isVertical(1), padding(0) {}

        Paint(Color start, Color end, bool vertical = true) 
            : color1(start), color2(end), isGradient(1), isVertical(vertical ? 1 : 0), padding(0) {}
    };
}