#include <windows.h>
#include "core/Window.h"
#include "core/Graphic.h"
#include "util/console_ink.h"
#include "util/logger.h"
#include <thread>
#include <atomic>

std::atomic<bool> running = true;

void GraphicsThread(Window* win) {
    auto& gfx = custom::Graphic::getInstance();
    gfx.setTarget(win->getHWND());

    while (running) {
        gfx.clear(RGB(20, 22, 25));

        int btnX = 300, btnY = 250, btnW = 200, btnH = 60;
        int mouseX = win->getMouseX();
        int mouseY = win->getMouseY();

        bool isHovered = (mouseX >= btnX && mouseX <= btnX + btnW &&
                          mouseY >= btnY && mouseY <= btnY + btnH);
        
        COLORREF btnColor = RGB(60, 60, 65);
        int glowSize = 0;
        COLORREF glowColor = RGB(0, 255, 255);

        if (isHovered) {
            glowSize = 15;
            if (win->isLeftPressed()) {
                btnColor = RGB(0, 120, 215);
                glowColor = RGB(255, 255, 255);
            } else {
                btnColor = RGB(80, 80, 85);
            }
        }

        gfx.draw_rect(btnX, btnY, btnW, btnH, btnColor, 20, glowSize, glowColor);

        gfx.draw_text(btnX + 65, btnY + 20, "CLICK ME", RGB(255, 255, 255));

        gfx.draw_circle(mouseX, mouseY, 5, RGB(255, 255, 255), false);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#if _DEBUG
    auto consoleSink = std::make_unique<custom::console_ink>(); 
    custom::logger::getInstance().add_sink(std::move(consoleSink));
    custom::logger::getInstance().log("Graphics Demo Started.");
#endif

    Window myWindow(800, 600);

    std::thread renderThread(GraphicsThread, &myWindow);

    while (myWindow.process_messages()) {
        
    }

    running = false;
    renderThread.join();
    
    return 0;
}