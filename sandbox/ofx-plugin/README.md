# NexKey Sandbox Reference OFX Plugin

The **NexKey Sandbox Plugin** is the OpenFX reference implementation designed to run inside **DaVinci Resolve**, **Filmlight Baselight**, **Assimilate Scratch**, and **Adobe Premiere Pro**.

It consolidates NexKeyRuntime capabilities into a single interactive interface:
1. **Profile A Zero-Overhead Licensing:** `< 1 ns` atomic check per frame.
2. **Watermarking Engine:** Subtle CPU watermark rendered in `demo` mode; clean rendering in `trial` and `full`.
3. **Inspector Telemetry:** Live display of plugin version, license status, active edition, and receipts filesystem path.
4. **Update Discovery:** In-panel release status (`v1.0.0 (Up to date)` or `Update Available: v1.1.0`) with a "Check for Updates" trigger and one-click update launcher.
5. **Product Notices:** Delivery of vendor advisories targeted to DaVinci Resolve with an in-panel dismiss action.
6. **Hardware MIDI Control (NexControl, optional):** Real-time rotary knob and toggle control for effect parameters (`exposure` and `invert`) with in-process active node tracking when `libnexcontrol` is available.

All commands below assume your working directory is the **repository root** (`NexKeyRuntime/`).

---

## 1. Prerequisites & Requirements

Before building the plugin, ensure your environment meets the following requirements:

- **Compiler**: C++17 compliant compiler (AppleClang 14+, GCC 10+, or MSVC 2019+).
- **CMake**: Version 3.20 or newer.
- **OpenFX SDK**: The build automatically fetches OpenFX headers and support sources via CMake `FetchContent`. If you maintain a local copy of OpenFX, pass `-DOFX_ROOT=/path/to/openfx`.
- **Static Library (`libnexkeyruntime.a` / `nexkeyruntime.lib`, required)**:
  - Download the precompiled static library for your platform from [GitHub Releases](https://github.com/ciqueira/NexKeyRuntime/releases) and place it in `<repo>/lib/` (`lib/libnexkeyruntime.a` on macOS, `lib/nexkeyruntime.lib` on Windows), or pass `-DCMAKE_PREFIX_PATH=...` / `-DNEXKEYRUNTIME_LIB=...`.
  - *macOS*: Links system frameworks (`CoreFoundation`, `IOKit`, `Security`, `Foundation`, `AppKit`).
  - *Windows*: Visual Studio with "Desktop development with C++"; automatically compiles with the static C runtime (`/MT`).
- **NexControl Client Library (`libnexcontrol.a` / `nexcontrol.lib`, optional)**:
  - If placed in `<repo>/lib/` (or pointed via `-DNEXCONTROL_LIB=...`), the plugin automatically enables the **NexControl MIDI Hardware** group; otherwise it builds cleanly with licensing, updates, and notices enabled.
- **Runtime Requirement for Hardware MIDI Testing**:
  - Offline licensing verification and update discovery function independently via `nexkeyctl`.
  - Testing real-time hardware MIDI control (`NexControl`) requires the **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)** running in the background to serve as the hardware hub bridging physical controllers to the plugin via local binary IPC.

---

## 2. Building the Plugin

### Recommended: Unified Root Build

**macOS (Terminal):**
```bash
cmake -S . -B build -DNEXKEY_BUILD_OFX=ON
cmake --build build -j8
```
*Artifact output:* `build/plugins/NexKeySandboxPlugin.ofx.bundle`

**Windows (PowerShell / Developer Command Prompt):**
```cmd
cmake -S . -B build -A x64 -DNEXKEY_BUILD_OFX=ON
cmake --build build --config Release
```
*(Use `-A ARM64` instead of `-A x64` if you downloaded `windows-arm64.zip`; `-A` must match your downloaded release archive architecture.)*
*Artifact output:* `build\plugins\Release\NexKeySandboxPlugin.ofx.bundle`

### Optional: Standalone Plugin Build
```bash
cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build
cmake --build sandbox/ofx-plugin/build -j8
```

---

## 3. Installing into DaVinci Resolve

### macOS:
```bash
sudo cp -R build/plugins/NexKeySandboxPlugin.ofx.bundle /Library/OFX/Plugins/
```

### Windows (PowerShell as Admin):
```powershell
Copy-Item -Recurse build\plugins\Release\NexKeySandboxPlugin.ofx.bundle "C:\Program Files\Common Files\OFX\Plugins\"
```

---

## 4. Interactive Testing inside DaVinci Resolve

1. Open **DaVinci Resolve**, navigate to the **Color** page, and apply **NexKey Sandbox Plugin** to a node.
2. Observe the initial state: without an active license, the effect is in bypass mode.
3. From the repository root, activate a test key using `nexkeyctl`:
   ```bash
   # Activate Demo license (permanent watermark):
   ./build/bin/nexkeyctl --activate \
     --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08 \
     --tenant=mcnexus-sandbox \
     --product-file=sandbox/mcnexus-sandbox.nexkeydat

   # In Resolve, click "Refresh License" to observe the watermark and DEMO status!

   # Activate Full license (unlocked commercial):
   ./build/bin/nexkeyctl --activate \
     --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D \
     --tenant=mcnexus-sandbox \
     --product-file=sandbox/mcnexus-sandbox.nexkeydat

   # In Resolve, click "Refresh License": watermark disappears!
   ```
   *(On Windows, use `.\build\bin\Release\nexkeyctl.exe`.)*
4. Click **"Check for Updates"** in the Inspector to query the Sandbox Gateway release manifest.
5. **Test Real-Time Hardware MIDI Control (NexControl, when built with `libnexcontrol`):**
   - Connect a supported MIDI controller (e.g., Midi Fighter Twister) and launch the **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)** in the background.
   - Select the node in DaVinci Resolve:
     * Rotate encoder 0 to adjust `Exposure`.
     * Toggle switch 1 to invert colors (`Invert Colors`).
     * Note that the timeline viewer refreshes in real-time even during playback pause.
