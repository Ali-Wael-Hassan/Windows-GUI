#pragma once
#include <Windows.h>
#include "core/RenderCommand.h"
#include "core/LinearCache.h"
#include "core/GlobalCacheAllocator.h"
#include <utility>

namespace custom {

    const size_t MAX_COMMANDS = 4096;
    const size_t ARENA_SIZE   = 1024 * 64;
    const size_t MAX_CACHE    = 64;

    struct PenKey {
        COLORREF color;
        int thickness;

        bool operator==(const PenKey& other) const {
            return color == other.color && thickness == other.thickness;
        }
    };

    class Graphic {
    public:
        static Graphic& getInstance();
        ~Graphic();

        // Lifecycle
        void beginFrame();
        void endFrame();
        void setTarget(HWND hwnd) { m_hwnd = hwnd; }
        void clear(const Paint& paint);
        void setBG(const Paint& paint) { m_bgPaint = paint; }

        // API - Shapes
        void draw_rect(int x, int y, int w, int h, const Paint& paint, int radius = 0);
        void draw_ellipse(int x, int y, int w, int h, const Paint& paint, bool fill = true);
        void draw_line(int x1, int y1, int x2, int y2, int thickness, const Paint& paint);
        void draw_text(int x, int y, const char* text, Color color);

        // API - Cache Helpers
        HBRUSH getCachedBrush(COLORREF color);
        HPEN   getCachedPen(COLORREF color, int thickness);

    private:
        // Singleton
        Graphic();
        Graphic(const Graphic&) = delete;
        Graphic& operator=(const Graphic&) = delete;

        void clearCache();

        template<typename T> T* allocateData(const T& source);
        char* allocateString(const char* text);

        void addDirtyRect(int x, int y, int w, int h);

        // Frame tracking
        uint8_t m_frameCounter = 0;

        // Dirty regions
        Paint m_bgPaint;
        RECT m_dirtyRect;
        RECT m_lastDirtyRect;

        // Memory Buffers
        alignas(64) RenderCommand m_commands[MAX_COMMANDS];
        size_t m_commandCount = 0;

        alignas(64) uint8_t m_dataArena[ARENA_SIZE];
        size_t m_arenaOffset = 0;

        // Fast Linear Caches using global allocator
        LinearCache<COLORREF, HBRUSH, MAX_CACHE, 120> m_brushCache;
        LinearCache<PenKey, HPEN, MAX_CACHE, 120>    m_penCache;

        // GDI Core
        HWND m_hwnd = NULL;
        HDC m_memDC = NULL;
        HBITMAP m_memBitmap = NULL;
        HBITMAP m_oldBitmap = NULL;
        int m_width = 0;
        int m_height = 0;
    };
}