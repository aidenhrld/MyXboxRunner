#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

typedef FARPROC (*ResolveXboxImport_t)(const char*, const char*);

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool IsXboxDll(const std::string& dllName) {
    std::string d = ToLower(dllName);
    return (d == "kernelx.dll" || d == "xgameruntime.dll" || d == "xgameplatform.dll" ||
            d == "xmem.dll" || d == "d3d12_xs.dll" || d == "d3d12_x.dll" ||
            d == "gameinput.dll" || d == "xinput1_4.dll" || d == "xaudio2_9.dll");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: .\\loader.exe \"<path_to_game_scarlett_master.exe>\"" << std::endl;
        return 1;
    }

    const char* targetExe = *(argv + 1);
    std::cout << "================================================" << std::endl;
    std::cout << "   MYXBOXRUNNER - 64-BIT PE EXECUTABLE LOADER   " << std::endl;
    std::cout << "================================================\n" << std::endl;
    std::cout << "[LOADER] Target Executable: " << targetExe << std::endl;

    // 1. Load our Master Xbox Import Router
    HMODULE router = LoadLibraryA("shim_router.dll");
    if (!router) {
        std::cout << "[FATAL] Could not load shim_router.dll!" << std::endl;
        return 1;
    }
    auto ResolveXboxImport = (ResolveXboxImport_t)GetProcAddress(router, "ResolveXboxImport");

    // 2. Read the target .exe file into a buffer
    std::ifstream file(targetExe, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "[FATAL] Cannot open target executable!" << std::endl;
        return 1;
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<BYTE> fileData(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);

    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(fileData.data());
    auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS64>(fileData.data() + dosHeader->e_lfanew);

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        std::cout << "[FATAL] Invalid PE executable headers!" << std::endl;
        return 1;
    }

    // 3. Allocate executable memory image (SizeOfImage)
    size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    BYTE* imageBase = reinterpret_cast<BYTE*>(VirtualAlloc(
        reinterpret_cast<LPVOID>(ntHeaders->OptionalHeader.ImageBase),
        imageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE
    ));

    // Fallback to any available base address if preferred ImageBase is occupied
    if (!imageBase) {
        imageBase = reinterpret_cast<BYTE*>(VirtualAlloc(nullptr, imageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    }
    if (!imageBase) {
        std::cout << "[FATAL] Failed to allocate " << (imageSize / (1024 * 1024)) << " MB image memory!" << std::endl;
        return 1;
    }
    std::cout << "[LOADER] Mapped PE Image Base at: 0x" << static_cast<void*>(imageBase)
              << " (" << (imageSize / 1024) << " KB)" << std::endl;

    // 4. Copy PE Headers and every Section (.text, .rdata, .data, etc.) into RAM
    memcpy(imageBase, fileData.data(), ntHeaders->OptionalHeader.SizeOfHeaders);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData > 0) {
            memcpy(imageBase + section->VirtualAddress,
                   fileData.data() + section->PointerToRawData,
                   section->SizeOfRawData);
        }
    }

    // 5. Apply Base Relocations (if loaded at a different address than preferred ImageBase)
    ptrdiff_t delta = reinterpret_cast<uintptr_t>(imageBase) - ntHeaders->OptionalHeader.ImageBase;
    if (delta != 0) {
        auto relocDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size > 0) {
            auto reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(imageBase + relocDir.VirtualAddress);
            while (reloc->VirtualAddress != 0) {
                DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* entries = reinterpret_cast<WORD*>(reloc + 1);
                for (DWORD j = 0; j < count; j++) {
                    WORD entry = *(entries + j);
                    if ((entry >> 12) == IMAGE_REL_BASED_DIR64) {
                        uintptr_t* patchAddr = reinterpret_cast<uintptr_t*>(imageBase + reloc->VirtualAddress + (entry & 0x0FFF));
                        *patchAddr += delta;
                    }
                }
                reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(reinterpret_cast<BYTE*>(reloc) + reloc->SizeOfBlock);
            }
        }
    }

    // 6. Resolve Imports (Route Xbox DLLs -> shim_router.dll, Windows DLLs -> native OS!)
    auto importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.Size > 0) {
        auto importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(imageBase + importDir.VirtualAddress);
        while (importDesc->Name != 0) {
            const char* dllName = reinterpret_cast<const char*>(imageBase + importDesc->Name);
            bool xboxMode = IsXboxDll(dllName);
            HMODULE nativeMod = xboxMode ? nullptr : LoadLibraryA(dllName);

            auto origThunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(imageBase + (importDesc->OriginalFirstThunk ? importDesc->OriginalFirstThunk : importDesc->FirstThunk));
            auto iatThunk  = reinterpret_cast<PIMAGE_THUNK_DATA64>(imageBase + importDesc->FirstThunk);

            while (origThunk->u1.AddressOfData != 0) {
                FARPROC funcAddr = nullptr;
                if (IMAGE_SNAP_BY_ORDINAL64(origThunk->u1.Ordinal)) {
                    if (nativeMod) funcAddr = GetProcAddress(nativeMod, MAKEINTRESOURCEA(IMAGE_ORDINAL64(origThunk->u1.Ordinal)));
                } else {
                    auto byName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(imageBase + origThunk->u1.AddressOfData);
                    if (xboxMode) {
                        funcAddr = ResolveXboxImport(dllName, byName->Name);
                    } else if (nativeMod) {
                        funcAddr = GetProcAddress(nativeMod, byName->Name);
                    }
                }
                iatThunk->u1.Function = reinterpret_cast<ULONGLONG>(funcAddr);
                origThunk++;
                iatThunk++;
            }
            importDesc++;
        }
    }

    // 7. Jump to the Executable's Entry Point!
    typedef int (*EntryPoint_t)();
    auto entryPoint = reinterpret_cast<EntryPoint_t>(imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint);
    // Catch crashes inside the mapped game and print the exact RVA offset!
    static BYTE* g_Base = imageBase;
    AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS info) -> LONG {
        DWORD code = info->ExceptionRecord->ExceptionCode;
        if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress);
            uintptr_t rva = addr - reinterpret_cast<uintptr_t>(g_Base);
            std::cout << "\n[CRASH CAUGHT] Exception 0x" << std::hex << code
                      << " at Address 0x" << addr << " (Game RVA: 0x" << rva << ")" << std::dec << std::endl;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    });
    std::cout << "[LOADER] Jumping to Entry Point at 0x" << reinterpret_cast<void*>(entryPoint) << "...\n" << std::endl;

    return entryPoint();
}