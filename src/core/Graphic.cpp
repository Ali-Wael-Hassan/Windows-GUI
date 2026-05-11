#include "core/Graphic.h"
#include <algorithm>

namespace custom {

    static void deleteBrush(HBRUSH b) { DeleteObject(b); }
    static void deletePen(HPEN p) { DeleteObject(p); }

    Graphic::Graphic()
        : m_commandCount(0), m_arenaOffset(0),
        m_width(0), m_height(0),
        m_frameCounter(0),
        m_brushCache(MAX_CACHE * (sizeof(COLORREF) + sizeof(HBRUSH) + 1), deleteBrush),
        m_penCache(MAX_CACHE * (sizeof(PenKey) + sizeof(HPEN) + 1), deletePen)
    {
    }

    Graphic& Graphic::getInstance() {
        static Graphic instance;
        return instance;
    }

    Graphic::~Graphic() {
        clearCache();
        if (m_memDC) {
            SelectObject(m_memDC, m_oldBitmap);
            DeleteObject(m_memBitmap);
            DeleteDC(m_memDC);
        }
    }

    void Graphic::beginFrame() {
        m_commandCount = 0;
        m_arenaOffset = 0;
        m_frameCounter++;

        if (m_memDC && m_lastDirtyRect.left < m_lastDirtyRect.right) {
            int lx = m_lastDirtyRect.left;
            int ly = m_lastDirtyRect.top;
            int lw = m_lastDirtyRect.right - m_lastDirtyRect.left;
            int lh = m_lastDirtyRect.bottom - m_lastDirtyRect.top;

            HBRUSH bgBrush = getCachedBrush(m_bgPaint.color1.toNative());
            RECT r = { lx, ly, lx + lw, ly + lh };
            FillRect(m_memDC, &r, bgBrush);
        }

        m_dirtyRect.left = 32767; 
        m_dirtyRect.top = 32767;
        m_dirtyRect.right = -32767;
        m_dirtyRect.bottom = -32767;

        if (!m_hwnd) return;

        RECT r; GetClientRect(m_hwnd, &r);
        int w = r.right - r.left, h = r.bottom - r.top;

        if (!m_memDC || w != m_width || h != m_height) {
            if (m_memDC) { 
                SelectObject(m_memDC, m_oldBitmap); 
                DeleteObject(m_memBitmap); 
                DeleteDC(m_memDC); 
            }
            HDC hdc = GetDC(m_hwnd);
            m_width = w; m_height = h;
            m_memDC = CreateCompatibleDC(hdc);
            m_memBitmap = CreateCompatibleBitmap(hdc, w, h);
            m_oldBitmap = (HBITMAP)SelectObject(m_memDC, m_memBitmap);
            ReleaseDC(m_hwnd, hdc);
            clear(m_bgPaint);
        }
    }

    void Graphic::addDirtyRect(int x, int y, int w, int h) {
        m_dirtyRect.left   = (std::min)((long)m_dirtyRect.left, (long)x - 2);
        m_dirtyRect.top    = (std::min)((long)m_dirtyRect.top, (long)y - 2);
        m_dirtyRect.right  = (std::max)((long)m_dirtyRect.right, (long)x + w + 2);
        m_dirtyRect.bottom = (std::max)((long)m_dirtyRect.bottom, (long)y + h + 2);
    }

    void Graphic::endFrame() {
        if (!m_memDC || m_commandCount == 0) return;

        int combinedLeft = (std::min)((int)m_dirtyRect.left, (int)m_lastDirtyRect.left);
        int combinedTop  = (std::min)((int)m_dirtyRect.top, (int)m_lastDirtyRect.top);
        int combinedRight = (std::max)((int)m_dirtyRect.right, (int)m_lastDirtyRect.right);
        int combinedBottom = (std::max)((int)m_dirtyRect.bottom, (int)m_lastDirtyRect.bottom);

        int dx = (std::max)(0, combinedLeft);
        int dy = (std::max)(0, combinedTop);
        int dw = (std::min)(m_width - dx, combinedRight - combinedLeft);
        int dh = (std::min)(m_height - dy, combinedBottom - combinedTop);

        if (dw > 0 && dh > 0) {
            for (size_t i = 0; i < m_commandCount; ++i) {
                m_commands[i].execute(m_memDC);
            }

            HDC hdc = GetDC(m_hwnd);
            BitBlt(hdc, dx, dy, dw, dh, m_memDC, dx, dy, SRCCOPY);
            ReleaseDC(m_hwnd, hdc);
        }

        m_lastDirtyRect = m_dirtyRect;
    }

    void Graphic::clear(const Paint& paint) {
        if (m_commandCount >= MAX_COMMANDS) return;

        addDirtyRect(0, 0, m_width, m_height);

        RenderCommand& cmd = m_commands[m_commandCount++];
        cmd.type = (uint8_t)CommandType::Rect;
        cmd.x = 0; cmd.y = 0; 
        cmd.w = (int16_t)m_width; 
        cmd.h = (int16_t)m_height;
        
        cmd.paint = paint;
        cmd.isGradient = paint.isGradient;
        cmd.isVertical = paint.isVertical;
        cmd.hBrush = getCachedBrush(paint.color1.toNative());

        cmd.renderer = [](HDC hdc, const RenderCommand& c) {
            if (c.isGradient) {
                TRIVERTEX v[2] = { 
                    { c.x, c.y, (COLOR16)(c.paint.color1.r << 8), (COLOR16)(c.paint.color1.g << 8), (COLOR16)(c.paint.color1.b << 8), 0 },
                    { (int16_t)(c.x + c.w), (int16_t)(c.y + c.h), (COLOR16)(c.paint.color2.r << 8), (COLOR16)(c.paint.color2.g << 8), (COLOR16)(c.paint.color2.b << 8), 0 }
                };
                GRADIENT_RECT g = {0, 1};
                GradientFill(hdc, v, 2, &g, 1, c.isVertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
            } else {
                RECT r = { c.x, c.y, c.x + c.w, c.y + c.h };
                FillRect(hdc, &r, c.hBrush);
            }
        };
    }

    void Graphic::draw_rect(int x, int y, int w, int h, const Paint& paint, int radius) {
        if (m_commandCount >= MAX_COMMANDS) return;
        RenderCommand& cmd = m_commands[m_commandCount++];
        addDirtyRect(x, y, w, h);
        
        cmd.type = (uint8_t)CommandType::Rect;
        cmd.x = (int16_t)x; cmd.y = (int16_t)y; cmd.w = (int16_t)w; cmd.h = (int16_t)h;
        cmd.paint = paint;
        cmd.isGradient = paint.isGradient;
        cmd.isVertical = paint.isVertical;
        cmd.hBrush = getCachedBrush(paint.color1.toNative());

        struct RectData { int radius; };
        cmd.data = allocateData(RectData{ radius });

        cmd.renderer = [](HDC hdc, const RenderCommand& c) {
            auto* d = (RectData*)c.data;
            if (d->radius > 0) {
                HRGN rgn = CreateRoundRectRgn(c.x, c.y, c.x + c.w + 1, c.y + c.h + 1, d->radius, d->radius);
                SelectClipRgn(hdc, rgn);
                if (c.isGradient) {
                    TRIVERTEX v[2] = { 
                        { c.x, c.y, (COLOR16)(c.paint.color1.r << 8), (COLOR16)(c.paint.color1.g << 8), (COLOR16)(c.paint.color1.b << 8), 0 },
                        { (int16_t)(c.x + c.w), (int16_t)(c.y + c.h), (COLOR16)(c.paint.color2.r << 8), (COLOR16)(c.paint.color2.g << 8), (COLOR16)(c.paint.color2.b << 8), 0 }
                    };
                    GRADIENT_RECT g = {0, 1};
                    GradientFill(hdc, v, 2, &g, 1, c.isVertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
                } else {
                    RECT r = { c.x, c.y, c.x + c.w, c.y + c.h };
                    FillRect(hdc, &r, c.hBrush);
                }
                SelectClipRgn(hdc, NULL); DeleteObject(rgn);
            } else {
                RECT r = { c.x, c.y, c.x + c.w, c.y + c.h };
                FillRect(hdc, &r, c.hBrush);
            }
        };
    }

    void Graphic::draw_ellipse(int x, int y, int w, int h, const Paint& paint, bool fill) {
        if (m_commandCount >= MAX_COMMANDS) return;
        RenderCommand& cmd = m_commands[m_commandCount++];
        addDirtyRect(x, y, w, h);

        cmd.type = (uint8_t)CommandType::Circle;
        cmd.x = (int16_t)x; cmd.y = (int16_t)y; cmd.w = (int16_t)w; cmd.h = (int16_t)h;
        cmd.paint = paint;
        cmd.fill = fill;
        cmd.isGradient = paint.isGradient;
        cmd.isVertical = paint.isVertical;
        cmd.hBrush = getCachedBrush(paint.color1.toNative());
        cmd.hPen = getCachedPen(paint.color1.toNative(), 1);

        cmd.renderer = [](HDC hdc, const RenderCommand& c) {
            if (c.fill) {
                if (c.isGradient) {
                    HRGN rgn = CreateEllipticRgn(c.x, c.y, c.x + c.w + 1, c.y + c.h + 1);
                    SelectClipRgn(hdc, rgn);

                    TRIVERTEX v[2] = {
                        { c.x, c.y, (COLOR16)(c.paint.color1.r << 8), (COLOR16)(c.paint.color1.g << 8), (COLOR16)(c.paint.color1.b << 8), 0 },
                        { (int16_t)(c.x + c.w), (int16_t)(c.y + c.h), (COLOR16)(c.paint.color2.r << 8), (COLOR16)(c.paint.color2.g << 8), (COLOR16)(c.paint.color2.b << 8), 0 }
                    };
                    GRADIENT_RECT g = { 0, 1 };
                    GradientFill(hdc, v, 2, &g, 1, c.isVertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);

                    SelectClipRgn(hdc, NULL);
                    DeleteObject(rgn);
                } else {
                    SelectObject(hdc, c.hBrush);
                    SelectObject(hdc, (HPEN)GetStockObject(NULL_PEN));
                    Ellipse(hdc, c.x, c.y, c.x + c.w, c.y + c.h);
                }
            } else {
                // Outline only
                SelectObject(hdc, (HBRUSH)GetStockObject(NULL_BRUSH));
                SelectObject(hdc, c.hPen);
                Ellipse(hdc, c.x, c.y, c.x + c.w, c.y + c.h);
            }
        };
    }

    void Graphic::draw_line(int x1, int y1, int x2, int y2, int thickness, const Paint& paint) {
        if (m_commandCount >= MAX_COMMANDS) return;
        RenderCommand& cmd = m_commands[m_commandCount++];
        int x = (std::min)(x1, x2) - thickness;
        int y = (std::min)(y1, y2) - thickness;
        int w = std::abs(x1 - x2) + (thickness * 2);
        int h = std::abs(y1 - y2) + (thickness * 2);

        addDirtyRect(x, y, w, h);

        cmd.type = (uint8_t)CommandType::Line;
        cmd.x1 = (int16_t)x1; cmd.y1 = (int16_t)y1; 
        cmd.x2 = (int16_t)x2; cmd.y2 = (int16_t)y2;
        cmd.paint = paint;
        cmd.isGradient = paint.isGradient;
        struct LineData { int thick; };
        cmd.data = allocateData(LineData{ thickness });
        cmd.hPen = getCachedPen(paint.color1.toNative(), thickness);

        cmd.renderer = [](HDC hdc, const RenderCommand& c) {
            if (c.isGradient) {
                TRIVERTEX v[4];
                v[0] = { c.x1, c.y1, (COLOR16)(c.paint.color1.r << 8), (COLOR16)(c.paint.color1.g << 8), (COLOR16)(c.paint.color1.b << 8), 0 };
                v[1] = { c.x2, c.y2, (COLOR16)(c.paint.color2.r << 8), (COLOR16)(c.paint.color2.g << 8), (COLOR16)(c.paint.color2.b << 8), 0 };
                
                GRADIENT_TRIANGLE t[1] = { 0, 1, 1 };
                SelectObject(hdc, c.hPen);
                MoveToEx(hdc, c.x1, c.y1, NULL);
                LineTo(hdc, c.x2, c.y2);
            } else {
                SelectObject(hdc, c.hPen);
                MoveToEx(hdc, c.x1, c.y1, NULL);
                LineTo(hdc, c.x2, c.y2);
            }
        };
    }

    void Graphic::draw_text(int x, int y, const char* text, Color color) {
        if (m_commandCount >= MAX_COMMANDS) return;
        RenderCommand& cmd = m_commands[m_commandCount++];
        addDirtyRect(x, y, 500, 30);

        cmd.type = (uint8_t)CommandType::Text;
        cmd.x = (int16_t)x; cmd.y = (int16_t)y;
        cmd.paint.color1 = color;
        cmd.data = allocateString(text);
        cmd.renderer = [](HDC hdc, const RenderCommand& c) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, c.paint.color1.toNative());
            TextOutA(hdc, c.x, c.y, (const char*)c.data, (int)strlen((const char*)c.data));
        };
    }

    HBRUSH Graphic::getCachedBrush(COLORREF color) {
        return m_brushCache.getOrCreate(color, m_frameCounter, [](COLORREF c) {
            return CreateSolidBrush(c);
        });
    }

    HPEN Graphic::getCachedPen(COLORREF color, int thickness) {
        PenKey key{ color, thickness };
        return m_penCache.getOrCreate(key, m_frameCounter, [thickness](const PenKey& k) {
            return CreatePen(PS_SOLID, k.thickness, k.color);
        });
    }

    void Graphic::clearCache() {
        m_brushCache.clear();
        m_penCache.clear();
    }

    template<typename T> T* Graphic::allocateData(const T& source) {
        if (m_arenaOffset + sizeof(T) > ARENA_SIZE) return nullptr;
        T* ptr = (T*)&m_dataArena[m_arenaOffset];
        *ptr = source; m_arenaOffset += sizeof(T);
        return ptr;
    }

    char* Graphic::allocateString(const char* text) {
        size_t len = strlen(text) + 1;
        if (m_arenaOffset + len > ARENA_SIZE) return nullptr;
        char* ptr = (char*)&m_dataArena[m_arenaOffset];
        memcpy(ptr, text, len); m_arenaOffset += len;
        return ptr;
    }
}