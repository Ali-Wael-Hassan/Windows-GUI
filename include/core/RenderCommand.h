#pragma once
#include <Windows.h>
#include <cstdint>
#include "Color.h"

namespace custom {
    struct RenderCommand;

    enum class CommandType : uint8_t { Clear, Rect, Circle, Line, Text };

    typedef void (*RenderFunc)(HDC hdc, const RenderCommand& cmd);

    struct alignas(64) RenderCommand {
        RenderCommand() {
            // Zero out the entire memory block for safety
            memset(this, 0, sizeof(RenderCommand));
        }
        // 32-bit Header
        uint32_t type       : 4;  // Up to 16 command types
        uint32_t isGradient : 1;
        uint32_t isVertical : 1;
        uint32_t fill       : 1;
        uint32_t padding    : 25; // Reserved for future flags

        // 8 bytes shared data
        union {
            struct { int16_t x, y, w, h; };
            struct { int16_t x1, y1, x2, y2; };
        };

        // Additional data and renderer
        void* data;
        RenderFunc renderer;

        // Styling & GDI
        Paint paint;
        HBRUSH hBrush;
        HPEN   hPen;

        // Unified execution call
        inline void execute(HDC hdc) const {
            if (renderer) renderer(hdc, *this);
        }
    };
}