#include <windows.h>
#include <gdiplus.h>
#include <dxgi1_6.h>
#include <iostream>
#include <string>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;

HWND gameWindow = nullptr;
ULONG_PTR gdiplusToken = 0;
Image* splashImage = nullptr;
HMODULE nativeWinD3D12 = nullptr;

typedef HRESULT (WINAPI *PFN_D3D12_CREATE_DEVICE_LOCAL)(IUnknown*, int, REFIID, void**);

void EnsureNativeD3D12Loaded() {
    if (!nativeWinD3D12) {
        nativeWinD3D12 = LoadLibraryA("C:\\Windows\\System32\\d3d12.dll");
        if (nativeWinD3D12) {
            std::cout << "[shim_d3d12.dll] Hooked native Windows 11 d3d12.dll (RTX 5060)!" << std::endl;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_PAINT && splashImage != nullptr) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        Graphics graphics(hdc);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.DrawImage(splashImage, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        EndPaint(hwnd, &ps);
        return 0;
    }
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

extern "C" {

    // 1. DirectX 12 Device Forwarder (No d3d12.h dllimport conflict!)
    __declspec(dllexport) HRESULT WINAPI D3D12CreateDevice(
        IUnknown* pAdapter,
        int MinimumFeatureLevel,
        REFIID riid,
        void** ppDevice) {

        EnsureNativeD3D12Loaded();
        std::cout << "[shim_d3d12.dll] Game called D3D12CreateDevice -> Forwarding to RTX 5060!" << std::endl;

        auto realCreateDevice = (PFN_D3D12_CREATE_DEVICE_LOCAL)GetProcAddress(nativeWinD3D12, "D3D12CreateDevice");
        if (!realCreateDevice) return E_FAIL;

        return realCreateDevice(pAdapter, MinimumFeatureLevel, riid, ppDevice);
    }

    // 2. DirectX 12 Debug Interface Stub
    __declspec(dllexport) HRESULT WINAPI D3D12GetDebugInterface(REFIID riid, void** ppvDebug) {
        std::cout << "[shim_d3d12.dll] Game called D3D12GetDebugInterface -> Disabled (Retail Mode)" << std::endl;
        if (ppvDebug) *ppvDebug = nullptr;
        return E_NOINTERFACE;
    }

    // 3. Opens the Game Window & Shows Boot Splash Screen
    __declspec(dllexport) bool D3D12CreateXboxDisplay(int width, int height) {
        EnsureNativeD3D12Loaded();
        std::cout << "[shim_d3d12.dll] Creating " << width << "x" << height << " Xbox display window..." << std::endl;

        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        const wchar_t* splashPath = L"C:\\Users\\aiden\\Downloads\\Grand Theft Auto VI\\xbs_splash.png";
        splashImage = new Image(splashPath);
        if (splashImage->GetLastStatus() == Ok) {
            std::cout << "[shim_d3d12.dll] Loaded boot splash screen: xbs_splash.png!" << std::endl;
        } else {
            delete splashImage;
            splashImage = nullptr;
        }

        WNDCLASSA wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = "MyXboxRunnerWindow";
        RegisterClassA(&wc);

        gameWindow = CreateWindowExA(
            0, "MyXboxRunnerWindow", "Grand Theft Auto VI — MyXboxRunner (Scarlett DX12)",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, wc.hInstance, nullptr
        );

        InvalidateRect(gameWindow, nullptr, TRUE);
        UpdateWindow(gameWindow);
        return true;
    }

    __declspec(dllexport) bool D3D12PresentFrame() {
        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(16);
        return true;
    }
}