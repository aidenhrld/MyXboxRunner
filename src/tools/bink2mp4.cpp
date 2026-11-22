#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

typedef void*    (__cdecl* OpenFn)(const char*, uint32_t);
typedef int      (__cdecl* DoFrameFn)(void*);
typedef void     (__cdecl* NextFn)(void*);
typedef void     (__cdecl* CloseFn)(void*);
typedef int      (__cdecl* CopyFn)(void*, void*, int, uint32_t, uint32_t, uint32_t, uint32_t);

int main() {
    HMODULE m = LoadLibraryA("Bink2Scarlett.dll");
    if (!m) {
        std::cout << "[ERROR] Failed to load Bink2Scarlett.dll! Make sure it is in the same folder." << std::endl;
        return 1;
    }

    auto BinkOpen         = (OpenFn)GetProcAddress(m, "BinkOpen");
    auto BinkDoFrame      = (DoFrameFn)GetProcAddress(m, "BinkDoFrame");
    auto BinkNextFrame    = (NextFn)GetProcAddress(m, "BinkNextFrame");
    auto BinkClose        = (CloseFn)GetProcAddress(m, "BinkClose");
    auto BinkCopyToBuffer = (CopyFn)GetProcAddress(m, "BinkCopyToBuffer");

    std::vector<fs::path> bk2Files;
    for (const auto& entry : fs::directory_iterator("D:\\all_bk2_videos")) {
        if (entry.path().extension() == ".bk2") {
            bk2Files.push_back(entry.path());
        }
    }
    std::sort(bk2Files.begin(), bk2Files.end());

    std::cout << "Found " << bk2Files.size() << " .bk2 files. Encoding via RTX 5060 NVENC -> D:\\all_597_movies.mp4...\n" << std::endl;

    const int OUT_W = 1920;
    const int OUT_H = 1080;

    // Uses NVIDIA NVENC hardware encoding (-c:v h264_nvenc) on your RTX 5060!
    const char* ffmpegCmd = "ffmpeg -y -f rawvideo -pix_fmt bgra -s 1920x1080 -r 30 -i - -c:v h264_nvenc -preset p4 -cq 20 -pix_fmt yuv420p \"D:\\all_597_movies.mp4\"";

    FILE* ffmpegPipe = _popen(ffmpegCmd, "wb");
    if (!ffmpegPipe) {
        std::cout << "[ERROR] Could not launch ffmpeg!" << std::endl;
        return 1;
    }

    std::vector<uint8_t> frameBuf(OUT_W * OUT_H * 4, 0);

    for (size_t idx = 0; idx < bk2Files.size(); idx++) {
        std::string pathStr = bk2Files.at(idx).string();
        void* bink = BinkOpen(pathStr.c_str(), 0);
        if (!bink) continue;

        const uint32_t* header = reinterpret_cast<const uint32_t*>(bink);
        uint32_t w = *(header + 0);
        uint32_t h = *(header + 1);
        uint32_t frames = *(header + 2);

        if (w <= OUT_W && h <= OUT_H && frames > 0) {
            for (uint32_t f = 0; f < frames; f++) {
                memset(frameBuf.data(), 0, frameBuf.size());
                BinkDoFrame(bink);
                uint32_t offX = (OUT_W - w) / 2;
                uint32_t offY = (OUT_H - h) / 2;
                BinkCopyToBuffer(bink, frameBuf.data(), OUT_W * 4, OUT_H, offX, offY, 3);
                fwrite(frameBuf.data(), 1, frameBuf.size(), ffmpegPipe);
                BinkNextFrame(bink);
            }
        }

        BinkClose(bink);
        if ((idx + 1) % 25 == 0 || idx + 1 == bk2Files.size()) {
            std::cout << "[NVENC PROGRESS] Encoded " << (idx + 1) << " / " << bk2Files.size() << " videos..." << std::endl;
        }
    }

    _pclose(ffmpegPipe);
    std::cout << "\nAll done! Watch D:\\all_597_movies.mp4!" << std::endl;
    return 0;
}