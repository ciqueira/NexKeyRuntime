# NexKey Sandbox Reference OFX Plugin (Profile A Showcase)

The **NexKey Sandbox Plugin** is the all-in-one OpenFX reference implementation designed to run inside **DaVinci Resolve**, **Filmlight Baselight**, **Assimilate Scratch**, and **Adobe Premiere Pro**.

It consolidates all NexKeyRuntime capabilities into a single interactive interface:
1. **Profile A Zero-Overhead Licensing:** `< 1 ns` atomic check per frame.
2. **Watermarking Engine:** Subtle CPU watermark rendered in `demo` mode; clean rendering in `trial` and `full`.
3. **Inspector Telemetry:** Live display of plugin version, license status, active edition, and receipts filesystem path.
4. **Update Discovery:** In-panel release status (`v1.0.0 (Up to date)` or `Update Available: v1.1.0`) with a "Check for Updates" trigger and one-click update launcher.
5. **Product Notices:** Delivery of vendor advisories targeted to DaVinci Resolve with an in-panel dismiss action.

---

## 1. Building the Plugin

```bash
cmake -S sandbox/ofx-plugin -B sandbox/ofx-plugin/build
cmake --build sandbox/ofx-plugin/build -j8
```

The build produces a standard OFX bundle:
- **macOS:** `sandbox/ofx-plugin/build/NexKeySandboxPlugin.ofx.bundle`
- **Windows:** `sandbox/ofx-plugin/build/NexKeySandboxPlugin.ofx`

---

## 2. Installing into DaVinci Resolve

### macOS:
```bash
sudo cp -R sandbox/ofx-plugin/build/NexKeySandboxPlugin.ofx.bundle /Library/OFX/Plugins/
```

### Windows:
```cmd
copy sandbox\ofx-plugin\build\Release\NexKeySandboxPlugin.ofx "C:\Program Files\Common Files\OFX\Plugins\"
```

---

## 3. Interactive Testing inside DaVinci Resolve

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

