#include "core/Graphic.h"

namespace custom {

    Graphic& Graphic::getInstance() {
        static Graphic instance;
        return instance;
    }

    void Graphic::clear(Color color) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        HBRUSH brush = CreateSolidBrush(color.toNative());
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_rect(int x, int y, int width, int height, const Paint& paint, int radius, int glowSize, const Paint& glowPaint) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);

        if (glowSize > 0) {
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            for (int i = 1; i <= glowSize; ++i) {
                Color currentGlow = glowPaint.color1;
                if (glowPaint.isGradient) {
                    float factor = (float)i / (float)glowSize;
                    currentGlow.r = (unsigned char)(glowPaint.color1.r + (glowPaint.color2.r - glowPaint.color1.r) * factor);
                    currentGlow.g = (unsigned char)(glowPaint.color1.g + (glowPaint.color2.g - glowPaint.color1.g) * factor);
                    currentGlow.b = (unsigned char)(glowPaint.color1.b + (glowPaint.color2.b - glowPaint.color1.b) * factor);
                }
                HPEN glowPen = CreatePen(PS_SOLID, 1, currentGlow.toNative());
                HGDIOBJ oldPen = SelectObject(hdc, glowPen);
                RoundRect(hdc, x - i, y - i, x + width + i, y + height + i, radius + (i * 2), radius + (i * 2));
                SelectObject(hdc, oldPen);
                DeleteObject(glowPen);
            }
            SelectObject(hdc, oldBrush);
        }

        if (paint.isGradient) {
            TRIVERTEX v[2];
            v[0] = { x, y, (COLOR16)(paint.color1.r << 8), (COLOR16)(paint.color1.g << 8), (COLOR16)(paint.color1.b << 8), 0 };
            v[1] = { x + width, y + height, (COLOR16)(paint.color2.r << 8), (COLOR16)(paint.color2.g << 8), (COLOR16)(paint.color2.b << 8), 0 };
            GRADIENT_RECT gRect = { 0, 1 };
            GradientFill(hdc, v, 2, &gRect, 1, paint.isVertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
        } else {
            HBRUSH brush = CreateSolidBrush(paint.color1.toNative());
            HPEN pen = CreatePen(PS_SOLID, 1, paint.color1.toNative());
            SelectObject(hdc, brush);
            SelectObject(hdc, pen);
            if (radius > 0) RoundRect(hdc, x, y, x + width, y + height, radius, radius);
            else {
                RECT r = { x, y, x + width, y + height };
                FillRect(hdc, &r, brush);
            }
            DeleteObject(brush);
            DeleteObject(pen);
        }
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_circle(int x, int y, int radius, const Paint& paint, int glowSize, const Paint& glowPaint, bool fill) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);

        if (glowSize > 0) {
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            for (int i = 1; i <= glowSize; ++i) {
                Color currentGlow = glowPaint.color1;
                if (glowPaint.isGradient) {
                    float factor = (float)i / (float)glowSize;
                    currentGlow.r = (unsigned char)(glowPaint.color1.r + (glowPaint.color2.r - glowPaint.color1.r) * factor);
                    currentGlow.g = (unsigned char)(glowPaint.color1.g + (glowPaint.color2.g - glowPaint.color1.g) * factor);
                    currentGlow.b = (unsigned char)(glowPaint.color1.b + (glowPaint.color2.b - glowPaint.color1.b) * factor);
                }
                HPEN glowPen = CreatePen(PS_SOLID, 1, currentGlow.toNative());
                HGDIOBJ oldPen = SelectObject(hdc, glowPen);
                Ellipse(hdc, x - radius - i, y - radius - i, x + radius + i, y + radius + i);
                SelectObject(hdc, oldPen);
                DeleteObject(glowPen);
            }
            SelectObject(hdc, oldBrush);
        }

        HBRUSH brush = fill ? CreateSolidBrush(paint.color1.toNative()) : (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN pen = CreatePen(PS_SOLID, 1, paint.color1.toNative());
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);
        Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
        
        if (fill) DeleteObject(brush);
        DeleteObject(pen);
        ReleaseDC(m_hwnd, hdc);
    }

    void Graphic::draw_line(int x1, int y1, int x2, int y2, int thickness, const Paint& paint, int glowSize, const Paint& glowPaint) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);

        if (glowSize > 0) {
            for (int i = 1; i <= glowSize; ++i) {
                Color currentGlow = glowPaint.color1;
                if (glowPaint.isGradient) {
                    float factor = (float)i / (float)glowSize;
                    currentGlow.r = (unsigned char)(glowPaint.color1.r + (glowPaint.color2.r - glowPaint.color1.r) * factor);
                    currentGlow.g = (unsigned char)(glowPaint.color1.g + (glowPaint.color2.g - glowPaint.color1.g) * factor);
                    currentGlow.b = (unsigned char)(glowPaint.color1.b + (glowPaint.color2.b - glowPaint.color1.b) * factor);
                }
                HPEN glowPen = CreatePen(PS_SOLID, thickness + (i * 2), currentGlow.toNative());
                HGDIOBJ oldPen = SelectObject(hdc, glowPen);
                MoveToEx(hdc, x1, y1, NULL);
                LineTo(hdc, x2, y2);
                SelectObject(hdc, oldPen);
                DeleteObject(glowPen);
            }
        }

        HPEN pen = CreatePen(PS_SOLID, thickness, paint.color1.toNative());
        SelectObject(hdc, pen);
        MoveToEx(hdc, x1, y1, NULL);
        LineTo(hdc, x2, y2);
        DeleteObject(pen);
        ReleaseDC(m_hwnd, hdc);
    }

    // --- TEXT ---
    void Graphic::draw_text(int x, int y, const std::string& text, Color color, int glowSize, Color glowColor) {
        if (!m_hwnd) return;
        std::lock_guard<std::mutex> lock(m_gfxMutex);
        HDC hdc = GetDC(m_hwnd);
        SetBkMode(hdc, TRANSPARENT);

        if (glowSize > 0) {
            SetTextColor(hdc, glowColor.toNative());
            for (int i = 1; i <= glowSize; ++i) {
                TextOutA(hdc, x - i, y, text.c_str(), (int)text.length());
                TextOutA(hdc, x + i, y, text.c_str(), (int)text.length());
                TextOutA(hdc, x, y - i, text.c_str(), (int)text.length());
                TextOutA(hdc, x, y + i, text.c_str(), (int)text.length());
            }
        }

        SetTextColor(hdc, color.toNative());
        TextOutA(hdc, x, y, text.c_str(), (int)text.length());
        ReleaseDC(m_hwnd, hdc);
    }
}