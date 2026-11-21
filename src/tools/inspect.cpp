#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>

// Helper: Converts a file's memory address (RVA) into its raw byte position on disk
DWORD RvaToFileOffset(DWORD rva, PIMAGE_NT_HEADERS64 ntHeaders) {
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
        DWORD sectionStart = section->VirtualAddress;
        DWORD sectionEnd = sectionStart + section->Misc.VirtualSize;
        if (rva >= sectionStart && rva < sectionEnd) {
            return (rva - sectionStart) + section->PointerToRawData;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: .\\inspect.exe <path_to_game.exe>" << std::endl;
        return 1;
    }

    // 1. Read the target .exe file into memory
    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "[ERROR] Could not open file: " << argv[1] << std::endl;
        return 1;
    }
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<BYTE> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // 2. Verify it's a valid 64-bit Windows/Xbox PE executable
    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(buffer.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cout << "[ERROR] Not a valid MZ executable!" << std::endl;
        return 1;
    }

    auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS64>(buffer.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE || ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        std::cout << "[ERROR] Not a valid 64-bit PE executable!" << std::endl;
        return 1;
    }

    std::cout << "=== XBOX / WINDOWS EXECUTABLE SCANNER ===" << std::endl;
    std::cout << "Target: " << argv[1] << "\n" << std::endl;

    // 3. Find the Import Directory (the shopping list of required DLLs & functions)
    DWORD importRva = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (importRva == 0) {
        std::cout << "No imports found." << std::endl;
        return 0;
    }

    DWORD importOffset = RvaToFileOffset(importRva, ntHeaders);
    auto importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(buffer.data() + importOffset);

    // 4. Loop through every required DLL and print its functions
    while (importDesc->Name != 0) {
        DWORD nameOffset = RvaToFileOffset(importDesc->Name, ntHeaders);
        const char* dllName = reinterpret_cast<const char*>(buffer.data() + nameOffset);
        std::cout << "[DLL REQUIRED] " << dllName << std::endl;

        DWORD thunkRva = importDesc->OriginalFirstThunk ? importDesc->OriginalFirstThunk : importDesc->FirstThunk;
        DWORD thunkOffset = RvaToFileOffset(thunkRva, ntHeaders);
        auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA64>(buffer.data() + thunkOffset);

        while (thunk->u1.AddressOfData != 0) {
            if (IMAGE_SNAP_BY_ORDINAL64(thunk->u1.Ordinal)) {
                std::cout << "    -> Ordinal #" << IMAGE_ORDINAL64(thunk->u1.Ordinal) << std::endl;
            } else {
                DWORD funcNameOffset = RvaToFileOffset(static_cast<DWORD>(thunk->u1.AddressOfData), ntHeaders);
                auto importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(buffer.data() + funcNameOffset);
                std::cout << "    -> " << importByName->Name << std::endl;
            }
            thunk++;
        }
        std::cout << std::endl;
        importDesc++;
    }

    return 0;
}