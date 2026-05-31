#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <algorithm>
#include <vector>
#include "screensaver.h"

// Named manual-reset event used to ask a running /p instance to exit cleanly.
// The /p render loop polls this event; any new /s or /p instance signals it
// so the old /p releases its DXGI swap chain before the new one is created.
static const wchar_t* EXIT_EVENT_NAME = L"Local\\JugglerExitPreview";

// Ask any running Juggler.scr instances to exit cleanly (via EXIT_EVENT_NAME),
// wait up to 1 second for a graceful exit, then force-terminate stragglers.
// Resets the event afterward so the incoming instance's render loop won't
// immediately see it as signaled.
static void KillOtherInstances() {
    DWORD currentPid = GetCurrentProcessId();

    // Signal any running /p instance so it can release its swap chain cleanly.
    HANDLE exitEvent = CreateEventW(nullptr, TRUE, FALSE, EXIT_EVENT_NAME);
    if (exitEvent) SetEvent(exitEvent);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        if (exitEvent) { ResetEvent(exitEvent); CloseHandle(exitEvent); }
        return;
    }

    std::vector<HANDLE> handles;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Juggler.scr") == 0 &&
                pe.th32ProcessID != currentPid) {
                HANDLE proc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (proc) handles.push_back(proc);
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);

    if (!handles.empty()) {
        // Wait up to 1 second for graceful exit (lets DXGI destructors run).
        DWORD deadline = GetTickCount() + 1000;
        bool anyForcedKill = false;
        for (HANDLE h : handles) {
            DWORD now = GetTickCount();
            DWORD remaining = (deadline > now) ? (deadline - now) : 0;
            if (WaitForSingleObject(h, remaining) == WAIT_TIMEOUT) {
                TerminateProcess(h, 0);
                anyForcedKill = true;
            }
            CloseHandle(h);
        }
        if (anyForcedKill)
            Sleep(200); // Brief pause for D3D/DXGI resources after force-kill
    }

    if (exitEvent) {
        ResetEvent(exitEvent); // Reset so the new instance's render loop doesn't exit immediately
        CloseHandle(exitEvent);
    }
}

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

    // Open the exit event created and reset by KillOtherInstances().
    // A new /s or /p instance will set this event to ask us to exit cleanly,
    // releasing our swap chain before it tries to create its own on this HWND.
    HANDLE exitEvent = OpenEventW(SYNCHRONIZE, FALSE, EXIT_EVENT_NAME);
    if (!exitEvent)
        exitEvent = CreateEventW(nullptr, TRUE, FALSE, EXIT_EVENT_NAME);

    Renderer renderer;
    if (!renderer.init(previewWnd, width, height)) {
        if (exitEvent) CloseHandle(exitEvent);
        return;
    }

    MSG msg = {};
    bool running = true;
    while (running && IsWindow(previewWnd)) {
        if (exitEvent && WaitForSingleObject(exitEvent, 0) == WAIT_OBJECT_0)
            break;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (running && IsWindow(previewWnd))
            renderer.render();
    }
    renderer.shutdown();
    if (exitEvent) CloseHandle(exitEvent);
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
        KillOtherInstances();
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

        if (previewWnd && IsWindow(previewWnd)) {
            KillOtherInstances();
            RunPreview(previewWnd);
        }
        return 0;
    }

    // Default: show config
    ShowConfigDialog(nullptr);
    return 0;
}
