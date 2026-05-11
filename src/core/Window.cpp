#include "core/Window.h"
#include "util/logger.h"
#include <windowsx.h>
#include <string>

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_MOUSEMOVE:
        if (pWindow) {
            pWindow->m_mouseX = GET_X_LPARAM(lParam);
            pWindow->m_mouseY = GET_Y_LPARAM(lParam);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (pWindow) {
            pWindow->m_isLeftPressed = true;
            custom::logger::getInstance().log("Click at: " + std::to_string(pWindow->m_mouseX));
        }
        return 0;

    case WM_LBUTTONUP:
        if (pWindow) pWindow->m_isLeftPressed = false;
        return 0;

    case WM_SIZE:
        if (pWindow) {
            pWindow->setWidth(LOWORD(lParam));
            pWindow->setHeight(HIWORD(lParam));
            pWindow->m_resized = true;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Window::Window(size_t w, size_t h) : m_width(w), m_height(h), m_hInstance(GetModuleHandle(NULL)) {
    const wchar_t* CLASS_NAME = L"CustomWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(
        0, CLASS_NAME, L"C++ Engine Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        static_cast<int>(m_width), static_cast<int>(m_height),
        NULL, NULL, m_hInstance, NULL
    );

    if (m_hwnd) {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        #if _DEBUG
        custom::logger::getInstance().log("Window created and linked.");
        #endif
        ShowWindow(m_hwnd, SW_SHOW);
    }
}

bool Window::process_messages() {
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void Window::setWidth(size_t w) { m_width = w; }
size_t Window::getWidth() const { return m_width; }
void Window::setHeight(size_t h) { m_height = h; }
size_t Window::getHeight() const { return m_height; }

HWND Window::getHWND() const { return m_hwnd; }

void Window::draw_pixel(int x, int y, COLORREF color) {
    if (!m_hwnd) return;
    HDC hdc = GetDC(m_hwnd);
    if (hdc) {
        SetPixel(hdc, x, y, color);
        ReleaseDC(m_hwnd, hdc);
    }
}

Window::~Window() {
    if (m_hwnd) DestroyWindow(m_hwnd);
    UnregisterClass(L"CustomWindowClass", m_hInstance);
}