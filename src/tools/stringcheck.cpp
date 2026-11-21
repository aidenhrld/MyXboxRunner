#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

int main() {
    std::string rpfPath = "C:\\Users\\aiden\\Downloads\\Grand Theft Auto VI\\data.rpf";
    std::ifstream file(rpfPath, std::ios::binary);
    if (!file) {
        std::cout << "[ERROR] Could not open: " << rpfPath << std::endl;
        return 1;
    }

    std::cout << "=== DATA.RPF STARTUP & ENTITLEMENT STRING SCANNER ===" << std::endl;
    std::cout << "Scanning: " << rpfPath << "\n" << std::endl;

    const size_t CHUNK_SIZE = 32 * 1024 * 1024;
    std::vector<char> buffer(CHUNK_SIZE);

    std::string currentWord;
    int matchesFound = 0;

    while (file) {
        file.read(buffer.data(), CHUNK_SIZE);
        size_t bytesRead = static_cast<size_t>(file.gcount());
        if (bytesRead == 0) break;

        for (size_t i = 0; i < bytesRead; i++) {
            unsigned char c = static_cast<unsigned char>(buffer[i]);

            // Collect printable ASCII strings (letters, numbers, slashes, dots, underscores, tags)
            if (c >= 32 && c <= 126) {
                if (currentWord.size() < 200) {
                    currentWord.push_back(static_cast<char>(c));
                }
            } else {
                if (currentWord.size() >= 8) {
                    std::string lower = ToLower(currentWord);

                    // Filter for startup, license, entitlement, or config file references
                    if (lower.find("license") != std::string::npos ||
                        lower.find("entitlement") != std::string::npos ||
                        lower.find("xstore") != std::string::npos ||
                        lower.find("socialclub") != std::string::npos ||
                        lower.find("startup") != std::string::npos ||
                        lower.find(".xml") != std::string::npos ||
                        lower.find(".meta") != std::string::npos) {

                        std::cout << "[MATCH] " << currentWord << std::endl;
                        matchesFound++;
                        if (matchesFound >= 60) {
                            std::cout << "\n(Reached 60 matches — stopping early so terminal stays readable!)" << std::endl;
                            return 0;
                        }
                    }
                }
                currentWord.clear();
            }
        }
    }

    std::cout << "\nScan complete! Total matching strings found: " << matchesFound << std::endl;
    return 0;
}