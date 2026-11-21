#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <iomanip>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: .\\dumpcheck.exe \"<path_to_208gb_dump_folder>\"" << std::endl;
        return 1;
    }

    fs::path dumpRoot(*(argv + 1));
    if (!fs::exists(dumpRoot) || !fs::is_directory(dumpRoot)) {
        std::cout << "[ERROR] Folder not found: " << dumpRoot << std::endl;
        return 1;
    }

    std::cout << "=== XBOX DUMP STRUCTURE & HEADER INSPECTOR ===" << std::endl;
    std::cout << "Scanning: " << dumpRoot << "\n" << std::endl;

    std::map<std::string, uint64_t> extCounts;
    uint64_t totalFiles = 0;
    uint64_t totalBytes = 0;

    for (const auto& entry : fs::recursive_directory_iterator(dumpRoot, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;

        totalFiles++;
        uint64_t fsize = entry.file_size();
        totalBytes += fsize;

        std::string ext = entry.path().extension().string();
        if (ext.empty()) ext = "(no extension)";
        extCounts[ext]++;

        std::string fname = entry.path().filename().string();

        if (ext == ".exe" || ext == ".dll" || fname == "MicrosoftGame.config" || fname == "appxmanifest.xml") {
            std::cout << "[KEY FILE FOUND] " << entry.path().string()
                      << " (" << (fsize / 1024) << " KB)" << std::endl;
        }

        if (fsize > 100ULL * 1024 * 1024) {
            std::ifstream f(entry.path(), std::ios::binary);
            std::vector<unsigned char> header(8, 0);
            f.read(reinterpret_cast<char*>(header.data()), 8);

            std::cout << "[LARGE ARCHIVE]  " << fname << " (" << (fsize / (1024 * 1024)) << " MB) | Magic Bytes: ";
            for (unsigned char b : header) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
            }
            std::cout << std::dec << " | ASCII: ";
            for (unsigned char b : header) {
                char c = (b >= 32 && b <= 126) ? (char)b : '.';
                std::cout << c;
            }
            std::cout << std::endl;
        }
    }

    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total Files: " << totalFiles << " | Total Size: " << (totalBytes / (1024ULL * 1024 * 1024)) << " GB\n" << std::endl;
    std::cout << "File Extensions Found:" << std::endl;
    for (const auto& pair : extCounts) {
        std::cout << "  " << std::setw(16) << std::left << pair.first << " : " << pair.second << " files" << std::endl;
    }

    return 0;
}