#include "core/Graphic.h"
#include <vector>

namespace custom {

    Graphic& Graphic::getInstance() {
        static Graphic instance;
        return instance;
    }

    void Graphic::clear(COLORREF color) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &clientRect, brush);
        DeleteObject(brush);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_rect(int x, int y, int width, int height, COLORREF color, int radius, int glowSize, COLORREF glowColor) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);

        if (glowSize > 0) {
            for (int i = 1; i <= glowSize; ++i) {
                HPEN glowPen = CreatePen(PS_SOLID, 1, glowColor);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                SelectObject(hdc, glowPen);
                RoundRect(hdc, x - i, y - i, x + width + i, y + height + i, radius + (i * 2), radius + (i * 2));
                DeleteObject(glowPen);
            }
        }

        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);

        if (radius > 0) RoundRect(hdc, x, y, x + width, y + height, radius, radius);
        else {
            RECT rect = { x, y, x + width, y + height };
            FillRect(hdc, &rect, brush);
        }

        DeleteObject(brush);
        DeleteObject(pen);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_circle(int x, int y, int radius, COLORREF color, bool fill) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        HBRUSH brush = fill ? CreateSolidBrush(color) : (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);
        Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
        if (fill) DeleteObject(brush);
        DeleteObject(pen);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_line(int x1, int y1, int x2, int y2, int thickness, COLORREF color) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        HPEN pen = CreatePen(PS_SOLID, thickness, color);
        SelectObject(hdc, pen);
        MoveToEx(hdc, x1, y1, NULL);
        LineTo(hdc, x2, y2);
        DeleteObject(pen);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_text(int x, int y, const std::string& text, COLORREF color) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        TextOutA(hdc, x, y, text.c_str(), static_cast<int>(text.length()));
        ReleaseDC(m_hwnd, hdc);
    }
}