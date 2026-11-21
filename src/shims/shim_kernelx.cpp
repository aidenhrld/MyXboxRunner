#include <windows.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

void logMessage(const std::string& message) {
    std::cout << "[shim_kernelx.dll] " << message << std::endl;
}

const std::string GAME_DUMP_ROOT = "C:/Users/aiden/Downloads/Grand Theft Auto VI/";

std::string InternalTranslatePath(const std::string& xboxPath) {
    std::string result = xboxPath;
    if (xboxPath.rfind("G:\\", 0) == 0 || xboxPath.rfind("g:\\", 0) == 0) {
        result = GAME_DUMP_ROOT + xboxPath.substr(3);
    } else if (xboxPath.rfind("U:\\", 0) == 0 || xboxPath.rfind("u:\\", 0) == 0) {
        result = "./save_files/" + xboxPath.substr(3);
    } else if (xboxPath.rfind("T:\\", 0) == 0 || xboxPath.rfind("t:\\", 0) == 0) {
        result = "./temp_files/" + xboxPath.substr(3);
    }
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        fs::create_directories("./save_files");
        fs::create_directories("./temp_files");
        logMessage("Initialized! G:\\ -> " + GAME_DUMP_ROOT);
    }
    return TRUE;
}

extern "C" {

    // 1. Exact xmem.dll Exports
    __declspec(dllexport) void* XMemAlloc(SIZE_T size, ULONGLONG attrs) {
        return VirtualAlloc(nullptr, size ? size : 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    __declspec(dllexport) void XMemFree(void* p, ULONGLONG attrs) {
        if (p) VirtualFree(p, 0, MEM_RELEASE);
    }

    __declspec(dllexport) BOOL XMemGetWorkingSetStatistics(DWORD kind, ULONGLONG* out) {
        MEMORYSTATUSEX m{ sizeof(m) };
        if (!out || !GlobalMemoryStatusEx(&m)) return FALSE;
        *(out + 0) = m.ullTotalPhys;
        *(out + 1) = m.ullAvailPhys;
        return TRUE;
    }

    __declspec(dllexport) BOOL XMemVirtualQuery(HANDLE process, const void* address, void* out) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!out || VirtualQueryEx(process ? process : GetCurrentProcess(), address, &mbi, sizeof(mbi)) == 0) return FALSE;
        memset(out, 0, 64);
        memcpy(out, &mbi, sizeof(mbi));
        *reinterpret_cast<DWORD*>(static_cast<char*>(out) + 0x30) = (mbi.State == MEM_COMMIT) ? 3u : 0u;
        return TRUE;
    }

    // 2. Title Identity & Series S 8GB VRAM Profile
    __declspec(dllexport) HRESULT XGameGetXboxTitleId(uint32_t* titleId) {
        if (titleId) *titleId = 0x7952987A;
        return S_OK;
    }

    __declspec(dllexport) int XSystemGetDeviceType() {
        return 2; // Xbox Series S (8GB VRAM Profile for RTX 5060)
    }

    __declspec(dllexport) uint32_t XSystemGetProcessorCount() {
        return 7;
    }

    // 3. Offline Xbox Live Profile & Virtual Filesystem
    __declspec(dllexport) bool XUserIsSignedIn(uint64_t* outXuid) {
        if (outXuid != nullptr) *outXuid = 2535412345678901ULL;
        return true;
    }

    __declspec(dllexport) HANDLE XCreateFileA(const char* xboxPath, DWORD desiredAccess, DWORD creationDisposition) {
        std::string pcPath = InternalTranslatePath(xboxPath);
        fs::path p(pcPath);
        if (p.has_parent_path() && pcPath.rfind("./", 0) == 0) {
            fs::create_directories(p.parent_path());
        }
        return CreateFileA(pcPath.c_str(), desiredAccess, FILE_SHARE_READ, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    __declspec(dllexport) uint64_t XGetTickCount64() { return GetTickCount64(); }
    __declspec(dllexport) bool XQueryPerformanceCounter(LARGE_INTEGER* outCounter) { return QueryPerformanceCounter(outCounter) != 0; }
}