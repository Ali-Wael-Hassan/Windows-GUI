#include <windows.h>
#include "core/Window.h"
#include "core/Graphic.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <timeapi.h>

#pragma comment(lib, "Winmm.lib")

std::atomic<bool> running = true;

void GraphicsThread(Window* win) {
    auto& gfx = custom::Graphic::getInstance();
    
    while (running && !win->getHWND()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    gfx.setTarget(win->getHWND());
    gfx.setBG(custom::Paint(custom::Color(20, 22, 25)));

    float avgFps = 0.0f;
    const float alpha = 0.05f;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float> frameDuration = frameStart - lastFrameTime;
        lastFrameTime = frameStart;

        float instantFps = 1.0f / frameDuration.count();
        
        if (avgFps == 0) avgFps = instantFps;
        else avgFps = (instantFps * alpha) + (avgFps * (1.0f - alpha));

        gfx.beginFrame();
        
        int btnX = 300, btnY = 250, btnW = 200, btnH = 60;
        int mouseX = win->getMouseX();
        int mouseY = win->getMouseY();

        bool isHovered = (mouseX >= btnX && mouseX <= btnX + btnW &&
                          mouseY >= btnY && mouseY <= btnY + btnH);
        
        custom::Paint btnPaint = custom::Color(60, 60, 65); 
        custom::Color textColor = custom::Color(200, 200, 200);

        if (isHovered) {
            if (win->isLeftPressed()) {
                btnPaint = custom::Paint(custom::Color(0, 120, 215), custom::Color(0, 80, 150), true);
                textColor = custom::Color(255, 255, 255);
            } else {
                btnPaint = custom::Paint(custom::Color(80, 80, 85), custom::Color(40, 40, 45), true);
                textColor = custom::Color(255, 255, 255);
            }
        }

        gfx.draw_rect(btnX, btnY, btnW, btnH, btnPaint, 15);
        gfx.draw_text(btnX + 65, btnY + 20, "CLICK ME", textColor);

        // Display the Smoothed Average
        std::string fpsStr = "FPS: " + std::to_string((int)avgFps);
        gfx.draw_text(10, 10, fpsStr.c_str(), custom::Color(0, 255, 0));

        gfx.draw_ellipse(mouseX - 3, mouseY - 3, 6, 6, custom::Paint(custom::Color(255, 255, 255)), true);
        
        gfx.endFrame();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto workTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
        if (workTime.count() < 1000) {
            std::this_thread::yield(); 
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    timeBeginPeriod(1);

    Window myWindow(800, 600);
    std::thread renderThread(GraphicsThread, &myWindow);

    while (myWindow.process_messages()) {
        std::this_thread::yield();
    }

    running = false;
    if (renderThread.joinable()) renderThread.join();
    
    timeEndPeriod(1);
    return 0;
}