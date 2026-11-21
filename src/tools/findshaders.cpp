#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int main() {
    std::string rpfPath = "C:\\Users\\aiden\\Downloads\\Grand Theft Auto VI\\shaders.rpf";
    std::ifstream file(rpfPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "[ERROR] Could not open: " << rpfPath << std::endl;
        return 1;
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    std::cout << "=== SHADERS.RPF CONTAINER SCANNER ===" << std::endl;
    std::cout << "Loaded " << (fileSize / (1024 * 1024)) << " MB into RAM. Scanning for DXBC headers...\n" << std::endl;

    int dxbcCount = 0;
    for (size_t i = 0; i + 32 < fileSize; i += 4) {
        // Standard Microsoft DirectX Shader Container Magic: 'D' 'X' 'B' 'C'
        if (buffer[i] == 'D' && buffer[i + 1] == 'X' && buffer[i + 2] == 'B' && buffer[i + 3] == 'C') {
            uint32_t containerSize = *reinterpret_cast<uint32_t*>(&buffer[i + 24]);
            if (containerSize > 64 && containerSize < 10 * 1024 * 1024) {
                dxbcCount++;
                if (dxbcCount <= 10) {
                    std::cout << "[SHADER #" << dxbcCount << "] Offset: " << i
                              << " | Size: " << containerSize << " bytes" << std::endl;
                }
            }
        }
    }

    std::cout << "\nScan complete! Found " << dxbcCount << " unencrypted DXBC shader containers in shaders.rpf!" << std::endl;
    return 0;
}