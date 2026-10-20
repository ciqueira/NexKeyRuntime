# NexKeyRuntime SDK Sandbox

Welcome to the **NexKeyRuntime Developer Sandbox**. This directory contains the official test credentials, pre-provisioned license keys, and instructions to validate native licensing, updates, and notices against the isolated **MCNexus Sandbox Gateway**.

The Sandbox environment runs on dedicated test infrastructure: activations, deactivations, and test transactions made here do not affect production metrics, customer databases, or Stripe live accounts.

---

## 1. Sandbox Architecture & Endpoints

| Resource | Value | Description |
| :--- | :--- | :--- |
| **Tenant ID** | `mcnexus-sandbox` | The test tenant identifier for developer onboarding. |
| **SDK Gateway** | `https://sdk-sandbox.mcnexus.app` | Issues Ed25519-signed activation certificates and manifests. |
| **Default Entitlement** | `default` | Base variant slug for SDK plugins and applications. |
| **Keyring Key ID** | `mcnexus-sandbox-sandbox-2026-10` | Active Ed25519 signing key for this sandbox period. |
| **ProductData File** | `mcnexus-sandbox.nexkeydat` | Base64-signed trust envelope containing public keyring and gateway URL. |

---

## 2. Pre-Provisioned Test License Keys

All keys are pre-registered on the Sandbox Gateway and ready for immediate testing without payment:

| Role | License Key | Edition | Expected Behavior |
| :--- | :--- | :--- | :--- |
| **Demo** | `2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08` | `demo` | Permanent watermark required. Render decision returns `ALLOW` indefinitely. |
| **Standard Trial** | `7ACDD8-B11617-0BF5F2-6CECF4-AD6BD9-B2704997` | `trial` | 7-day countdown starting upon first activation. Clean rendering. |
| **Extended Trial** | `BFE30C-87617B-ECD794-267E82-39A65F-A330A912` | `trial` | 730-day (2-year) countdown for long-term QA and continuous integration. |
| **Expired Trial** | `A00B07-DE23BD-7F048D-94F064-C7D13D-FADF4353` | `trial` | Activation refused or render blocked with `E_LICENSE_EXPIRED` (`DENY`). |
| **Full Commercial** | `B11C74-8A87F8-C8EA8A-6AE757-4A62D3-89FED38D` | `full` | 100% unlocked rendering, no watermarks, perpetual validity. |
| **Suspended / Revoked** | `AF65F0-A84D1C-51F3B6-2E3139-4C5931-534E28FA` | `full` | Server immediately refuses activation or denies render (`E_LICENSE_SUSPENDED`). |

*Full machine-readable metadata is available in [licenses.json](licenses.json).*

---

## 3. The Two Sandbox Testbeds

The Sandbox provides two unified showcases to test all features:

1. **Host & Visual Showcase:** [**`sandbox/ofx-plugin/`**](ofx-plugin/)
   - Real OpenFX plugin bundle (`NexKeySandboxPlugin.ofx.bundle`) that installs directly into DaVinci Resolve or Premiere Pro.
   - Allows testing atomic render decisions (`< 1 ns`), CPU demo watermarks, license edition changes, update notifications, and vendor notices live in the host Inspector.
2. **Terminal & Autonomous Showcase:** [**`tools/nexkeyctl/`**](../tools/nexkeyctl/)
   - Command-line tool to activate test keys, inspect local receipts, release seats, check updates, and test offline air-gapped exports.

```bash
# Activate a Sandbox Demo license:
./tools/nexkeyctl/build/nexkeyctl --activate --key=2D3CD5-45AEE1-B61D54-86DB2C-7307CD-A7664D08

# Check local status:
./tools/nexkeyctl/build/nexkeyctl --status
```

---

## 4. Environment Variables (Optional Helper)

You can configure your terminal session so tools automatically pick up sandbox credentials:

```bash
export NEXKEYRUNTIME_TENANT="mcnexus-sandbox"
export NEXKEYRUNTIME_PRODUCT_FILE="$PWD/sandbox/mcnexus-sandbox.nexkeydat"
```

---

## 5. AI Prompts

<details>
<summary>Click to view ready-to-use prompts for Cursor, Claude Code, Copilot, and ChatGPT</summary>

### Prompt: Profile A Integration (OFX Plugin)
```text
I am developing an OpenFX (OFX) plugin in C++.
I need to integrate native offline licensing using NexKeyRuntime SDK (Profile A).

Key Architectural Rules:
1. In Profile A, the plugin NEVER connects to the internet or makes network requests during render. Activation is handled externally (by MCNexus App or the nexkeyctl CLI tool).
2. During plugin load/constructor, initialize the license handle:
   - Load the ProductData trust envelope from "sandbox/mcnexus-sandbox.nexkeydat".
   - Set tenant ID to "mcnexus-sandbox" and variant to "default".
   - Call nexkeyruntime_license_load_local() to read the signed Ed25519 receipt from disk.
3. In the render callback (kOfxImageEffectActionRender):
   - Query nexkeyruntime_license_render_decision(handle) (this is an atomic < 1 ns memory check).
   - If DENY: bypass processing (passthrough input to output untouched).
   - If ALLOW: process the effect normally.
   - If the active edition is "demo", overlay a visible watermark or demo banner on the output frame.
   - If "trial" or "full", render clean image with no watermarks.
4. In describeInContext():
   - Add a read-only parameter group "License Diagnostics" showing the plugin version, license status (ACTIVE, EXPIRED, etc.), active edition, and render decision.
5. On plugin unload/destroy:
   - Call nexkeyruntime_license_destroy(handle).
```

### Prompt: Profile B Integration (Autonomous In-App Activation)
```text
I am building a native desktop application in C++.
I need to implement autonomous license activation and seat management using the NexKeyRuntime SDK (Profile B).

Requirements:
1. Initialize the license handle on startup:
   - Set ProductData using the signed trust envelope from "sandbox/mcnexus-sandbox.nexkeydat".
   - Set tenant ID to "mcnexus-sandbox" and variant to "default".
   - Call nexkeyruntime_license_load_local() to see if an activation already exists on this machine.
2. In the "Activate License" UI dialog:
   - When the user submits their license key string, call nexkeyruntime_license_set_license_key(handle, userKey).
   - Execute nexkeyruntime_license_activate(handle) on a background worker thread.
   - Handle the returned status: NEXKEYRUNTIME_OK, NEXKEYRUNTIME_E_LICENSE_KEY, NEXKEYRUNTIME_E_ACTIVATION_LIMIT, NEXKEYRUNTIME_E_LICENSE_EXPIRED, NEXKEYRUNTIME_E_LICENSE_SUSPENDED.
3. In the "Deactivate License" button:
   - Call nexkeyruntime_license_deactivate(handle) on a background thread to return the seat to the server.
```

### Prompt: Updates & Product Notices
```text
I need to implement background update discovery in my C++ native product using NexKeyRuntimeHandle.

Specifications:
1. Initialize NexKeyRuntimeConfig:
   - tenant_id = "mcnexus-sandbox", artifact_id = "default", base_url = "https://sdk-sandbox.mcnexus.app"
   - current_version = "1.0.0", platform = "macos" (or "windows"), channel = "stable"
2. Create the update handle using nexkeyruntime_create(&config) as a process-wide singleton.
3. When checking for updates:
   - Call nexkeyruntime_request_check(handle, 1).
   - Read snapshot via nexkeyruntime_get_snapshot().
   - If NEXKEYRUNTIME_UPDATE_AVAILABLE: display latest_version and trigger download action.
   - Read notices via nexkeyruntime_get_active_notice() and support dismiss/remind later.
```
</details>
