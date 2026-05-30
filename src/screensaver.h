#pragma once

#include <Windows.h>
#ifdef ENABLE_MAGNIFIER
#include <TlHelp32.h>
#include <shellapi.h>
#endif
#include "renderer.h"

class ScreenSaver {
public:
    bool init(HINSTANCE hInstance);
    int run();
    void shutdown();

    static ScreenSaver* getInstance() { return s_instance; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void onMouseMove(int x, int y);

    static ScreenSaver* s_instance;

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    Renderer m_renderer;
    bool m_running = false;

    // Mouse tracking for exit
    POINT m_initialMousePos = {};
    bool m_mouseInitialized = false;
    static constexpr int MOUSE_THRESHOLD = 5;

#ifdef ENABLE_MAGNIFIER
    // Magnifier handling
    bool m_magnifierWasRunning = false;
#endif
};
