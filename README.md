[简体中文](README.zh_CN.md)

# Zumi Pocket

An offline, three-button pocket companion for FoloToy AI Passport (ESP32-C3,
240 x 320 portrait display, 8 MB Flash, no PSRAM). Zumi is a boy; this prototype
uses ZCO's existing ivory 2D character master, with small face-only expression
derivatives. His appearance has not been redesigned.

## Try the browser prototype

From the repository root:

```sh
python3 -m http.server 8876 --bind 127.0.0.1
```

Open <http://127.0.0.1:8876/preview/>. The checked-in WebAssembly module needs no
package install to preview. Use the three buttons below the display or keyboard
Up / Down / Enter. Hold OK / Enter for 500 ms to return home. The screen itself
has no touch controls. Keep the local server running; no external service is
needed. Gameplay continues offline after the page's local resources load.

## Phase one

- **Home:** Talk / Play / Garden, with happy, focused and cheering expressions.
- **Carbon Pop:** select one of three rows, pop CO2 with OK, protect leaves and
  sunlight. Correct hits earn 10 play points. A wrong hit or an expired 1.8-second
  target costs one of three hearts. Survive 30 seconds to finish.
- **Pocket Journey:** move between three lanes, collect green + tokens for five
  points, and jump rocks with OK. Jumps last 600 ms with a 1,000 ms cooldown.
  Survive 30 seconds to finish. Collisions cost one heart; jumping protects from
  rocks but still allows token collection.
- **Talk / Garden:** explicit unavailable-service screens and C interfaces only;
  no recording, network calls, paid AI, or real Garden synchronization.
- **Result:** OK replays; hold OK returns home. Scores are play points, not
  measured carbon savings. Session state resets on reboot; there is no save data.

The firmware and browser compile the **same C game logic and draw list**. The
browser paints with Canvas; firmware paints with LVGL through the original BSP.
Browser rendering is a behavioral preview, not proof of LCD appearance or timing.
The baseline hardware-test sources remain reference material but are not linked
into this application's entry point.

## Development and validation

Use ESP-IDF **5.5.3** from this repository's main baseline; do not use a different
SDK requirement from a Xiaozhi port. Install the five maintained skills with
`python3 tools/install_passport_skills.py --install` and check them with `--check`.

```sh
. /path/to/esp-idf-v5.5.3/export.sh
idf.py --version
./tools/validate.sh
```

For a host-only gate use `./tools/validate.sh --static`. For a firmware-only gate
use `./tools/validate.sh --firmware`. Neither command flashes a device. A successful
build preserves a verified merged image and matching ELF/MAP bundle under
`build/firmware/`; the app-only image is not a full-device image.

To rebuild the browser module, install wasi-sdk 34 and run:

```sh
WASI_SDK_PATH=/path/to/wasi-sdk-34 ./tools/build_preview.sh
node tests/wasm_parity.cjs
```

For authoring/test dependencies, use `pnpm install --frozen-lockfile` (Node.js required).

The parity test compiles a native C replay and compares 5,000 transitions with
WebAssembly. With Playwright and Chrome installed, run
`node tests/browser_zumi.cjs` while the preview server is running. Set
`PLAYWRIGHT_CHANNEL` if using another Playwright Chromium channel. Screenshots and
machine-readable results are written under ignored `build/`.

The resource compiler is `node tools/build_zumi_assets.cjs` (requires Sharp).
Source artwork and provenance are in `assets/images/zumi-pocket/`; generated C
sprites are checked in so firmware builds do not require Node or Sharp.

See [design and hardware acceptance](docs/assets/zumi-pocket.md) and
[validation report](docs/assets/zumi-pocket-validation.md).

## Branch boundary

Fork: `9pk8vftyr2-gif/ai-passport`; branch: `feature/zumi-pocket`.
Based on upstream main `3d685a90bdb96dd8526d84edde1e63fec25b460e`.
Upstream and fork main stay unchanged. This project does not modify the ZCO
website, deploy to Cloudflare, flash devices, erase storage, or connect paid AI.
