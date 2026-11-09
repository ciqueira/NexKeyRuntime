# NexKey Sandbox Reference OFX Plugin

The **NexKey Sandbox Plugin** is the OpenFX reference implementation designed to run inside **DaVinci Resolve**, **Filmlight Baselight**, **Assimilate Scratch**, and **Adobe Premiere Pro**.

It consolidates all NexKeyRuntime capabilities into a single interactive interface:
1. **Profile A Zero-Overhead Licensing:** `< 1 ns` atomic check per frame.
2. **Watermarking Engine:** Subtle CPU watermark rendered in `demo` mode; clean rendering in `trial` and `full`.
3. **Inspector Telemetry:** Live display of plugin version, license status, active edition, and receipts filesystem path.
4. **Update Discovery:** In-panel release status (`v1.0.0 (Up to date)` or `Update Available: v1.1.0`) with a "Check for Updates" trigger and one-click update launcher.
5. **Product Notices:** Delivery of vendor advisories targeted to DaVinci Resolve with an in-panel dismiss action.
6. **Hardware MIDI Control (NexControl):** Real-time rotary knob and toggle control for effect parameters (`exposure` and `invert`) with in-process active node tracking.

---

## 1. Prerequisites & Requirements

Before building the plugin, ensure your environment meets the following requirements:

- **Compiler**: C++17 compliant compiler (AppleClang 14+, GCC 10+, or MSVC 2019+).
- **CMake**: Version 3.20 or newer.
- **OpenFX SDK**: The build automatically fetches OpenFX headers and support sources via CMake `FetchContent`. If you maintain a local copy of OpenFX, you can point to it directly:
  ```bash
  cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build -DOFX_ROOT=/path/to/openfx
  ```
- **Static Libraries (`libnexkeyruntime.a` and `libnexcontrol.a`)**:
  - The plugin links statically against `libnexkeyruntime` and `libnexcontrol`.
  - Download precompiled static libraries for your platform from [GitHub Releases](https://github.com/ciqueira/NexKeyRuntime/releases) and place them in the `lib/` directory, or pass `-DNEXKEYRUNTIME_LIB=...` and `-DNEXCONTROL_LIB=...`.
  - *macOS*: Links system frameworks (`CoreFoundation`, `IOKit`, `Security`, `Foundation`, `AppKit`).
  - *Windows*: Visual Studio with "Desktop development with C++"; automatically compiles with the static C runtime (`/MT`).
- **Runtime Requirement for Hardware MIDI Testing**:
  - Offline licensing verification and update discovery function independently via `nexkeyctl`.
  - Testing real-time hardware MIDI control (`NexControl`) requires the **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)** running in the background to serve as the hardware hub bridging physical controllers to the plugin via local binary IPC.

---

## 2. Building the Plugin

### macOS (Terminal)
```bash
cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build
cmake --build sandbox/ofx-plugin/build -j8
```
*Artifact output:* `sandbox/ofx-plugin/build/NexKeySandboxPlugin.ofx.bundle`

### Windows (PowerShell / Developer Command Prompt)
```cmd
cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build
cmake --build sandbox/ofx-plugin/build --config Release
```
*Artifact output:* `sandbox\ofx-plugin\build\Release\NexKeySandboxPlugin.ofx`

---

## 3. Installing into DaVinci Resolve

### macOS:
```bash
sudo cp -R sandbox/ofx-plugin/build/NexKeySandboxPlugin.ofx.bundle /Library/OFX/Plugins/
```

### Windows:
```cmd
copy sandbox\ofx-plugin\build\Release\NexKeySandboxPlugin.ofx "C:\Program Files\Common Files\OFX\Plugins\"
```

---

## 4. Interactive Testing inside DaVinci Resolve

1. Open **DaVinci Resolve**, navigate to the **Color** page, and apply **NexKey Sandbox Plugin** to a node.
2. Observe the initial state: without an active license, the effect is in bypass mode.
3. Open your terminal and activate a test key:
   ```bash
   # Activate Demo license (permanent watermark):
   ./tools/nexkeyctl/build/nexkeyctl --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08

   # In Resolve, click "Refresh License" to observe the watermark and DEMO status!

   # Activate Full license (unlocked commercial):
   ./tools/nexkeyctl/build/nexkeyctl --activate --key=B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D

   # In Resolve, click "Refresh License": watermark disappears!
   ```
4. Click **"Check for Updates"** in the Inspector to query the Sandbox Gateway release manifest.
5. **Test Real-Time Hardware MIDI Control (NexControl):**
   - Connect a supported MIDI controller (e.g., Midi Fighter Twister) and launch the **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)** in the background.
   - Select the node in DaVinci Resolve:
     * Rotate encoder 0 to adjust `Exposure`.
     * Toggle switch 1 to invert colors (`Invert Colors`).
     * Note that the timeline viewer refreshes in real-time even during playback pause.
