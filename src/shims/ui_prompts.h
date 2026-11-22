#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <map>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace Gdiplus;

// Converts a Windows Virtual-Key code from controls.ini into a clean PC keycap label!
inline std::wstring VkToKeycapLabel(int vk) {
    switch (vk) {
        case 1:  return L"LMB";
        case 2:  return L"RMB";
        case 4:  return L"MMB";
        case 8:  return L"BKSP";
        case 9:  return L"TAB";
        case 13: return L"ENTER";
        case 16: return L"SHIFT";
        case 17: return L"CTRL";
        case 18: return L"ALT";
        case 27: return L"ESC";
        case 32: return L"SPACE";
        case 37: return L"LEFT";
        case 38: return L"UP";
        case 39: return L"RIGHT";
        case 40: return L"DOWN";
        default:
            if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
                return std::wstring(1, static_cast<wchar_t>(vk));
            }
            if (vk >= VK_F1 && vk <= VK_F12) {
                return L"F" + std::to_wstring(vk - VK_F1 + 1);
            }
            return L"KEY";
    }
}

inline int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

// Renders a crisp dark PC keycap badge with white border & text into ./ui_overrides/
inline void RenderKeycapBadge(const std::wstring& filename, const std::wstring& label, CLSID& pngClsid) {
    int width = (label.length() > 2) ? 96 : 64;
    int height = 64;

    Bitmap bmp(width, height, PixelFormat32bppARGB);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // Dark translucent keycap background + crisp white key border
    SolidBrush bgBrush(Color(230, 24, 24, 28));
    Pen borderPen(Color(255, 235, 235, 240), 3.0f);
    g.FillRectangle(&bgBrush, 4, 4, width - 8, height - 8);
    g.DrawRectangle(&borderPen, 4, 4, width - 8, height - 8);

    // Centered keycap text
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, (label.length() > 3) ? 16.0f : 22.0f, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 255, 255, 255));

    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);

    RectF layoutRect(0.0f, 0.0f, static_cast<REAL>(width), static_cast<REAL>(height));
    g.DrawString(label.c_str(), -1, &font, layoutRect, &fmt, &textBrush);

    bmp.Save(filename.c_str(), &pngClsid, nullptr);
}

// Generates all dynamic PC HUD prompt badges from controls.ini on startup!
inline void GenerateDynamicPCPrompts() {
    fs::create_directories("./ui_overrides");

    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) < 0) return;

    const char* ini = ".\\controls.ini";
    struct Binding { const char* iniKey; int defVk; const wchar_t* outFile; };

    Binding list[] = {
        { "Button_A",  32, L"./ui_overrides/prompt_A.png"  }, // Space
        { "Button_B",  67, L"./ui_overrides/prompt_B.png"  }, // C
        { "Button_X",  82, L"./ui_overrides/prompt_X.png"  }, // R
        { "Button_Y",  69, L"./ui_overrides/prompt_Y.png"  }, // E
        { "Button_LB", 81, L"./ui_overrides/prompt_LB.png" }, // Q
        { "Button_RB", 71, L"./ui_overrides/prompt_RB.png" }, // G
        { "Trigger_LT", 2, L"./ui_overrides/prompt_LT.png" }, // RMB
        { "Trigger_RT", 1, L"./ui_overrides/prompt_RT.png" }  // LMB
    };

    for (const auto& b : list) {
        int vk = GetPrivateProfileIntA("XboxControls", b.iniKey, b.defVk, ini);
        std::wstring label = VkToKeycapLabel(vk);
        RenderKeycapBadge(b.outFile, label, pngClsid);
    }

    std::cout << "[shim_d3d12.dll] Generated dynamic PC HUD keycap prompts in ./ui_overrides/ from controls.ini!" << std::endl;
}