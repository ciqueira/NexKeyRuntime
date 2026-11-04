# NexKeyRuntime DCTL-to-OFX Conversion Sandbox

The **DCTL-to-OFX Conversion Sandbox** is a dedicated evaluation testbed for packaging **DCTL (DaVinci Color Transform Language)** color algorithms and 3D LUTs into native **OpenFX (OFX)** plugin bundles for DaVinci Resolve Studio (macOS and Windows).

This environment operates entirely offline using pre-provisioned sandbox credentials: no external network connection, account creation, or production keys are required.

---

## 1. Architecture & Precompiled Runners

The sandbox distribution bundles precompiled, immutable OpenFX runner binaries for macOS and Windows directly inside this repository:

```
sandbox/dctl-to-ofx/
├── runners/
│   ├── macos/NexDCTLRunner.ofx      # Universal binary (arm64 Apple Silicon + x86_64 Intel)
│   └── windows/NexDCTLRunner.ofx    # 64-bit PE DLL (Windows x64 with OpenCL)
├── examples/
│   └── 01-exposure/                 # Basic exposure and gamma slider example
└── scripts/
    ├── package_sandbox.sh           # macOS packaging helper
    └── package_sandbox.bat          # Windows drag-and-drop packaging helper
```

### The 4 Sandbox Guarantees
1. **Zero C++ Compilation for Creators**: The colorist writes only standard DCTL math (`transform()`) and UI definitions (`DEFINE_UI_PARAMS`).
2. **Immutable Universal Runners**: `NexDCTLRunner.ofx` contains no hardcoded shaders. Shaders are encrypted into `payload.nexasset` and dynamically compiled on GPU at instance creation.
3. **Hardware Acceleration**: Metal on macOS (with automatic fallback to OS JIT compiler if Command Line Tools are absent) and OpenCL on Windows via the GPU video driver.
4. **Deterministic Sandbox Isolation**: Tenant locked to `mcnexus-sandbox` with pre-embedded test keys.

---

## 2. Developer Workflow (CLI)

### Step 1: Package DCTL into an OpenFX Bundle

Run `nexkeyctl package-dctl` with the `--sandbox` flag pointing to the precompiled runner for your platform:

**macOS:**
```bash
nexkeyctl package-dctl \
  --input="examples/01-exposure/Exposure.dctl" \
  --name="Exposure Simple" \
  --runner="runners/macos/NexDCTLRunner.ofx" \
  --output="./dist" \
  --sandbox
```

**Windows:**
```cmd
nexkeyctl.exe package-dctl ^
  --input="examples\01-exposure\Exposure.dctl" ^
  --name="Exposure Simple" ^
  --runner="runners\windows\NexDCTLRunner.ofx" ^
  --output=".\dist" ^
  --sandbox
```

#### Debug & Diagnostic Logging (`--debug`)
To inspect shader compilation, macro preprocessing, and GPU kernel logs, append `--debug`:
```bash
nexkeyctl package-dctl \
  --input="examples/01-exposure/Exposure.dctl" \
  --runner="runners/macos/NexDCTLRunner.ofx" \
  --output="./dist" \
  --sandbox \
  --debug
```
* **Log file destination:** `/tmp/nexdctl_sandbox.log` (macOS/Linux) or `%TEMP%\nexdctl_sandbox.log` (Windows).
* *Zero playback stalls:* Diagnostic logging only captures initialization and shader compile errors; the hot render path runs with zero I/O.

---

### Step 2: Install Bundle into DaVinci Resolve

Move the generated `.ofx.bundle` to the standard OpenFX directory:

* **macOS:**
  ```bash
  sudo cp -R ./dist/ExposureSimple.ofx.bundle /Library/OFX/Plugins/
  ```
* **Windows (PowerShell as Admin):**
  ```powershell
  Copy-Item -Recurse .\dist\ExposureSimple.ofx.bundle "C:\Program Files\Common Files\OFX\Plugins\"
  ```

---

### Step 3: Validate License Behavior in DaVinci Resolve

1. **Default Mode (Demo / Watermark):**
   - Apply the plugin to a node in DaVinci Resolve.
   - Without an active sandbox license key, the runner displays a procedural test watermark while maintaining full interactive controls.

2. **Full Mode (Watermark Removed):**
   - Activate the pre-provisioned Sandbox Full Commercial test key:
     ```bash
     nexkeyctl --activate \
       --key="B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D" \
       --tenant="mcnexus-sandbox" \
       --product-file="../mcnexus-sandbox.nexkeydat"
     ```
   - Reload or scrub the timeline in DaVinci Resolve: the watermark disappears and rendering is clean.

---

## 3. Supported DCTL Macros

The packager automatically preprocesses standard DaVinci Resolve DCTL macros:

| Macro | Description | Syntax |
| :--- | :--- | :--- |
| `DEFINE_UI_PARAMS` | Float Slider | `DEFINE_UI_PARAMS(id, Label, DCTLUI_SLIDER_FLOAT, default, min, max, step)` |
| `DEFINE_UI_PARAMS` | Checkbox | `DEFINE_UI_PARAMS(id, Label, DCTLUI_CHECK_BOX, default)` |
| `DEFINE_UI_PARAMS` | Combobox | `DEFINE_UI_PARAMS(id, Label, DCTLUI_COMBO_BOX, default, enum_0, enum_1, ...)` |
| `DEFINE_UI_PARAMS` | Color Picker | `DEFINE_UI_PARAMS(id, Label, DCTLUI_COLOR_PICKER, def_r, def_g, def_b)` |
| `DEFINE_UI_TOOLTIP` | Inspector Tooltip | `DEFINE_UI_TOOLTIP(id, "Help text")` |
| `DEFINE_LUT` | 3D Texture LUT | `DEFINE_LUT(id, "relative_path.cube")` |
| `#include` | Header Inlining | Recursive file include (up to 16 levels deep) |

---

## 4. Helper Packaging Scripts (macOS & Windows)

For rapid packaging without typing CLI flags, helper scripts are provided in `scripts/`:

* **macOS:**
  ```bash
  ./scripts/package_sandbox.sh examples/01-exposure/Exposure.dctl
  ```
  *(Or double-click `package_sandbox.sh` in Finder and drag the `.dctl` file into the terminal prompt).*

* **Windows:**
  ```cmd
  scripts\package_sandbox.bat examples\01-exposure\Exposure.dctl
  ```
  *(Or drag and drop your `.dctl` file directly onto `package_sandbox.bat` in Windows Explorer).*

Both scripts automatically detect the local precompiled runner and assemble the `.ofx.bundle` into `dist/`.

---

## 5. Pre-Provisioned Test License Keys

All keys below are pre-registered on the Sandbox authority (`../mcnexus-sandbox.nexkeydat`):

| Edition | Key String | Expected Host Behavior |
| :--- | :--- | :--- |
| **Demo** | `2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08` | Procedural watermark overlay |
| **Full** | `B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D` | Clean rendering, watermark removed |
| **Trial** | `7ACDD8-B11617-0BF5F2-6CECF4-AD6BD9-B2704997` | Clean rendering for 7-day countdown |
| **Expired** | `A00B07-DE23BD-7F048D-94F064-C7D13D-FADF4353` | Render blocked (`DENY` bypass) |

*For complete metadata, inspect [`../licenses.json`](../licenses.json).*
