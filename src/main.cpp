#include <windows.h>
#include "core/Window.h"
#include "core/Graphic.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <cmath>
#include <timeapi.h>
#include <vector>
#include <mutex>

// ============================================================================
// Shared State
// ============================================================================

static std::atomic<bool> running{ true };
static std::atomic<HWND> g_hwnd{ NULL };
static Window* g_window = nullptr;
static std::atomic<bool> g_renderDone{ false };

// ============================================================================
// Data Structures
// ============================================================================

/** One bouncing particle spawned on click */
struct Particle {
    float x, y;
    float vx, vy;
    custom::Color color;
    int size;
};

/**
 * @brief Holds runtime state shared between window and render threads.
 *
 * All particle and wheelSpeed access is guarded by sceneMutex.
 * time and angle are only touched by the render thread (no sync needed).
 */
struct DemoScene {
    float time = 0.0f;
    float angle = 0.0f;
    float wheelSpeed = 0.5f;
    std::vector<Particle> particles;
    std::mutex sceneMutex;

    struct Button {
        int x, y, w, h;
        const char* text;
    };

    Button buttons[3] = {
        { 50,  500, 140, 50, "SPEED UP"   },
        { 210, 500, 140, 50, "SLOW DOWN"  },
        { 370, 500, 140, 50, "RESET WHEEL" },
    };
};

static DemoScene g_scene;

// ============================================================================
// Helpers
// ============================================================================

/**
 * @brief Spawn count particles at (cx, cy) with random velocities.
 * @param scene  Scene to add particles to
 * @param cx,cy  Origin position
 * @param count  Number of particles to spawn
 * @param base   Base colour (each particle gets a slight random variation)
 */
static void spawn_particles(DemoScene& scene, int cx, int cy, int count, custom::Color base) {
    std::lock_guard<std::mutex> lock(scene.sceneMutex);
    for (int i = 0; i < count; ++i) {
        float a = (rand() % 360) * 3.14159f / 180.0f;
        float speed = (rand() % 200 + 50) / 100.0f;
        scene.particles.push_back({
            (float)cx, (float)cy,
            cosf(a) * speed, sinf(a) * speed,
            custom::Color(
                (uint8_t)(base.r + (rand() % 60 - 30)),
                (uint8_t)(base.g + (rand() % 60 - 30)),
                (uint8_t)(base.b + (rand() % 60 - 30))
            ),
            rand() % 6 + 3
        });
    }
}

/** @return 1 when the mouse is inside button bounds */
static int hit_test(const DemoScene::Button& btn, int mx, int my) {
    return (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h);
}

/** Step particle physics: gravity, bounce, wall clamp */
static void update_particles(DemoScene& scene, float dt) {
    std::lock_guard<std::mutex> lock(scene.sceneMutex);
    for (auto& p : scene.particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 120.0f * dt;

        if (p.x < 0)  { p.x = 0;  p.vx = -p.vx; }
        if (p.x > 800){ p.x = 800; p.vx = -p.vx; }
        if (p.y < 0)  { p.y = 0;  p.vy = -p.vy; }
        if (p.y > 600){ p.y = 600; p.vy = -p.vy * 0.85f; }
    }
}

// ============================================================================
// Window Thread
// ============================================================================

/**
 * @brief Owns the Window and runs the Win32 message pump.
 *
 * Handles click logic (edge-triggered) to modify the shared scene.
 * After the message loop ends, waits for the render thread to finish
 * before the Window object is destroyed (prevents dangling-pointer use).
 */
static void WindowThread() {
    Window win(800, 600);
    g_window = &win;
    g_hwnd = win.getHWND();

    bool lastClick = false;

    while (running && win.process_messages()) {
        // --- Edge-triggered click detection ---
        bool click = win.isLeftPressed();
        if (click && !lastClick) {
            int mx = win.getMouseX();
            int my = win.getMouseY();

            if      (hit_test(g_scene.buttons[0], mx, my)) { std::lock_guard<std::mutex> lk(g_scene.sceneMutex); g_scene.wheelSpeed += 0.25f; }
            else if (hit_test(g_scene.buttons[1], mx, my)) { std::lock_guard<std::mutex> lk(g_scene.sceneMutex); g_scene.wheelSpeed = std::max(0.0f, g_scene.wheelSpeed - 0.25f); }
            else if (hit_test(g_scene.buttons[2], mx, my)) { std::lock_guard<std::mutex> lk(g_scene.sceneMutex); g_scene.wheelSpeed = 0.5f; }
            else spawn_particles(g_scene, mx, my, 8, custom::Color(255, 180, 60));
        }
        lastClick = click;

        std::this_thread::yield();
    }

    // --- Signal render thread to stop, then wait for it ---
    running = false;
    while (!g_renderDone.load())
        std::this_thread::yield();
}

// ============================================================================
// Render Thread
// ============================================================================

/**
 * @brief Drives the frame loop: update, clear, draw, present.
 *
 * Waits for the window thread to create the HWND, then runs until
 * running goes false. Sets g_renderDone before exiting so the window
 * thread knows it is safe to destroy the Window object.
 */
static void GraphicsThread() {
    auto& gfx = custom::Graphic::getInstance();

    // --- Wait for the window to be ready ---
    while (running && g_hwnd.load() == NULL)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    gfx.setTarget(g_hwnd.load());
    gfx.setBG(custom::Paint(custom::Color(18, 20, 22)));

    float avgFps = 0.0f;
    const float alpha = 0.05f;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameStart - lastFrameTime;
        float dt = std::chrono::duration<float>(frameDuration).count();
        lastFrameTime = frameStart;

        // --- Smoothed FPS counter ---
        float instantFps = 1.0f / dt;
        if (avgFps == 0) avgFps = instantFps;
        else avgFps = (instantFps * alpha) + (avgFps * (1.0f - alpha));

        // --- Advance scene time and wheel angle ---
        g_scene.time += dt;
        {
            std::lock_guard<std::mutex> lk(g_scene.sceneMutex);
            g_scene.angle += g_scene.wheelSpeed * dt;
        }

        update_particles(g_scene, dt);

        // --- Snapshot particle count under the lock for the HUD ---
        size_t particleCount;
        float wheelSpeed;
        {
            std::lock_guard<std::mutex> lk(g_scene.sceneMutex);
            particleCount = g_scene.particles.size();
            wheelSpeed = g_scene.wheelSpeed;
        }

        // --- Begin drawing ---
        gfx.beginFrame();

        int mx = g_window->getMouseX();
        int my = g_window->getMouseY();
        bool pressed = g_window->isLeftPressed();

        // Engine handles backbuffer recreation + full clear on resize automatically
        g_window->wasResized();

        // --- HUD header ---
        gfx.draw_rect(0, 0, 800, 40,
            custom::Paint(custom::Color(30, 32, 36), custom::Color(18, 20, 22), true));

        gfx.draw_text(10, 12,
            ("FPS: " + std::to_string((int)avgFps)).c_str(),
            custom::Color(140, 200, 255));

        gfx.draw_text(120, 12,
            ("Mouse: " + std::to_string(mx) + ", " + std::to_string(my)).c_str(),
            custom::Color(180, 180, 180));

        gfx.draw_text(350, 12,
            ("Particles: " + std::to_string(particleCount)).c_str(),
            custom::Color(180, 220, 180));

        gfx.draw_text(560, 12,
            ("Speed: " + std::to_string(wheelSpeed).substr(0, 4)).c_str(),
            custom::Color(220, 200, 140));

        // --- Animated wheel (orbiting dots) ---
        int cx = 400, cy = 220;
        for (int i = 0; i < 12; ++i) {
            float a = g_scene.angle + i * 3.14159f / 6.0f;
            int px = cx + (int)(cosf(a) * 80);
            int py = cy + (int)(sinf(a) * 80);
            float t = (sinf(g_scene.time * 2.0f + i) + 1.0f) * 0.5f;
            uint8_t r = (uint8_t)(100 + (int)(t * 155));
            uint8_t g = (uint8_t)(50 + (int)((1.0f - t) * 150));
            uint8_t b = (uint8_t)(200 + (int)(t * 55));
            gfx.draw_ellipse(px - 12, py - 12, 24, 24,
                custom::Paint(custom::Color(r, g, b), custom::Color(r/2, g/2, b/2), false), true);
        }

        // --- Connecting lines ---
        for (int i = 0; i < 12; ++i) {
            float a = g_scene.angle + i * 3.14159f / 6.0f;
            int px = cx + (int)(cosf(a) * 80);
            int py = cy + (int)(sinf(a) * 80);
            gfx.draw_line(cx, cy, px, py, 1, custom::Paint(custom::Color(60, 60, 80)));
        }

        // --- Pulsing centre circle ---
        float pulse = sinf(g_scene.time * 1.5f) * 0.3f + 0.7f;
        int pulseR = (int)(40 * pulse);
        gfx.draw_ellipse(cx - pulseR, cy - pulseR, pulseR * 2, pulseR * 2,
            custom::Paint(custom::Color(60, 180, 255, 80), custom::Color(20, 80, 160, 40), true), true);
        gfx.draw_ellipse(cx - pulseR, cy - pulseR, pulseR * 2, pulseR * 2,
            custom::Paint(custom::Color(100, 200, 255)), false);

        // --- Title text ---
        gfx.draw_text(295, 80, "WINDOWS GUI DEMO", custom::Color(200, 200, 200));

        // --- Interactive buttons ---
        for (int i = 0; i < 3; ++i) {
            auto& btn = g_scene.buttons[i];
            bool hover = hit_test(btn, mx, my);

            custom::Paint btnPaint;
            custom::Color tcol;
            if (hover && pressed) {
                btnPaint = custom::Paint(custom::Color(0, 110, 200), custom::Color(0, 70, 140), true);
                tcol = custom::Color(255, 255, 255);
            } else if (hover) {
                btnPaint = custom::Paint(custom::Color(75, 78, 85), custom::Color(45, 47, 52), true);
                tcol = custom::Color(220, 220, 220);
            } else {
                btnPaint = custom::Paint(custom::Color(50, 52, 58), custom::Color(30, 32, 36), true);
                tcol = custom::Color(160, 160, 160);
            }

            gfx.draw_rect(btn.x, btn.y, btn.w, btn.h, btnPaint, 8);
            gfx.draw_text(btn.x + 14, btn.y + 17, btn.text, tcol);
        }

        // --- Particles ---
        {
            std::lock_guard<std::mutex> lk(g_scene.sceneMutex);
            for (auto& p : g_scene.particles) {
                gfx.draw_ellipse((int)p.x - p.size/2, (int)p.y - p.size/2,
                    p.size, p.size, custom::Paint(p.color), true);
            }
        }

        // --- Custom crosshair cursor ---
        gfx.draw_line(mx - 10, my, mx + 10, my, 1, custom::Paint(custom::Color(255, 255, 255)));
        gfx.draw_line(mx, my - 10, mx, my + 10, 1, custom::Paint(custom::Color(255, 255, 255)));
        gfx.draw_ellipse(mx - 3, my - 3, 6, 6,
            custom::Paint(custom::Color(255, 255, 255, 120)), true);

        // --- Present frame ---
        gfx.endFrame();

        // --- Yield if we finished early (1 ms target) ---
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - frameStart);
        if (elapsed.count() < 1000)
            std::this_thread::yield();
    }

    g_renderDone = true;
}

// ============================================================================
// Entry Point
// ============================================================================

/**
 * @brief Application entry point.
 *
 * Spawns the window thread (message pump) and the render thread (GDI drawing),
 * then waits for both to finish before cleaning up.
 */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    timeBeginPeriod(1);

    std::thread winThread(WindowThread);
    std::thread renderThread(GraphicsThread);

    winThread.join();
    renderThread.join();

    timeEndPeriod(1);
    return 0;
}
