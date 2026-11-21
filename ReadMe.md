# MyXboxRunner

A modular C++17 High-Level Emulation (HLE) compatibility layer and 64-bit PE loader for running x86-64 Xbox Series X|S (Scarlett) GDK executables natively on Windows 11 with DirectX 12 and full Keyboard & Mouse (KBM) translation.

> **Disclaimer:** This repository contains only original C++ source code and analysis tooling. No copyrighted game binaries, assets, media files, or cryptographic keys are included.

---

## Repository Structure

```text
MyXboxRunner/
├── src/
│   ├── loader/          # 64-bit PE memory mapper, IAT hooker & test harness
│   │   ├── loader.cpp
│   │   └── main.cpp
│   ├── shims/           # Xbox Series X|S -> Windows 11 translation DLLs
│   │   ├── shim_router.cpp
│   │   ├── shim_kernelx.cpp
│   │   ├── shim_d3d12.cpp
│   │   ├── shim_input.cpp
│   │   └── shim_audio.cpp
│   └── tools/           # Standalone PE, archive & media inspection utilities
│       ├── inspect.cpp
│       ├── stubgen.cpp
│       ├── dumpcheck.cpp
│       ├── findmovies.cpp
│       ├── extractmovies.cpp
│       ├── bink2mp4.cpp
│       ├── findshaders.cpp
│       └── stringcheck.cpp
├── build.bat            # One-click MSVC build script (outputs to .\build\)
├── README.md
└── .gitignore
```

---

## Components

### Runtime & Loader (`src/loader/` & `src/shims/`)
* **`loader.exe`:** Maps 64-bit PE executables into memory, applies base relocations, patches the Import Address Table (IAT) to route Xbox DLLs through `shim_router.dll`, and installs a Vectored Exception Handler to report crash RVAs.
* **`shim_router.dll`:** Master import switchboard that routes real Xbox DLL names (`kernelx.dll`, `xgameruntime.dll`, `xmem.dll`, `d3d12_xs.dll`, `GameInput.dll`, `xaudio2_9.dll`) to local shims and logs every resolved or missing entry point to `runner_log.txt`.
* **`shim_kernelx.dll`:** Translates Xbox virtual drives (`G:\`, `U:\`, `T:\`) to host directories, implements `xmem.dll` memory functions (`XMemAlloc`, `XMemFree`, `XMemGetWorkingSetStatistics`, `XMemVirtualQuery`), provides offline user/title identity, high-precision timers, 7-core CPU reporting, and Series S (8GB VRAM) device profile reporting.
* **`shim_d3d12.dll`:** Forwards DirectX 12 device creation (`D3D12CreateDevice`) directly to native Windows 11 `d3d12.dll` and manages the display window.
* **`shim_input.dll`:** Translates PC Keyboard & Mouse inputs into a 16-button + dual-thumbstick Xbox controller state, customizable via an auto-generated `controls.ini` file.
* **`shim_audio.dll`:** Connects Xbox audio initialization directly to Windows `XAudio2.9`.

### Inspection & Analysis Tools (`src/tools/`)
* **`inspect.exe`:** Scans 64-bit PE files and dumps imported DLLs and function names.
* **`stubgen.exe`:** Auto-generates safe C++ `extern "C"` stub exports from `inspect.exe` output.
* **`dumpcheck.exe`:** Recursively inspects unpacked GDK directories for manifests, middleware DLLs, and archive magic headers (`RPF9` / `RDS5`).
* **`findmovies.exe` / `extractmovies.exe` / `bink2mp4.exe`:** Scans, extracts, and pipes sector-aligned Bink 2 (`KB2n`) video streams through `Bink2Scarlett.dll` and FFmpeg.

---

## Building From Source

### Prerequisites
* Windows 10/11 (x64)
* Visual Studio 2022 / Build Tools with **Desktop development with C++** (`cl.exe` in PATH)

### Build Command
Open **Developer PowerShell for VS** inside the repository root and run:

```powershell
.\build.bat
```

All compiled `.exe` and `.dll` targets are output cleanly to `.\build\`.

---

## Usage

1. **Test the PE Loader & Shim Pipeline:**
   ```powershell
   cd build
   .\loader.exe .\main.exe
   ```
2. **Scan a Target Executable:**
   ```powershell
   .\build\inspect.exe "C:\(\path\to\target.\)exe" > imports.txt
   ```
3. **Generate Dummy Stubs for Unimplemented Imports:**
   ```powershell
   .\build\stubgen.exe imports.txt auto_stubs.cpp
   ```