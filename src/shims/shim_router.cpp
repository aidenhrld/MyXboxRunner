#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>

std::unordered_map<std::string, HMODULE> loadedShims;
std::ofstream logFile;

void RouterLog(const std::string& msg) {
    std::cout << msg << std::endl;
    if (logFile.is_open()) {
        logFile << msg << std::endl;
        logFile.flush();
    }
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

HMODULE GetOrLoadShim(const std::string& shimDllName) {
    auto it = loadedShims.find(shimDllName);
    if (it != loadedShims.end()) return it->second;

    HMODULE mod = LoadLibraryA(shimDllName.c_str());
    if (mod) {
        loadedShims[shimDllName] = mod;
    }
    return mod;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        logFile.open(".\\runner_log.txt", std::ios::out | std::ios::trunc);
        RouterLog("=== MyXboxRunner Import Router Log Started ===");
    } else if (reason == DLL_PROCESS_DETACH) {
        if (logFile.is_open()) logFile.close();
    }
    return TRUE;
}

extern "C" {

    __declspec(dllexport) FARPROC ResolveXboxImport(const char* xboxDllName, const char* funcName) {
        std::string dll = ToLower(xboxDllName);
        std::string targetShim = "";

        if (dll == "kernelx.dll" || dll == "xgameruntime.dll" || dll == "xgameplatform.dll" || dll == "xmem.dll") {
            targetShim = "shim_kernelx.dll";
        } else if (dll == "gameinput.dll" || dll == "xinput1_4.dll" || dll == "xinput9_1_0.dll") {
            targetShim = "shim_input.dll";
        } else if (dll == "d3d12_xs.dll" || dll == "d3d12_x.dll" || dll == "d3d12.dll") {
            targetShim = "shim_d3d12.dll";
        } else if (dll == "xaudio2_9.dll" || dll == "xaudio2_8.dll") {
            targetShim = "shim_audio.dll";
        } else {
            RouterLog("[UNMAPPED DLL] " + std::string(xboxDllName) + "!" + std::string(funcName));
            return nullptr;
        }

        HMODULE shimMod = GetOrLoadShim(targetShim);
        if (!shimMod) {
            RouterLog("[ROUTER ERROR] Failed to load target shim: " + targetShim);
            return nullptr;
        }

        FARPROC funcAddr = GetProcAddress(shimMod, funcName);
        if (funcAddr) {
            RouterLog("[RESOLVED] " + std::string(xboxDllName) + "!" + std::string(funcName) + " -> " + targetShim);
        } else {
            RouterLog("[MISSING]  " + std::string(xboxDllName) + "!" + std::string(funcName) + " (Add to " + targetShim + ")");
        }

        return funcAddr;
    }

    __declspec(dllexport) void ShutdownXboxRouter() {
        for (auto& pair : loadedShims) {
            if (pair.second) FreeLibrary(pair.second);
        }
        loadedShims.clear();
        RouterLog("=== All Xbox shims unloaded cleanly ===");
    }
}