#pragma once
#include <Windows.h>
#include <string>
#include <mutex>
#include "core/Color.h"

namespace custom {

    class Graphic {
    public:
        static Graphic& getInstance();
        
        void setTarget(HWND hwnd) { m_hwnd = hwnd; }

        void clear(Color color);

        void draw_rect(int x, int y, int width, int height, 
                       const Paint& paint, 
                       int radius = 0, 
                       int glowSize = 0, 
                       const Paint& glowPaint = Color(255, 255, 255));

        void draw_circle(int x, int y, int radius, 
                         const Paint& paint, 
                         int glowSize = 0, 
                         const Paint& glowPaint = Color(255, 255, 255), 
                         bool fill = true);

        void draw_line(int x1, int y1, int x2, int y2, 
                       int thickness, 
                       const Paint& paint, 
                       int glowSize = 0, 
                       const Paint& glowPaint = Color(255, 255, 255));

        void draw_text(int x, int y, 
                       const std::string& text, 
                       Color color, 
                       int glowSize = 0, 
                       Color glowColor = Color(255, 255, 255));

    private:
        Graphic() = default;
        HWND m_hwnd = NULL;
        std::mutex m_gfxMutex;
    };
}