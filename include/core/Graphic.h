#pragma once
#include <Windows.h>
#include "RenderCommand.h"

namespace custom {

    const size_t MAX_COMMANDS = 4096;
    const size_t ARENA_SIZE   = 1024 * 64;
    const size_t MAX_CACHE    = 64;

    struct CachedBrush { COLORREF color; HBRUSH handle; };
    struct CachedPen   { COLORREF color; int thickness; HPEN handle; };

    class Graphic {
    public:
        static Graphic& getInstance();
        ~Graphic();

        // Lifecycle
        void beginFrame();
        void endFrame();
        void setTarget(HWND hwnd) { m_hwnd = hwnd; }
        void clear(const Paint& paint);
        void setBG(const Paint& paint) { m_bgPaint = paint;}

        // API - Shapes
        void draw_rect(int x, int y, int w, int h, const Paint& paint, int radius = 0);
        void draw_ellipse(int x, int y, int w, int h, const Paint& paint, bool fill = true);
        void draw_line(int x1, int y1, int x2, int y2, int thickness, const Paint& paint);
        void draw_text(int x, int y, const char* text, Color color);

        // API - Cache Helpers
        HBRUSH getCachedBrush(COLORREF color);
        HPEN   getCachedPen(COLORREF color, int thickness);

    private:
        // for Singletons
        Graphic(); 
        Graphic(const Graphic&) = delete;
        Graphic& operator=(const Graphic&) = delete;

        void clearCache();

        template<typename T> T* allocateData(const T& source);
        char* allocateString(const char* text);

        void addDirtyRect(int x, int y, int w, int h);
        
        // for updating only certain parts
        Paint m_bgPaint;
        RECT m_dirtyRect;
        RECT m_lastDirtyRect;

        // Memory Buffers
        RenderCommand alignas(64) m_commands[MAX_COMMANDS];
        size_t m_commandCount = 0;

        uint8_t alignas(64) m_dataArena[ARENA_SIZE];
        size_t m_arenaOffset = 0;

        // Fast Linear Caches
        CachedBrush alignas(64) m_brushCache[MAX_CACHE];
        size_t m_brushCacheCount = 0;

        CachedPen alignas(64) m_penCache[MAX_CACHE];
        size_t m_penCacheCount = 0;

        // GDI Core
        HWND m_hwnd = NULL;
        HDC m_memDC = NULL;
        HBITMAP m_memBitmap = NULL, m_oldBitmap = NULL;
        int m_width = 0, m_height = 0;
    };
}