#include <Windows.h>
#include <string>
#include <algorithm>
#include "screensaver.h"

static void ShowConfigDialog(HWND parent) {
    MessageBoxW(parent,
        L"Amiga Juggler DXR 1.1 Screen Saver\n\n"
        L"Recreates the iconic 1985 Amiga Juggler demo\n"
        L"using DirectX 12 DXR 1.1 hardware ray tracing.\n\n"
        L"Requires a DXR 1.1 capable GPU.\n\n"
        L"No configuration options available.",
        L"Juggler Screen Saver",
        MB_OK | MB_ICONINFORMATION);
}

static void RunPreview(HWND previewWnd) {
    RECT rc;
    GetClientRect(previewWnd, &rc);
    uint32_t width  = std::max(1, (int)(rc.right  - rc.left));
    uint32_t height = std::max(1, (int)(rc.bottom - rc.top));

    Renderer renderer;
    if (!renderer.init(previewWnd, width, height))
        return;

    MSG msg = {};
    bool running = true;
    while (running && IsWindow(previewWnd)) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (running && IsWindow(previewWnd))
            renderer.render();
    }
    renderer.shutdown();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
    std::wstring cmdLine(lpCmdLine);

    // Trim leading whitespace
    size_t start = cmdLine.find_first_not_of(L" \t");
    if (start != std::wstring::npos)
        cmdLine = cmdLine.substr(start);
    else
        cmdLine.clear();

    // Parse first argument
    std::wstring arg;
    if (!cmdLine.empty()) {
        // Handle /s, /c, /p, -s, -c, -p and variants with ':'
        if (cmdLine[0] == L'/' || cmdLine[0] == L'-') {
            arg = cmdLine.substr(0, 2);
            std::transform(arg.begin(), arg.end(), arg.begin(), ::towlower);
        }
    }

    if (arg == L"/s" || arg == L"-s") {
        // Full-screen mode
        ScreenSaver ss;
        if (!ss.init(hInstance)) {
            return 1;
        }
        int result = ss.run();
        ss.shutdown();
        return result;
    }
    else if (arg == L"/c" || arg == L"-c" || cmdLine.empty()) {
        // Configuration mode (or no args = default to config)
        HWND parent = nullptr;
        // Check for :HWND suffix
        if (cmdLine.length() > 2 && cmdLine[2] == L':') {
            parent = (HWND)(uintptr_t)_wcstoui64(cmdLine.c_str() + 3, nullptr, 10);
        }
        ShowConfigDialog(parent);
        return 0;
    }
    else if (arg == L"/p" || arg == L"-p") {
        // Preview mode
        HWND previewWnd = nullptr;
        std::wstring rest = cmdLine.substr(2);

        // Trim leading whitespace and ':'
        start = rest.find_first_not_of(L" \t:");
        if (start != std::wstring::npos) {
            rest = rest.substr(start);
            previewWnd = (HWND)(uintptr_t)_wcstoui64(rest.c_str(), nullptr, 10);
        }

        if (previewWnd && IsWindow(previewWnd))
            RunPreview(previewWnd);
        return 0;
    }

    // Default: show config
    ShowConfigDialog(nullptr);
    return 0;
}
