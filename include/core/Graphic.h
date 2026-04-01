#pragma once
#include <Windows.h>
#include <string>
#include <mutex>

namespace custom {
    class Graphic {
    public:
        static Graphic& getInstance();
        void setTarget(HWND hwnd) { m_hwnd = hwnd; }

        void clear(COLORREF color);
        
        void draw_rect(int x, int y, int width, int height, COLORREF color, 
                       int radius = 0, int glowSize = 0, COLORREF glowColor = RGB(255, 255, 255));
        
        void draw_circle(int x, int y, int radius, COLORREF color, bool fill = true);
        void draw_line(int x1, int y1, int x2, int y2, int thickness, COLORREF color);
        void draw_text(int x, int y, const std::string &text, COLORREF color);

    private:
        Graphic() = default;
        HWND m_hwnd = NULL;
        std::mutex m_gfxMutex;
    };
}