#pragma once

#include <Windows.h>
#include <atomic>

class Window {
private:
    HWND m_hwnd;
    HINSTANCE m_hInstance;
    size_t m_width, m_height;

    std::atomic<int> m_mouseX{ 0 };
    std::atomic<int> m_mouseY{ 0 };
    std::atomic<bool> m_isLeftPressed{ false };

public:
    Window(size_t w, size_t h);
    ~Window();

    void setWidth(size_t w);
    size_t getWidth() const;
    void setHeight(size_t h);
    size_t getHeight() const;

    HWND getHWND() const;

    int getMouseX() const { return m_mouseX.load(); }
    int getMouseY() const { return m_mouseY.load(); }
    bool isLeftPressed() const { return m_isLeftPressed.load(); }

    bool process_messages();
    void draw_pixel(int x, int y, COLORREF color);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};