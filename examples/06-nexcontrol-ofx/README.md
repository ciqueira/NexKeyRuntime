# NexControl Minimal OpenFX Example (06-nexcontrol-ofx)

This example demonstrates how to integrate the **NexControl** real-time MIDI hardware control subsystem into a native OpenFX (OFX) plugin.

---

## 1. Architecture: Client & Hub Model

NexControl operates on an asynchronous Client-Hub architecture:
- **OpenFX Plugin (Client)**: Links the lightweight `libnexcontrol.a` library. It contains zero MIDI drivers, opens no raw device ports, and requires no OS Accessibility permissions.
- **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus) (Hub)**: Runs in the background to handle physical USB/BLE MIDI device enumeration, profile mapping, and encoder delta processing. It streams parameter mutations to the focused OFX plugin via low-latency binary IPC (Unix Domain Sockets on macOS, Named Pipes on Windows).

---

## 2. What It Demonstrates

1. **Subsystem Initialization**:
   `mc::nexcontrol::init(OFX::Private::gHost)` attaches the client dispatcher to the host's main thread event loop.
2. **Instance Slot Binding**:
   `std::make_unique<mc::nexcontrol::InstanceBinding>(handle)` allocates an isolated slot for the active node.
3. **Parameter Registration**:
   - `exposure`: Continuous rotary slider (`NEXCONTROL_PARAM_DOUBLE`).
   - `contrast`: Continuous rotary slider (`NEXCONTROL_PARAM_DOUBLE`).
   - `color_mode`: Discrete dropdown option stepping (`NEXCONTROL_PARAM_CHOICE`).
   - `bypass`: Toggle switch (`NEXCONTROL_PARAM_BOOL`).
4. **Active Node Focus Anchor**:
   The string parameter `instance_info` placed inside the `grpNexControl` group acts as an in-process focus anchor for DaVinci Resolve's Inspector.
5. **Multi-Plugin Node Isolation**:
   Instance identifiers are allocated via process-scoped shared memory (`/nexcontrol_seq_<PID>`), avoiding parameter collisions across distinct plugins on the timeline.

---

## 3. Prerequisites & Requirements

### Build-Time
- **Compiler**: C++17 compliant compiler (AppleClang 14+, GCC 10+, or MSVC 2019+).
- **CMake**: Version 3.20 or newer.
- **OpenFX SDK**: Automatically fetched via CMake `FetchContent` (or specified via `-DOFX_ROOT=/path/to/openfx`).
- **Client Library (`libnexcontrol.a` / `nexcontrol.lib`)**: Place in `../../lib/` or point via `-DNEXCONTROL_LIB=/path/to/libnexcontrol.a`.

### Runtime (Hardware Testing)
- **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)**: Must be running in the background to act as the hardware MIDI hub.
- **MIDI Controller**: A connected physical MIDI control surface (e.g. Midi Fighter Twister or custom controller profile).

---

## 4. Building the Plugin

### macOS (Terminal)
```bash
cmake -S examples/06-nexcontrol-ofx -B examples/06-nexcontrol-ofx/build
cmake --build examples/06-nexcontrol-ofx/build -j8
```
*Artifact output:* `examples/06-nexcontrol-ofx/build/NexControlSamplePlugin.ofx.bundle`

### Windows (PowerShell / Developer Command Prompt)
```cmd
cmake -S examples/06-nexcontrol-ofx -B examples/06-nexcontrol-ofx/build
cmake --build examples/06-nexcontrol-ofx/build --config Release
```
*Artifact output:* `examples\06-nexcontrol-ofx\build\Release\NexControlSamplePlugin.ofx`

---

## 5. Installing into DaVinci Resolve

### macOS:
```bash
sudo cp -R examples/06-nexcontrol-ofx/build/NexControlSamplePlugin.ofx.bundle /Library/OFX/Plugins/
```

### Windows:
```cmd
copy examples\06-nexcontrol-ofx\build\Release\NexControlSamplePlugin.ofx "C:\Program Files\Common Files\OFX\Plugins\"
```

---

## 6. Interactive Testing with Hardware

1. Connect your physical MIDI controller via USB or Bluetooth.
2. Launch the **[MCNexus Desktop Application](https://github.com/ciqueira/MCNexus)** (confirming that the controller is recognized).
3. Open **DaVinci Resolve**, navigate to the **Color** page, and add **NexControl Sample Plugin** to a node.
4. Select the node so its Inspector controls become visible.
5. Rotate or press mapped controls:
   - **Knob 0**: Adjusts `Exposure`.
   - **Knob 1**: Adjusts `Contrast`.
   - **Knob 2**: Steps through `Color Mode` options.
   - **Knob/Switch 3**: Toggles `Bypass`.
6. Select a different node on the timeline and confirm that hardware movements route only to the focused instance.
