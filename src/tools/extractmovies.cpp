#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

struct VideoClip {
    int index;
    uint64_t offset;
    uint64_t size;
};

int main() {
    std::string rpfPath = "C:\\Users\\aiden\\Downloads\\Grand Theft Auto VI\\movies.rpf";
    std::ifstream file(rpfPath, std::ios::binary);
    if (!file) {
        std::cout << "[ERROR] Could not open: " << rpfPath << std::endl;
        return 1;
    }

    std::cout << "=== EXTRACTING ALL 597 BINK2 (KB2n) VIDEOS TO D:\\all_bk2_videos ===" << std::endl;

    const size_t SECTOR_SIZE = 2048;
    std::vector<uint8_t> sector(SECTOR_SIZE);
    std::vector<VideoClip> clips;

    uint64_t offset = 0;
    int vidNum = 0;

    while (file.read(reinterpret_cast<char*>(sector.data()), SECTOR_SIZE)) {
        if (sector.at(0) == 'K' && sector.at(1) == 'B' && sector.at(2) == '2' && sector.at(3) == 'n') {
            uint32_t rawSize = *reinterpret_cast<uint32_t*>(&sector.at(4));
            uint64_t totalVideoBytes = static_cast<uint64_t>(rawSize) + 8;

            if (totalVideoBytes > 100 * 1024 && totalVideoBytes < 1024ULL * 1024 * 1024) {
                vidNum++;
                clips.push_back({ vidNum, offset, totalVideoBytes });
            }
        }
        offset += SECTOR_SIZE;
    }

    std::cout << "Found " << clips.size() << " verified videos. Extracting to D:\\all_bk2_videos\\...\n" << std::endl;

    fs::create_directories("D:\\all_bk2_videos");
    std::ofstream listFile("D:\\all_bk2_videos\\list.txt");
    file.clear();

    std::vector<char> copyBuf(8 * 1024 * 1024);

    for (const auto& clip : clips) {
        std::ostringstream ss;
        ss << "D:\\all_bk2_videos\\movie_" << std::setw(3) << std::setfill('0') << clip.index << ".bk2";
        std::string outName = ss.str();

        std::ofstream outFile(outName, std::ios::binary);
        file.seekg(clip.offset, std::ios::beg);
        uint64_t remaining = clip.size;

        while (remaining > 0 && file) {
            size_t toRead = static_cast<size_t>(std::min<uint64_t>(copyBuf.size(), remaining));
            file.read(copyBuf.data(), toRead);
            outFile.write(copyBuf.data(), file.gcount());
            remaining -= file.gcount();
        }

        listFile << "file '" << outName << "'\n";

        if (clip.index % 50 == 0 || clip.index == static_cast<int>(clips.size())) {
            std::cout << "[PROGRESS] Extracted " << clip.index << " / " << clips.size() << " videos..." << std::endl;
        }
    }

    std::cout << "\nDone! All " << clips.size() << " videos + list.txt saved to D:\\all_bk2_videos\\" << std::endl;
    return 0;
}