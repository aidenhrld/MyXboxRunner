#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <algorithm>

#define XBOX_DPAD_UP        0x0001
#define XBOX_DPAD_DOWN      0x0002
#define XBOX_DPAD_LEFT      0x0004
#define XBOX_DPAD_RIGHT     0x0008
#define XBOX_MENU_PAUSE     0x0010
#define XBOX_VIEW_BACK      0x0020
#define XBOX_LS_CLICK       0x0040
#define XBOX_RS_CLICK       0x0080
#define XBOX_LB             0x0100
#define XBOX_RB             0x0200
#define XBOX_GUIDE          0x0400
#define XBOX_SHARE          0x0800
#define XBOX_A              0x1000
#define XBOX_B              0x2000
#define XBOX_X              0x4000
#define XBOX_Y              0x8000

struct XboxGamepadState {
    uint16_t buttons;
    uint8_t  leftTrigger;
    uint8_t  rightTrigger;
    int16_t  thumbLX;
    int16_t  thumbLY;
    int16_t  thumbRX;
    int16_t  thumbRY;
};

// Configurable bindings loaded from controls.ini (using Windows Virtual-Key codes)
struct KBMConfig {
    int mouseSensitivity = 1200;
    int keyA = VK_SPACE;    // 32 (Space)
    int keyB = 'C';         // 67 (C)
    int keyX = 'R';         // 82 (R)
    int keyY = 'E';         // 69 (E)
    int keyLB = 'Q';        // 81 (Q)
    int keyRB = 'G';        // 71 (G)
    int keyLS = VK_SHIFT;   // 16 (Shift)
    int keyRS = 'V';        // 86 (V)
    int keyPause = 'P';     // 80 (P)
    int keyView = VK_TAB;   // 9  (Tab)
} cfg;

void CreateDefaultIniIfMissing(const char* iniPath) {
    std::ifstream check(iniPath);
    if (check.good()) return;

    std::ofstream out(iniPath);
    out << "; MyXboxRunner Keyboard & Mouse Config (Windows Virtual-Key Decimal Codes)\n";
    out << "; Space=32, Shift=16, Ctrl=17, Tab=9, Enter=13, A-Z = ASCII (e.g., C=67, E=69, R=82)\n";
    out << "[Controls]\n";
    out << "MouseSensitivity=1200\n";
    out << "ButtonA=32\n";
    out << "ButtonB=67\n";
    out << "ButtonX=82\n";
    out << "ButtonY=69\n";
    out << "ButtonLB=81\n";
    out << "ButtonRB=71\n";
    out << "ButtonLS=16\n";
    out << "ButtonRS=86\n";
    out << "ButtonPause=80\n";
    out << "ButtonView=9\n";
    std::cout << "[shim_input.dll] Created default controls.ini!" << std::endl;
}

void LoadControlsIni() {
    const char* iniPath = ".\\controls.ini";
    CreateDefaultIniIfMissing(iniPath);

    cfg.mouseSensitivity = GetPrivateProfileIntA("Controls", "MouseSensitivity", 1200, iniPath);
    cfg.keyA     = GetPrivateProfileIntA("Controls", "ButtonA", VK_SPACE, iniPath);
    cfg.keyB     = GetPrivateProfileIntA("Controls", "ButtonB", 'C', iniPath);
    cfg.keyX     = GetPrivateProfileIntA("Controls", "ButtonX", 'R', iniPath);
    cfg.keyY     = GetPrivateProfileIntA("Controls", "ButtonY", 'E', iniPath);
    cfg.keyLB    = GetPrivateProfileIntA("Controls", "ButtonLB", 'Q', iniPath);
    cfg.keyRB    = GetPrivateProfileIntA("Controls", "ButtonRB", 'G', iniPath);
    cfg.keyLS    = GetPrivateProfileIntA("Controls", "ButtonLS", VK_SHIFT, iniPath);
    cfg.keyRS    = GetPrivateProfileIntA("Controls", "ButtonRS", 'V', iniPath);
    cfg.keyPause = GetPrivateProfileIntA("Controls", "ButtonPause", 'P', iniPath);
    cfg.keyView  = GetPrivateProfileIntA("Controls", "ButtonView", VK_TAB, iniPath);

    std::cout << "[shim_input.dll] Loaded controls.ini (MouseSensitivity=" << cfg.mouseSensitivity << ")" << std::endl;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        LoadControlsIni();
    }
    return TRUE;
}

bool IsKeyDown(int vKey) {
    return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}

POINT lastMousePos = { 0, 0 };
bool firstMouseRead = true;

extern "C" {

    __declspec(dllexport) bool XInputGetXboxPadFromKBM(int playerIndex, XboxGamepadState* outState) {
        if (playerIndex != 0 || outState == nullptr) return false;
        ZeroMemory(outState, sizeof(XboxGamepadState));

        // Configurable Face, Bumper, Stick & System Buttons
        if (IsKeyDown(cfg.keyA))                        outState->buttons |= XBOX_A;
        if (IsKeyDown(cfg.keyB) || IsKeyDown(VK_CONTROL)) outState->buttons |= XBOX_B;
        if (IsKeyDown(cfg.keyX))                        outState->buttons |= XBOX_X;
        if (IsKeyDown(cfg.keyY) || IsKeyDown('F'))      outState->buttons |= XBOX_Y;
        if (IsKeyDown(cfg.keyLB))                       outState->buttons |= XBOX_LB;
        if (IsKeyDown(cfg.keyRB))                       outState->buttons |= XBOX_RB;
        if (IsKeyDown(cfg.keyLS))                       outState->buttons |= XBOX_LS_CLICK;
        if (IsKeyDown(cfg.keyRS) || IsKeyDown(VK_MBUTTON)) outState->buttons |= XBOX_RS_CLICK;
        if (IsKeyDown(cfg.keyPause) || IsKeyDown(VK_RETURN)) outState->buttons |= XBOX_MENU_PAUSE;
        if (IsKeyDown(cfg.keyView))                     outState->buttons |= XBOX_VIEW_BACK;
        if (IsKeyDown(VK_F12))                          outState->buttons |= XBOX_SHARE;
        if (IsKeyDown(VK_HOME))                         outState->buttons |= XBOX_GUIDE;

        // D-Pad (Arrow Keys)
        if (IsKeyDown(VK_UP))    outState->buttons |= XBOX_DPAD_UP;
        if (IsKeyDown(VK_DOWN))  outState->buttons |= XBOX_DPAD_DOWN;
        if (IsKeyDown(VK_LEFT))  outState->buttons |= XBOX_DPAD_LEFT;
        if (IsKeyDown(VK_RIGHT)) outState->buttons |= XBOX_DPAD_RIGHT;

        // Left Stick (WASD)
        if (IsKeyDown('W')) outState->thumbLY = 32767;
        if (IsKeyDown('S')) outState->thumbLY = -32768;
        if (IsKeyDown('D')) outState->thumbLX = 32767;
        if (IsKeyDown('A')) outState->thumbLX = -32768;

        // Triggers (Mouse Clicks)
        if (IsKeyDown(VK_RBUTTON)) outState->leftTrigger = 255;
        if (IsKeyDown(VK_LBUTTON)) outState->rightTrigger = 255;

        // Right Stick (Mouse Look with configurable sensitivity)
        POINT currentMouse;
        if (GetCursorPos(&currentMouse)) {
            if (firstMouseRead) {
                lastMousePos = currentMouse;
                firstMouseRead = false;
            }
            int deltaX = currentMouse.x - lastMousePos.x;
            int deltaY = lastMousePos.y - currentMouse.y;
            lastMousePos = currentMouse;

            outState->thumbRX = static_cast<int16_t>(std::clamp(deltaX * cfg.mouseSensitivity, -32768, 32767));
            outState->thumbRY = static_cast<int16_t>(std::clamp(deltaY * cfg.mouseSensitivity, -32768, 32767));
        }

        return true;
    }
}
