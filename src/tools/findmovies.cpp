#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string rpfPath = "C:\\Users\\aiden\\Downloads\\Grand Theft Auto VI\\movies.rpf";
    if (argc >= 2) rpfPath = *(argv + 1);

    std::ifstream file(rpfPath, std::ios::binary);
    if (!file) {
        std::cout << "[ERROR] Could not open: " << rpfPath << std::endl;
        return 1;
    }

    std::cout << "=== BINK2 (.BK2) VIDEO SCANNER ===" << std::endl;
    std::cout << "Scanning: " << rpfPath << "\n" << std::endl;

    // Read in 64 MB chunks so it scans 8.4 GB in just a few seconds!
    const size_t CHUNK_SIZE = 64 * 1024 * 1024;
    std::vector<uint8_t> buffer(CHUNK_SIZE + 32);

    uint64_t fileOffset = 0;
    int videoCount = 0;

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
        size_t bytesRead = static_cast<size_t>(file.gcount());
        if (bytesRead < 16) break;

        for (size_t i = 0; i + 16 < bytesRead; i++) {
            // Check for Bink 2 Magic Header: 'K' 'B' '2' followed by version letter ('a'-'z')
            if (buffer[i] == 'K' && buffer[i + 1] == 'B' && buffer[i + 2] == '2' &&
                buffer[i + 3] >= 'a' && buffer[i + 3] <= 'z') {

                // Bytes 4..7 in a Bink2 header store (fileSize - 8) as a 32-bit unsigned int!
                uint32_t rawSize = *reinterpret_cast<uint32_t*>(&buffer[i + 4]);
                uint64_t totalVideoBytes = static_cast<uint64_t>(rawSize) + 8;

                // Sanity filter: real video clips are between 100 KB and 4 GB
                if (totalVideoBytes > 100 * 1024 && totalVideoBytes < 4ULL * 1024 * 1024 * 1024) {
                    videoCount++;
                    double mb = totalVideoBytes / (1024.0 * 1024.0);
                    std::cout << "[VIDEO #" << videoCount << "] Header: KB2" << static_cast<char>(buffer[i + 3])
                              << " | Offset: " << (fileOffset + i)
                              << " | Size: " << mb << " MB" << std::endl;

                    // Skip past this video so we don't match bytes inside the video stream
                    if (i + totalVideoBytes < bytesRead) {
                        i += static_cast<size_t>(totalVideoBytes) - 1;
                    }
                }
            }
        }
        fileOffset += bytesRead;
    }

    std::cout << "\nScan complete! Found " << videoCount << " Bink2 (.bk2) video streams." << std::endl;
    return 0;
}