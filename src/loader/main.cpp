#include <windows.h>
#include <iostream>
#include <cstdint>

typedef FARPROC (*ResolveXboxImport_t)(const char*, const char*);
typedef void    (*ShutdownXboxRouter_t)();

typedef void* (*XMemAlloc_t)(size_t, unsigned long);
typedef void  (*XMemFree_t)(void*);
typedef bool  (*XUserIsSignedIn_t)(uint64_t*);
typedef bool  (*D3D12CreateXboxDisplay_t)(int, int);
typedef bool  (*D3D12PresentFrame_t)();
typedef bool  (*XAudio2Create_Shim_t)();
typedef void  (*XAudio2PlayBootChime_t)();

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "   MYXBOXRUNNER - MASTER IMPORT ROUTER TEST     " << std::endl;
    std::cout << "================================================\n" << std::endl;

    HMODULE router = LoadLibraryA("shim_router.dll");
    if (!router) {
        std::cout << "[FATAL] Could not load shim_router.dll!" << std::endl;
        return 1;
    }

    auto ResolveImport  = (ResolveXboxImport_t)GetProcAddress(router, "ResolveXboxImport");
    auto ShutdownRouter = (ShutdownXboxRouter_t)GetProcAddress(router, "ShutdownXboxRouter");

    // Simulate your future Loader resolving real Xbox Series X|S imports!
    auto XMemAlloc     = (XMemAlloc_t)ResolveImport("kernelx.dll", "XMemAlloc");
    auto XMemFree      = (XMemFree_t)ResolveImport("kernelx.dll", "XMemFree");
    auto XUserSignedIn = (XUserIsSignedIn_t)ResolveImport("xgameruntime.dll", "XUserIsSignedIn");
    auto InitAudio     = (XAudio2Create_Shim_t)ResolveImport("xaudio2_9.dll", "XAudio2Create_Shim");
    auto PlayChime     = (XAudio2PlayBootChime_t)ResolveImport("xaudio2_9.dll", "XAudio2PlayBootChime");
    auto CreateDisplay = (D3D12CreateXboxDisplay_t)ResolveImport("d3d12_xs.dll", "D3D12CreateXboxDisplay");
    auto PresentFrame  = (D3D12PresentFrame_t)ResolveImport("d3d12_xs.dll", "D3D12PresentFrame");

    // Also test what happens when a game asks for a function you haven't built yet!
    ResolveImport("kernelx.dll", "XSomeUnimplementedFunction");

    std::cout << "\n--- Executing Routed Xbox Calls ---\n" << std::endl;

    uint64_t xuid = 0;
    if (XUserSignedIn) XUserSignedIn(&xuid);
    if (InitAudio && InitAudio() && PlayChime) PlayChime();

    void* ram = XMemAlloc ? XMemAlloc(64 * 1024 * 1024, 0x80000001) : nullptr;

    if (CreateDisplay && CreateDisplay(1280, 720)) {
        std::cout << "[ROUTER TEST] Window active! Press ESC to shut down.\n" << std::endl;
        while (PresentFrame && PresentFrame()) {
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;
        }
    }

    if (XMemFree && ram) XMemFree(ram);
    if (ShutdownRouter) ShutdownRouter();
    FreeLibrary(router);

    return 0;
}