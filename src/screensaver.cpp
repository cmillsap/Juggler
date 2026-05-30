#include "screensaver.h"

ScreenSaver* ScreenSaver::s_instance = nullptr;

bool ScreenSaver::init(HINSTANCE hInstance) {
    s_instance = this;
    m_hInstance = hInstance;

#ifdef ENABLE_MAGNIFIER
    // Check if Windows Magnifier is running and close it
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"Magnify.exe") == 0) {
                    m_magnifierWasRunning = true;

                    // Try graceful close via its top-level window
                    HWND magWnd = FindWindowW(L"Screen Magnifier Window", nullptr);
                    if (magWnd) {
                        PostMessageW(magWnd, WM_CLOSE, 0, 0);
                    } else {
                        // Fallback: terminate the process directly
                        HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if (proc) {
                            TerminateProcess(proc, 0);
                            CloseHandle(proc);
                        }
                    }
                    break;
                }
            } while (Process32NextW(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
#endif

    // Get primary monitor dimensions
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = nullptr;
    wc.lpszClassName = L"JugglerScreenSaver";
    RegisterClassExW(&wc);

    // Create full-screen borderless window
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"JugglerScreenSaver",
        L"Juggler",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        nullptr, nullptr, hInstance, nullptr);

    if (!m_hwnd) return false;

    ShowCursor(FALSE);
    SetFocus(m_hwnd);
    SetForegroundWindow(m_hwnd);

    if (!m_renderer.init(m_hwnd, screenW, screenH)) {
        return false;
    }

    m_running = true;
    return true;
}

int ScreenSaver::run() {
    MSG msg = {};
    while (m_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (m_running) {
            m_renderer.render();
        }
    }
    return (int)msg.wParam;
}

void ScreenSaver::shutdown() {
    m_renderer.shutdown();
    ShowCursor(TRUE);
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

#ifdef ENABLE_MAGNIFIER
    // Restart Magnifier if it was running before
    if (m_magnifierWasRunning) {
        ShellExecuteW(nullptr, L"open", L"Magnify.exe", nullptr, nullptr, SW_SHOWNORMAL);
    }
#endif
}

void ScreenSaver::onMouseMove(int x, int y) {
    if (!m_mouseInitialized) {
        m_initialMousePos = { x, y };
        m_mouseInitialized = true;
        return;
    }

    int dx = x - m_initialMousePos.x;
    int dy = y - m_initialMousePos.y;
    if (dx * dx + dy * dy > MOUSE_THRESHOLD * MOUSE_THRESHOLD) {
        PostQuitMessage(0);
    }
}

LRESULT CALLBACK ScreenSaver::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ScreenSaver* ss = s_instance;

    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            PostQuitMessage(0);
            return 0;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            PostQuitMessage(0);
            return 0;

        case WM_MOUSEMOVE:
            if (ss) {
                ss->onMouseMove(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SETCURSOR:
            SetCursor(nullptr);
            return TRUE;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                PostQuitMessage(0);
            }
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
