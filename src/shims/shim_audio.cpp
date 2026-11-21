#include <windows.h>
#include <xaudio2.h>
#include <iostream>
#include <cmath>
#include <vector>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

IXAudio2* xaudioEngine = nullptr;
IXAudio2MasteringVoice* masteringVoice = nullptr;

extern "C" {

    // 1. Initializes the Xbox Audio Engine -> Connects to Windows XAudio2.9
    __declspec(dllexport) bool XAudio2Create_Shim() {
        std::cout << "[shim_audio.dll] Initializing Xbox Audio Engine (XAudio2.9)..." << std::endl;

        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        if (FAILED(XAudio2Create(&xaudioEngine, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
            std::cout << "[ERROR] Failed to create XAudio2 engine!" << std::endl;
            return false;
        }

        if (FAILED(xaudioEngine->CreateMasteringVoice(&masteringVoice))) {
            std::cout << "[ERROR] Failed to create audio mastering voice!" << std::endl;
            return false;
        }

        std::cout << "[shim_audio.dll] Connected to Windows PC Speakers/Headphones!" << std::endl;
        return true;
    }

    // 2. Plays a quick test tone (verifies audio buffers stream to your speakers!)
    __declspec(dllexport) void XAudio2PlayBootChime() {
        if (!xaudioEngine) return;

        const int sampleRate = 44100;
        const int durationMs = 250;
        const int numSamples = (sampleRate * durationMs) / 1000;
        std::vector<int16_t> pcmData(numSamples);

        // Generate a clean 523.25 Hz (C5) chime wave
        for (int i = 0; i < numSamples; i++) {
            double t = static_cast<double>(i) / sampleRate;
            double fade = 1.0 - (static_cast<double>(i) / numSamples); // Smooth fade out
            pcmData[i] = static_cast<int16_t>(sin(2.0 * 3.14159265 * 523.25 * t) * 12000.0 * fade);
        }

        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = sampleRate;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = 2;
        wfx.nAvgBytesPerSec = sampleRate * 2;

        IXAudio2SourceVoice* sourceVoice = nullptr;
        if (SUCCEEDED(xaudioEngine->CreateSourceVoice(&sourceVoice, &wfx))) {
            XAUDIO2_BUFFER buffer = {};
            buffer.AudioBytes = static_cast<UINT32>(pcmData.size() * sizeof(int16_t));
            buffer.pAudioData = reinterpret_cast<const BYTE*>(pcmData.data());
            buffer.Flags = XAUDIO2_END_OF_STREAM;

            sourceVoice->SubmitSourceBuffer(&buffer);
            sourceVoice->Start(0);
            Sleep(durationMs + 30);
            sourceVoice->DestroyVoice();
        }
    }

    // 3. Clean Audio Shutdown
    __declspec(dllexport) void XAudio2Shutdown_Shim() {
        if (masteringVoice) { masteringVoice->DestroyVoice(); masteringVoice = nullptr; }
        if (xaudioEngine)   { xaudioEngine->Release(); xaudioEngine = nullptr; }
        CoUninitialize();
        std::cout << "[shim_audio.dll] Audio engine shut down cleanly." << std::endl;
    }
}